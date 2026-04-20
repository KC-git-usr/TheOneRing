#ifndef THE_ONE_RING_INCLUDE_APP_SETUP_H_
#define THE_ONE_RING_INCLUDE_APP_SETUP_H_

#include <sys/mman.h>
#include <sys/utsname.h>

#include <charconv>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>

namespace tor::app_setup {
namespace detail {

/// Parse "major.minor.patch" from a kernel version string.
/// Returns true on success.
inline auto ParseKernelVersion(const char* release, int* major, int* minor, int* patch) -> bool {
  std::string_view sv(release);

  // Parse one integer, consume it from sv, return success.
  auto next_int = [&sv](int* out) -> bool {
    auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), *out);
    if (ec != std::errc{}) {
      return false;
    }
    sv.remove_prefix(static_cast<size_t>(ptr - sv.data()));
    return true;
  };

  // Consume a '.' separator from sv.
  auto expect_dot = [&sv]() -> bool {
    if (sv.empty() || sv.front() != '.') {
      return false;
    }
    sv.remove_prefix(1);
    return true;
  };

  return next_int(major) && expect_dot() && next_int(minor) && expect_dot() && next_int(patch);
}

[[nodiscard]] inline auto EnableRTEnvImpl() -> std::pair<bool, std::string> {
  // minimum >= 2.6 kernel | okay >= 5.15 kernel | recommended >= 6.12 kernel
  utsname system_name{};
  if (uname(&system_name) != 0) {
    return {false, "ERROR calling uname(), required Linux kernel >= 2.6"};
  }

  int major{};
  int minor{};
  int patch{};
  if (!ParseKernelVersion(system_name.release, &major, &minor, &patch)) {
    return {false, "Failed to parse kernel version from: " + std::string(system_name.release)};
  }

  if ((major != 2 || minor < 6) && major < 3) {
    return {false, "Detected kernel = " + std::to_string(major) + "." + std::to_string(minor) +
                       "." + std::to_string(patch) + ", required Linux kernel >= 2.6"};
  }

  // disable paging
  if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
    return {false, "Cannot disable paging! Ensure you are running as root user!"};
  }

  return {true, {}};
}

}  // namespace detail

/// Thread-safe, idempotent real-time environment initialization.
/// Safe to call from any thread; the rt setup is done exactly once.
/// \returns true on success, else false with an error string message.
[[nodiscard]] inline auto EnableRTEnv() -> std::pair<bool, std::string> {
  static std::pair<bool, std::string> result;
  static std::once_flag flag;
  std::call_once(flag, []() { result = detail::EnableRTEnvImpl(); });
  return result;
}

}  // namespace tor::app_setup

#endif  // THE_ONE_RING_INCLUDE_APP_SETUP_H_
