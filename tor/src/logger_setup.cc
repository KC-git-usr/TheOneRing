#include "logger_setup.h"

#include <chrono>
#include <memory>
#include <mutex>
#include <sstream>
#include <vector>

#include "constants.h"
#include "spdlog/async.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

namespace tor::logger {

namespace detail {

void CreateLoggerImpl() {
  // Setup spdlog
  // Customize log msg format
  spdlog::set_pattern("[%H:%M:%S:%e:%f] [thread %t] [%^%l%$] %v");

  // stdout console sink
  const auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  // logs below debug severity level are not logged
  console_sink->set_level(spdlog::level::debug);

  // File sink
  // get current time for a more precise filename
  const auto now = std::chrono::system_clock::now();
  const std::time_t now_c = std::chrono::system_clock::to_time_t(now);
  std::tm tm_now{};
  ::localtime_r(&now_c, &tm_now);
  std::stringstream ss;
  // format for filename
  ss << std::put_time(&tm_now, "_%Y-%m-%d_%H:%M:%S");
  const auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
      constants::kSpdlogLogFileName + ss.str() + ".log", true);
  file_sink->set_level(spdlog::level::trace);  // support all levels

  // async logger, thread safe and non-blocking
  spdlog::init_thread_pool(constants::kSpdlogQueueSize, constants::kNumberOfSpdlogThreads);
  std::vector<spdlog::sink_ptr> ecat_log_sinks{console_sink, file_sink};
  // Application logger object
  const auto logger = std::make_shared<spdlog::async_logger>(
      constants::kSpdlogLoggerName, ecat_log_sinks.begin(), ecat_log_sinks.end(),
      spdlog::thread_pool(), spdlog::async_overflow_policy::overrun_oldest);
  spdlog::register_logger(logger);
  // By default, using spdlog::<log_level> will log only to the console, and
  // will not log to the log file. Hence, resetting the default logger to our
  // async logger.
  spdlog::set_default_logger(logger);

  // spdlog messages must pass both the global logger's minimum level and the
  // individual sink's minimum level to be logged, ensure compliance with ros2
  // rcutils logger too
  spdlog::set_level(spdlog::level::trace);
}

}  // namespace detail

void CreateLogger() {
  static std::once_flag flag;
  std::call_once(flag, detail::CreateLoggerImpl);
}

}  // namespace tor::logger
