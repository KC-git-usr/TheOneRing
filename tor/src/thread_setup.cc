#include "thread_setup.h"

#include <pthread.h>

#include <string>
#include <string_view>
#include <utility>

namespace tor::thread_setup {

auto SetThreadName(std::string_view thread_name) -> std::pair<bool, std::string> {
  // pthread_setname_np requires a null-terminated string.
  const std::string name_str(thread_name);
  if (const auto result = ::pthread_setname_np(::pthread_self(), name_str.c_str()); result != 0) {
    return {false, "Failed to set " + name_str +
                       " thread name, raw error code: " + std::to_string(result)};
  }
  return {true, {}};
}

}  // namespace tor::thread_setup
