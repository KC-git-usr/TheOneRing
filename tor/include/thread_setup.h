#ifndef TOR_INCLUDE_THREAD_SETUP_H_
#define TOR_INCLUDE_THREAD_SETUP_H_

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

namespace tor::thread_setup {

/// \brief Maximum number of characters allowed for a pthread name
///        (including null terminator). pthread_setname_np enforces this limit.
constexpr std::size_t kMaxPthreadNameLength{15};

/// \brief Set the name of the calling thread.
/// \param thread_name   Human-readable name for the thread (max 15 chars).
/// \return true on success, false with error msg if the syscall fails.
auto SetThreadName(std::string_view thread_name) -> std::pair<bool, std::string>;

/// \brief Set thread affinity
/// \param[in] core The id of the core that you want to set this thread
/// affinity to
/// \return true on success, false with error msg
auto SetThreadAffinity(unsigned int core) -> std::pair<bool, std::string>;

/// \brief Configure thread priority and scheduler to FIFO
/// \param[in] priority SCHED_FIFO priority [1, 99]
/// \return true on success, false with error msg
auto ConfigureSchedFifo(uint8_t priority) -> std::pair<bool, std::string>;

/// \brief Set the CPU affinity and SCHED_FIFO scheduling policy for the
/// calling thread.
/// \param thread_name   Thread name used in error messages.
/// \param cpu_core      CPU core to pin the thread to.
/// \param priority      SCHED_FIFO priority (1–99).
/// \return true on success, false with error msg if any step fails.
auto SetThreadAffinityAndSchedFifo(std::string_view thread_name,
                                   uint8_t cpu_core,
                                   uint8_t priority) -> std::pair<bool, std::string>;

/// \brief Configure a calling thread's real-time properties: name, CPU
/// affinity, and SCHED_FIFO scheduling policy.
/// \param thread_name   Human-readable name for the thread (max 15 chars).
/// \param cpu_core      CPU core to pin the thread to.
/// \param priority      SCHED_FIFO priority (1–99).
/// \return true on success, false with error msg if any step fails.
auto InitializeRealtimeThread(std::string_view thread_name,
                              uint8_t cpu_core,
                              uint8_t priority) -> std::pair<bool, std::string>;

}  // namespace tor::thread_setup

#endif  // TOR_INCLUDE_THREAD_SETUP_H_
