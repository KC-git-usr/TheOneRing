#include "thread_setup.h"

#include <pthread.h>
#include <sched.h>

#include <cassert>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace tor::thread_setup {

auto SetThreadName(std::string_view thread_name) -> std::pair<bool, std::string> {
  // pthread_setname_np requires a null-terminated string.
  const std::string name_str(thread_name);
  if (const auto result = ::pthread_setname_np(pthread_self(), name_str.c_str()); result != 0) {
    return {false, "Failed to set " + name_str +
                       " thread name, raw error code: " + std::to_string(result)};
  }
  return {true, {}};
}

auto SetThreadAffinity(const unsigned int core) -> std::pair<bool, std::string> {
  std::string message{};
  auto set_affinity_result_message = [](const int result, std::string& msg) -> bool {
    if (result == 0) {
      return true;
    }
    switch (result) {
      case EFAULT:
        msg = "Call of sched_setaffinity with invalid cpuset!";
        break;
      case EINVAL:
        msg = "Call of sched_setaffinity with an invalid cpu core!";
        break;
      case ESRCH:
        msg =
            "Call of sched_setaffinity with a thread id/process id that is "
            "invalid or not found!";
        break;
      case EPERM:
        msg = "Call of sched_setaffinity with insufficient privileges!";
        break;
      default:
        msg = "Unknown error code: " + std::to_string(result);
    }
    return false;
  };
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(core, &cpuset);

  const auto result = set_affinity_result_message(
      pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset), message);
  return std::make_pair(result, message);
}

auto ConfigureSchedFifo(const uint8_t priority) -> std::pair<bool, std::string> {
  int const min = sched_get_priority_min(SCHED_FIFO);
  int const max = sched_get_priority_max(SCHED_FIFO);
  if (min == -1 || max == -1) {
    return {false, "Failed to get priority limits"};
  }
  if (priority < static_cast<uint8_t>(min) || priority > static_cast<uint8_t>(max)) {
    return {false, "Priority out of range"};
  }
  sched_param schedp{};
  schedp.sched_priority = priority;
  return {(pthread_setschedparam(pthread_self(), SCHED_FIFO, &schedp) == 0), ""};
}

auto SetThreadAffinityAndSchedFifo(std::string_view thread_name,
                                   const uint8_t cpu_core,
                                   const uint8_t priority) -> std::pair<bool, std::string> {
  std::string error_msg{};
  if (const auto [result, err_msg] = SetThreadAffinity(cpu_core); !result) {
    error_msg =
        "Failed to set " + std::string(thread_name) + " thread affinity, error message: " + err_msg;
    return {false, error_msg};
  }
  if (const auto [result, err_msg] = ConfigureSchedFifo(priority); !result) {
    error_msg = "Failed to set " + std::string(thread_name) +
                " thread SCHED_FIFO scheduling policy, error message: " + err_msg +
                " errno: " + std::error_code(errno, std::generic_category()).message();
    return {false, error_msg};
  }
  return {true, error_msg};
}

auto InitializeRealtimeThread(std::string_view thread_name,
                              const uint8_t cpu_core,
                              const uint8_t priority) -> std::pair<bool, std::string> {
  if (const auto result = SetThreadName(thread_name); !result.first) {
    return result;
  }
  return SetThreadAffinityAndSchedFifo(thread_name, cpu_core, priority);
}

}  // namespace tor::thread_setup
