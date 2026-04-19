cmake_minimum_required(VERSION 3.20)
project(the_one_ring VERSION 0.1.0 LANGUAGES CXX)

# ── Standards ─────────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD          20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)   # No GNU extensions; strict ISO C++20

# Generate compile_commands.json → consumed by clang-tidy, clangd, and ccls.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── Module path ───────────────────────────────────────────────────────────────
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# ── Compiler guard ────────────────────────────────────────────────────────────
# RTSan, LLVM coverage, and several warning flags are Clang-specific.
if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  message(FATAL_ERROR
    "This project requires Clang (detected: ${CMAKE_CXX_COMPILER_ID}). "
    "Re-run with CC=clang CXX=clang++.")
endif()

# ── Feature options ───────────────────────────────────────────────────────────
# Sanitizers (mutually exclusive combinations are validated in Sanitizers.cmake)
option(ENABLE_ASAN       "AddressSanitizer"                   OFF)
option(ENABLE_UBSAN      "UndefinedBehaviorSanitizer"         OFF)
option(ENABLE_TSAN       "ThreadSanitizer"                    OFF)
option(ENABLE_RTSAN      "RealtimeSanitizer — Clang 18+"      OFF)  # -fsanitize=realtime

# Coverage / analysis / docs
option(ENABLE_COVERAGE   "LLVM source-based code coverage"    OFF)
option(ENABLE_CLANG_TIDY "Run clang-tidy during compilation"  OFF)
option(ENABLE_CPPCHECK   "Run cppcheck during compilation"    OFF)
option(ENABLE_DOXYGEN    "Generate Doxygen documentation"     OFF)

# Tests
option(BUILD_TESTING     "Build GTest unit tests"             OFF)

# ── Base diagnostics (Google C++ Style Guide aligned) ─────────────────────────
add_compile_options(
  -Wall                       # Enable most warnings
  -Wextra                     # Extra warnings beyond -Wall
  -Wpedantic                  # Strict ISO compliance
  -Werror                     # Warnings → errors (enforced style)
  -Wshadow                    # Variable shadowing
  -Wnon-virtual-dtor          # Non-virtual dtor in polymorphic classes
  -Wold-style-cast            # No C-style casts
  -Wcast-align                # Alignment-breaking casts
  -Wcast-qual                 # Dropping const/volatile qualifiers
  -Wunused                    # Unused variables / parameters
  -Woverloaded-virtual        # Hidden virtual function overloads
  -Wconversion                # Implicit narrowing conversions
  -Wsign-conversion           # Signed/unsigned implicit conversions
  -Wnull-dereference          # Potential null dereference
  -Wdouble-promotion          # float → double implicit promotion
  -Wformat=2                  # printf-family format string safety
  -Wimplicit-fallthrough      # Missing [[fallthrough]] in switch
  -fno-omit-frame-pointer     # Retain frame pointers for stack traces / profiling
)

# ── Sanitizers ────────────────────────────────────────────────────────────────
# RTSan usage: annotate real-time functions with [[clang::nonblocking]].
# RTSan will abort at runtime if those functions perform blocking syscalls,
# heap allocation, or any other real-time-unsafe operation.
include(Sanitizers)
validate_sanitizer_combinations()

# ── Code coverage ─────────────────────────────────────────────────────────────
# Uses llvm-profdata + llvm-cov (LLVM source-based, not gcov).
# Workflow:  cmake -DENABLE_COVERAGE=ON ..
#            cmake --build . --target the_one_ring_tests
#            LLVM_PROFILE_FILE=default.profraw ./the_one_ring_tests
#            cmake --build . --target coverage-html
include(CodeCoverage)

# ── Static analysis ───────────────────────────────────────────────────────────
if(ENABLE_CLANG_TIDY)
  find_program(CLANG_TIDY_EXE NAMES clang-tidy REQUIRED
    DOC "clang-tidy executable")
  message(STATUS "clang-tidy : ${CLANG_TIDY_EXE}")
  # .clang-tidy is discovered automatically by walking up from each source file.
  set(CMAKE_CXX_CLANG_TIDY
    "${CLANG_TIDY_EXE}"
    "--header-filter=${CMAKE_CURRENT_SOURCE_DIR}/include/.*|${CMAKE_CURRENT_SOURCE_DIR}/src/.*")
endif()

if(ENABLE_CPPCHECK)
  find_program(CPPCHECK_EXE NAMES cppcheck REQUIRED
    DOC "cppcheck executable")
  message(STATUS "cppcheck   : ${CPPCHECK_EXE}")
  set(CMAKE_CXX_CPPCHECK
    "${CPPCHECK_EXE}"
    "--std=c++20"
    "--enable=all"
    "--inconclusive"
    "--inline-suppr"
    "--suppress=missingIncludeSystem"
    "--error-exitcode=1")
endif()

# ── Main executable ───────────────────────────────────────────────────────────
#file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")
set(SOURCES
src/main.cc)

if(SOURCES)
  add_executable(${PROJECT_NAME} ${SOURCES})

  target_include_directories(${PROJECT_NAME}
    PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

  target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

  # Real-time: POSIX timers (librt) + pthreads
  target_link_libraries(${PROJECT_NAME} PRIVATE rt pthread)

  apply_sanitizers(${PROJECT_NAME})
  # Coverage is applied to the test binary in tests/CMakeLists.txt.
  # The main RT executable runs indefinitely and is not amenable to profiling.
else()
  message(STATUS "No .cpp files found in src/ — skipping executable target.")
endif()

# ── Unit tests ────────────────────────────────────────────────────────────────
if(BUILD_TESTING)
  enable_testing()
  add_subdirectory(tests)
endif()

# ── Doxygen ───────────────────────────────────────────────────────────────────
if(ENABLE_DOXYGEN)
  include(Doxygen)
  generate_doxygen()
endif()

# ── Format / lint convenience targets ────────────────────────────────────────
file(GLOB_RECURSE ALL_CXX_FILES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.h"
)

find_program(CLANG_FORMAT_EXE NAMES clang-format)
if(CLANG_FORMAT_EXE)
  add_custom_target(format
    COMMAND ${CLANG_FORMAT_EXE} --style=file -i ${ALL_CXX_FILES}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Auto-formatting with clang-format"
    VERBATIM)
  add_custom_target(format-check
    COMMAND ${CLANG_FORMAT_EXE} --style=file --dry-run --Werror ${ALL_CXX_FILES}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Checking clang-format compliance (no changes written)"
    VERBATIM)
endif()

find_program(CPPLINT_EXE NAMES cpplint cpplint.py)
if(CPPLINT_EXE AND ALL_CXX_FILES)
  add_custom_target(cpplint
    COMMAND ${CPPLINT_EXE}
      --filter=-build/include_subdir,-legal/copyright
      --linelength=100
      --counting=detailed
      ${ALL_CXX_FILES}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running cpplint (Google C++ Style Guide)"
    VERBATIM)
endif()
