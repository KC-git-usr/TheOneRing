#include "app_setup.h"
#include "internal_signal.h"
#include "logger.h"

int main() {
  tor::app_setup::SetupSignalHandler();
  tor::logger::CreateLogger();
  if (const auto [result, err_msg] = tor::app_setup::EnableRTEnv(); !result) {
    spdlog::critical(err_msg);
    return -1;
  }

  tor::internal_signal::shutdown_requested.wait(false);
  spdlog::info("Shutdown command received");
  spdlog::info("Application exited");
  return 0;
}
