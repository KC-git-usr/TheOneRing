#include "app_setup.h"

#include <sys/mman.h>
#include <sys/utsname.h>

#include <charconv>
#include <csignal>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

#include "constants.h"
#include "internal_signal.h"
#include "spdlog/spdlog.h"
#include "thread_setup.h"

namespace tor::app_setup {
namespace detail {

auto ParseKernelVersion(const char* release, int* major, int* minor, int* patch) -> bool {
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

[[nodiscard]] auto EnableRTEnvImpl() -> std::pair<bool, std::string> {
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

void SignalHandler(sigset_t sigset) {
  static_assert(constants::kSignalHandlerThreadName.size() <= thread_setup::kMaxPthreadNameLength,
                "Signal handler thread name exceeds pthread name length limit");
  if (const auto [result, err_msg] =
          thread_setup::SetThreadName(constants::kSignalHandlerThreadName);
      !result) {
    spdlog::error(err_msg);
  }
  int sig{};
  // Blocks until SIGINT or SIGTERM is delivered.
  if (sigwait(&sigset, &sig) == 0) {
    internal_signal::shutdown_requested.store(true);
    internal_signal::shutdown_requested.notify_all();
  }
}

void SetupSignalHandlerImpl() {
  // Ignore SIGALRM — not used by this application.
  signal(SIGALRM, SIG_IGN);

  sigset_t sigset;
  sigemptyset(&sigset);
  sigaddset(&sigset, SIGINT);
  sigaddset(&sigset, SIGTERM);
  // Block in the main thread so all child threads inherit the mask.
  pthread_sigmask(SIG_BLOCK, &sigset, nullptr);
  // Spawn a dedicated signal-wait thread (detached — runs until process exits).
  std::thread(SignalHandler, sigset).detach();
}

}  // namespace detail

[[nodiscard]] auto EnableRTEnv() -> std::pair<bool, std::string> {
  static std::pair<bool, std::string> result;
  static std::once_flag flag;
  std::call_once(flag, []() { result = detail::EnableRTEnvImpl(); });
  return result;
}

void SetupSignalHandler() {
  static std::once_flag flag;
  std::call_once(flag, detail::SetupSignalHandlerImpl);
}

}  // namespace tor::app_setup
