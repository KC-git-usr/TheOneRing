#include "app_setup.h"
#include "logger.h"

int main() {
  tor::logger::CreateLogger();

  if (const auto [result, err_msg] = tor::app_setup::EnableRTEnv(); !result) {
    spdlog::critical(err_msg);
    return -1;
  }

  spdlog::info("Application exited");
  return 0;
}
