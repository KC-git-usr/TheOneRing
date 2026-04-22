#ifndef TOR_INCLUDE_CONSTANTS_H_
#define TOR_INCLUDE_CONSTANTS_H_

#include <string_view>

namespace tor::constants {

/// spdlog logger name
constexpr auto kSpdlogLoggerName{"tor_logger"};
/// spdlog async queue size
constexpr auto kSpdlogQueueSize{8192 * 4};  // 8192 * 4 * (256 bytes) = 8 MB
/// spdlog async number of threads
constexpr auto kNumberOfSpdlogThreads{1};
/// spdlog log file name, with full path, conforming to ros2 file
/// structure std
constexpr auto kSpdlogLogFileName{"log/tor_logs"};

/// Application signal handler thread name
constexpr std::string_view kSignalHandlerThreadName{"SignalHandler"};
/// Application timer thread name
constexpr std::string_view kTimerThreadName{"Timer"};

/// Application timer thread priority
constexpr auto kTimerThreadPriority{99};

}  // namespace tor::constants

#endif  // TOR_INCLUDE_CONSTANTS_H_
