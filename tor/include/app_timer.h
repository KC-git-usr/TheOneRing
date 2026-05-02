#ifndef TOR_INCLUDE_APP_TIMER_H_
#define TOR_INCLUDE_APP_TIMER_H_

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace tor::app_timer {

/// \brief Class that encapsulates a periodic timer implemented with a dedicated
/// thread that sleeps until the next timer tick, and notifies waiting cyclic
/// tasks via a std::condition_variable. The timer thread is configured with
/// real-time properties, including CPU affinity and scheduling policy
/// \param cpu_core_number the CPU core number to set the timer thread affinity
/// \param timer_thread_priority the priority to set for the timer thread, from
/// 1 to 99 for SCHED_FIFO, else default to SCHED_OTHER if no params provided.
/// \param interval the duration between each timer tick
class PeriodicTimer {
 public:
  explicit PeriodicTimer(std::chrono::steady_clock::duration interval);

  explicit PeriodicTimer(uint8_t cpu_core_number,
                         uint8_t timer_thread_priority,
                         std::chrono::steady_clock::duration interval);

  ~PeriodicTimer() { Stop(); }

  PeriodicTimer(const PeriodicTimer&) = delete;
  PeriodicTimer& operator=(const PeriodicTimer&) = delete;
  PeriodicTimer(PeriodicTimer&&) = delete;
  PeriodicTimer& operator=(PeriodicTimer&&) = delete;

  auto Start() -> void { start_timer_.release(); }

  auto Stop() -> void {
    // Guard against double-release: only release the semaphore on the first Stop().
    if (!stopped_.test_and_set()) {
      start_timer_.release();
    }
    timer_thread_.request_stop();
  }

  /// \brief Block the current thread until the next timer tick has been
  /// notified.
  /// \tparam N the multiple of timer ticks to wait on, e.g. N=2 means wait
  ///           until 2 timer ticks have occurred, i.e. run this task at half
  ///           the timer frequency. N must be at least 1. If the task overruns,
  ///           it will run immediately on the next available tick and resume
  ///           the N-tick pattern from there (catch-up, not skip).
  /// \param last_seen_tick pointer to the last seen tick count for this task.
  ///        Must be initialised to 0 before the first call; pass the same
  ///        variable on every subsequent call so the task tracks its own
  ///        position in the tick sequence.
  /// \param stoken stop token to allow the wait to be interrupted if stop is
  ///        requested.
  /// \return true if a tick was received; false if stop was requested.
  template <std::uint64_t N>
  [[nodiscard]] auto WaitForNextTick(uint64_t* last_seen_tick,
                                     const std::stop_token& stoken) -> bool {
    static_assert(N >= 1, "N must be at least 1");
    // condition_variable has no stop_token overload, so register a
    // stop_callback that calls notify_all() — this wakes the wait so the
    // predicate can see the stop. std::stop_callback is scoped to exactly the
    // duration of one wait() call — created just before, destroyed just after.
    const std::stop_callback stop_wake{stoken, [this]() { timer_cv_.notify_all(); }};
    std::unique_lock lock(timer_mutex_);
    // Wait until tick_count advances by N, or a stop is requested.
    // Algorithm note: tick_count - last_seen_tick >= N was chosen since the
    // task may overrun, but still allows the task to catch up.
    timer_cv_.wait(lock, [this, &last_seen_tick, &stoken]() {
      return stoken.stop_requested() || ((tick_count_ - *last_seen_tick) >= N);
    });
    if (stoken.stop_requested()) {
      return false;
    }
    *last_seen_tick = tick_count_;
    return true;
  }

 private:
  /// \brief Entry point for the timer thread, which runs the periodic timer
  /// loop
  auto PeriodicTimerEntry(const std::stop_token& stoken) -> void;

  /// \brief Set real-time properties for the timer thread, including CPU
  /// affinity and scheduling policy
  [[nodiscard]] auto InitializeThreadProperties() const -> bool;

  /// \brief: Periodic timer that notifies timer ticks via
  /// std::condition_variable
  /// \param stoken stop token to signal timer thread to stop
  /// \param interval the duration between each timer tick
  auto RunPeriodicTimer(const std::stop_token& stoken,
                        std::chrono::steady_clock::duration interval) -> bool;

  /// \brief CPU core number to set the timer thread affinity to
  uint8_t cpu_core_number_{};
  /// \brief Timer thread priority number (99 to 1 for SCHED_FIFO)
  uint8_t timer_thread_priority_{};
  /// \brief Whether to use SCHED_FIFO or SCHED_OTHER scheduling policy for the
  /// timer thread
  bool use_sched_fifo_{false};
  /// \brief Timer tick interval duration
  std::chrono::steady_clock::duration interval_{};

  std::mutex timer_mutex_;
  std::condition_variable timer_cv_;
  std::uint64_t tick_count_{};

  std::jthread timer_thread_;
  /// Guard to prevent double-release of start_timer_ if Stop() is called after Start()
  std::atomic_flag stopped_{};
  /// Signal from timer thread to constructor about initialization status
  std::binary_semaphore timer_thread_initialization_{0};
  std::atomic<bool> timer_thread_initialize_result_{false};
  /// Signal from main thread to timer thread to start the timer loop after
  /// successful initialization
  std::binary_semaphore start_timer_{0};
};

/// \brief Non-thread safe class to record and print cycle time statistics for a
/// periodic cyclic task.
class CycleTimeStatistics {
 public:
  /// \brief Automatically record cycle time statistics for the current cycle.
  /// Should be called once per cycle, ideally at the very beginning of the
  /// cycle before any work is done, so that the recorded cycle time reflects
  /// the full cycle duration including any potential overruns.
  auto RecordCycleTimeStatistics() -> void;
  /// \brief Print the recorded cycle time statistics (min, average, max) to the
  /// log.
  /// \param task_name the name of the task for logging purposes.
  auto PrintCycleTimeStatistics(std::string_view task_name) -> void;

 private:
  std::uint64_t sample_count_{0};
  std::chrono::duration<double, std::micro> min_interval_{
      std::chrono::duration<double, std::micro>::max()};
  std::chrono::duration<double, std::micro> avg_interval_{0.0};
  std::chrono::duration<double, std::micro> max_interval_{0.0};
  std::chrono::steady_clock::time_point prev_cycle_start_time_;
  // Skip the first few cycles before recording statistics to avoid startup
  // jitter (timer thread and job task thread may fire in rapid succession
  // before the periodic timer settles into its steady cadence).
  static constexpr uint64_t kWarmupCycles_{5};
  uint64_t cycle_count_{0};
};

}  // namespace tor::app_timer

#endif  // TOR_INCLUDE_APP_TIMER_H_
