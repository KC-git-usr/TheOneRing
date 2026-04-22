#ifndef TOR_INCLUDE_LOGGER_SETUP_H_
#define TOR_INCLUDE_LOGGER_SETUP_H_

namespace tor::logger {
namespace detail {

void CreateLoggerImpl();

}  // namespace detail

/// Thread-safe, idempotent logger initialization.
/// Safe to call from any thread; the logger is created exactly once.
void CreateLogger();

}  // namespace tor::logger

#endif  // TOR_INCLUDE_LOGGER_SETUP_H_
