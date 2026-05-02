#ifndef TOR_INCLUDE_INTERNAL_SIGNAL_H_
#define TOR_INCLUDE_INTERNAL_SIGNAL_H_

#include <atomic>

namespace tor::internal_signal {

namespace detail {
/// Private: only app_setup may write to this flag.
inline std::atomic<bool> shutdown_requested_flag{};
}  // namespace detail

/// \brief Returns true once a SIGINT/SIGTERM has been received.
/// Read-only access for all consumers; only app_setup::SignalHandler writes it.
[[nodiscard]] inline auto ShutdownRequested() noexcept -> bool {
  return detail::shutdown_requested_flag.load(std::memory_order_acquire);
}

/// \brief Block until shutdown is requested (mirrors the old .wait(false) usage).
inline auto WaitForShutdown() noexcept -> void {
  detail::shutdown_requested_flag.wait(false, std::memory_order_acquire);
}

}  // namespace tor::internal_signal

#endif  // TOR_INCLUDE_INTERNAL_SIGNAL_H_
