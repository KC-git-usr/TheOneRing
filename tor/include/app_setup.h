#ifndef TOR_INCLUDE_APP_SETUP_H_
#define TOR_INCLUDE_APP_SETUP_H_

#include <string>
#include <utility>

namespace tor::app_setup {

/// \brief Idempotent real-time environment initialization.
/// Safe to call from any thread; the rt setup is done exactly once.
/// \returns true on success, else false with an error string message.
[[nodiscard]] auto EnableRTEnv() -> std::pair<bool, std::string>;

/// \brief  Idempotent application signal handler for graceful shutdown.
/// Blocks SIGINT/SIGTERM in the calling thread (so child threads inherit the
/// mask), then spawns a dedicated thread that waits for those signals.
/// SIGALRM is ignored outright — this app uses clocknanosleep, not POSIX timers.
void SetupSignalHandler();

}  // namespace tor::app_setup

#endif  // TOR_INCLUDE_APP_SETUP_H_
