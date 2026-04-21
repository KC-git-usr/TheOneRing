#ifndef THE_ONE_RING_INCLUDE_INTERNAL_SIGNAL_H_
#define THE_ONE_RING_INCLUDE_INTERNAL_SIGNAL_H_

#include <atomic>

namespace tor::internal_signal {

/// Global application shutdown signal-true when shutdown signal was received. Read only, must not
/// modify!
inline std::atomic<bool> shutdown_requested{};

}  // namespace tor::internal_signal

#endif  // THE_ONE_RING_INCLUDE_INTERNAL_SIGNAL_H_
