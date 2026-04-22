#ifndef THE_ONE_RING_INCLUDE_LOGGER_SETUP_H_
#define THE_ONE_RING_INCLUDE_LOGGER_SETUP_H_

namespace tor::logger {
namespace detail {

void CreateLoggerImpl();

}  // namespace detail

/// Thread-safe, idempotent logger initialization.
/// Safe to call from any thread; the logger is created exactly once.
void CreateLogger();

}  // namespace tor::logger

#endif  // THE_ONE_RING_INCLUDE_LOGGER_SETUP_H_
