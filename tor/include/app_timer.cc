
#include "app_timer.h"

#include <algorithm>
#include <csignal>

#include "constants.h"
#include "spdlog/spdlog.h"
#include "thread_setup.h"

namespace tor::app_timer {

PeriodicTimer::PeriodicTimer(const std::chrono::steady_clock::duration interval)
    : interval_(interval) {
  timer_thread_ = std::jthread(std::bind_front(&PeriodicTimer::PeriodicTimerEntry, this));
  // Block until the timer thread signals initialization success or failure.
  timer_thread_initialization_.acquire();
  if (!timer_thread_initialize_result_) {
    // Clean up the already-started thread before propagating the failure.
    Stop();
    spdlog::critical("Failed to initialize PeriodicTimer thread");
    std::raise(SIGTERM);
  }
}

PeriodicTimer::PeriodicTimer(const uint8_t cpu_core_number,
                             const uint8_t timer_thread_priority,
                             const std::chrono::steady_clock::duration interval)
    : cpu_core_number_(cpu_core_number),
      timer_thread_priority_(timer_thread_priority),
      use_sched_fifo_(true),
      interval_(interval) {
  // Dev TL;DR:
  // HAPPY PATH: spawn timer thread -> set timer thread properties success -> run timer logic in
  // timer thread NON-HAPPY PATH: spawn timer thread -> set timer thread properties failed ->
  // shutdown timer thread and raise SIGTERM from c'tor
  timer_thread_ = std::jthread(std::bind_front(&PeriodicTimer::PeriodicTimerEntry, this));
  // Block until the timer thread signals initialization success or failure.
  timer_thread_initialization_.acquire();
  if (!timer_thread_initialize_result_) {
    // Clean up the already-started thread before propagating the failure.
    Stop();
    spdlog::critical("Failed to initialize PeriodicTimer thread");
    std::raise(SIGTERM);
  }
}

auto PeriodicTimer::PeriodicTimerEntry(const std::stop_token& stoken) -> void {
  timer_thread_initialize_result_ = InitializeThreadProperties();
  // Always release the semaphore so the constructor unblocks regardless of
  // whether initialization succeeded or failed.
  timer_thread_initialization_.release();
  if (!timer_thread_initialize_result_) {
    return;
  }
  if (const auto result = RunPeriodicTimer(stoken, interval_); !result) {
    spdlog::critical("RunPeriodicTimer did not complete successfully, exiting");
  }
}

auto PeriodicTimer::InitializeThreadProperties() const -> bool {
  static_assert(constants::kTimerThreadName.size() <= thread_setup::kMaxPthreadNameLength,
                "Timer thread name exceeds pthread limit of 15 characters");
  if (use_sched_fifo_) {
    if (const auto [result, err_msg] = thread_setup::InitializeRealtimeThread(
            constants::kTimerThreadName, cpu_core_number_, timer_thread_priority_);
        !result) {
      spdlog::critical("Failed to initialize {} thread properties. Error msg: {}",
                       constants::kTimerThreadName, err_msg);
      return false;
    }
  }
  return true;
}

auto PeriodicTimer::RunPeriodicTimer(const std::stop_token& stoken,
                                     const std::chrono::steady_clock::duration interval) -> bool {
  // wait until Start() is called before starting the timer loop
  start_timer_.acquire();
  spdlog::debug("Started Periodic Timer with interval {} us",
                std::chrono::duration_cast<std::chrono::microseconds>(interval).count());

  timespec next_period{};
  if (const auto result = ::clock_gettime(CLOCK_MONOTONIC, &next_period); result != 0) {
    spdlog::error("Failed to get current time for timer initialization. errno: {}",
                  std::error_code(errno, std::generic_category()).message());
    return false;
  }

  // Pre-compute the interval as a timespec once, outside the loop.
  const auto interval_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(interval).count();
  constexpr auto kNsecPerSec{1'000'000'000L};

  while (!stoken.stop_requested()) {
    next_period.tv_nsec += interval_ns;
    // Normalise: carry any overflow in nanoseconds into seconds.
    if (next_period.tv_nsec >= kNsecPerSec) {
      next_period.tv_sec += next_period.tv_nsec / kNsecPerSec;
      next_period.tv_nsec %= kNsecPerSec;
    }
    if (const auto result =
            ::clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_period, nullptr);
        result != 0) {
      spdlog::error("clock_nanosleep() failed with raw error code: {}", result);
      timer_cv_.notify_all();
      return false;
    }
    {
      std::scoped_lock<std::mutex> const lock(timer_mutex_);
      // advance the tick counter under the lock
      ++tick_count_;
    }
    // Wake ALL waiting task threads at once; timer is unaware of subscribers
    timer_cv_.notify_all();
  }
  // Final notify so all tasks can observe stop_requested and exit
  timer_cv_.notify_all();

  spdlog::debug("Exiting RunPeriodicTimer()");
  return true;
}

auto CycleTimeStatistics::RecordCycleTimeStatistics() -> void {
  const auto current_cycle_start_time = std::chrono::steady_clock::now();
  if (cycle_count_ >= kWarmupCycles_) {
    const std::chrono::duration<double, std::micro> current_interval =
        current_cycle_start_time - prev_cycle_start_time_;
    max_interval_ = std::max(current_interval, max_interval_);
    min_interval_ = std::min(current_interval, min_interval_);
    ++sample_count_;
    // Welford online mean: avg += (x - avg) / n
    avg_interval_ += (current_interval - avg_interval_) / static_cast<double>(sample_count_);
  }
  ++cycle_count_;
  prev_cycle_start_time_ = current_cycle_start_time;
}

auto CycleTimeStatistics::PrintCycleTimeStatistics(std::string_view task_name) -> void {
  if (sample_count_ > 0) {
    spdlog::debug(
        "{} cycle time statistics over {} samples: min = {:.3f} us, avg = "
        "{:.3f} "
        "us, max = {:.3f} us",
        task_name, sample_count_, min_interval_.count(), avg_interval_.count(),
        max_interval_.count());
  } else {
    spdlog::warn("No cycle time samples recorded; cycle statistics not computed.");
  }
}

}  // namespace tor::app_timer
