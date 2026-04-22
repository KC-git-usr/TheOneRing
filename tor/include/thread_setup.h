#ifndef TOR_INCLUDE_THREAD_SETUP_H_
#define TOR_INCLUDE_THREAD_SETUP_H_

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

}  // namespace tor::thread_setup

#endif  // TOR_INCLUDE_THREAD_SETUP_H_
