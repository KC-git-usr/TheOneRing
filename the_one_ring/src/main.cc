#include "logger.h"

int main() {
  tor::logger::CreateLogger();

  spdlog::info("Hello, Mellon!");
  return 0;
}
