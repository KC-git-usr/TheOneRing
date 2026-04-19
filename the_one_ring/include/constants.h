#ifndef THE_ONE_RING_INCLUDE_CONSTANTS_H_
#define THE_ONE_RING_INCLUDE_CONSTANTS_H_

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

}  // namespace tor::constants

#endif  // THE_ONE_RING_INCLUDE_CONSTANTS_H_
