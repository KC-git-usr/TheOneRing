# cmake-format: off
# cmake/CodeCoverage.cmake
#
# LLVM source-based code coverage (not gcov).
# Enable with -DENABLE_COVERAGE=ON.
#
# Workflow:
#   1.  cmake -DENABLE_COVERAGE=ON -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Debug -S . -B build-cov
#   2.  cmake --build build-cov
#   3.  cd build-cov && LLVM_PROFILE_FILE=$(pwd)/default.profraw ctest --output-on-failure
#   4.  cmake --build . --target coverage-html   # browse coverage/html/index.html
#       cmake --build . --target coverage-summary # prints per-file % to stdout
#
# Requires: llvm-profdata, llvm-cov (same major version as your clang).
# Install:  sudo apt install llvm
# cmake-format: on

include_guard(GLOBAL)

# ── Instrumentation flags (applied per target) ────────────────────────────────
function(apply_coverage target)
  if(NOT ENABLE_COVERAGE)
    return()
  endif()

  if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "ENABLE_COVERAGE requires Clang (llvm-profdata / llvm-cov).")
  endif()

  target_compile_options(
    ${target}
    PRIVATE -fprofile-instr-generate # Emit instrumented code
            -fcoverage-mapping # Embed source-to-bytecode mapping
            -O0 # No optimisation — accurate line coverage
            -g
  ) # Debug symbols for readable reports

  target_link_options(${target} PRIVATE -fprofile-instr-generate)
endfunction()

# ── Report targets (call once, after the test executable is defined) ──────────
function(setup_coverage_targets test_executable)
  if(NOT ENABLE_COVERAGE)
    return()
  endif()

  find_program(
    LLVM_PROFDATA_EXE
    NAMES llvm-profdata llvm-profdata-22 llvm-profdata-21 llvm-profdata-20 llvm-profdata-19
          llvm-profdata-18
    DOC "llvm-profdata (part of the LLVM toolchain)"
  )
  find_program(
    LLVM_COV_EXE
    NAMES llvm-cov llvm-cov-22 llvm-cov-21 llvm-cov-20 llvm-cov-19 llvm-cov-18
    DOC "llvm-cov (part of the LLVM toolchain)"
  )

  if(NOT LLVM_PROFDATA_EXE OR NOT LLVM_COV_EXE)
    message(WARNING "llvm-profdata / llvm-cov not found — coverage report targets will not "
                    "be available.  Install with: sudo apt install llvm"
    )
    return()
  endif()

  set(COV_DIR "${CMAKE_BINARY_DIR}/coverage")
  set(PROFDATA_FILE "${COV_DIR}/coverage.profdata")
  set(PROFRAW_FILE "${CMAKE_BINARY_DIR}/default.profraw")

  # Step 1: merge .profraw into .profdata
  add_custom_target(
    coverage-merge
    COMMAND ${CMAKE_COMMAND} -E make_directory ${COV_DIR}
    COMMAND ${LLVM_PROFDATA_EXE} merge --sparse ${PROFRAW_FILE} -o ${PROFDATA_FILE}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Merging raw coverage profiles → ${PROFDATA_FILE}"
    VERBATIM
  )

  # Step 2: HTML report (browse at coverage/html/index.html)
  add_custom_target(
    coverage-html
    COMMAND
      ${LLVM_COV_EXE} show $<TARGET_FILE:${test_executable}> --instr-profile=${PROFDATA_FILE}
      --format=html --output-dir=${COV_DIR}/html --show-line-counts-or-regions --show-expansions
      --ignore-filename-regex=.*/tests/.* --ignore-filename-regex=.*/googletest/.*
    DEPENDS coverage-merge
    COMMENT "HTML coverage report → ${COV_DIR}/html/index.html"
    VERBATIM
  )

  # Step 3: text summary (prints % covered per file)
  add_custom_target(
    coverage-summary
    COMMAND
      ${LLVM_COV_EXE} report $<TARGET_FILE:${test_executable}> --instr-profile=${PROFDATA_FILE}
      --ignore-filename-regex=.*/tests/.* --ignore-filename-regex=.*/googletest/.*
    DEPENDS coverage-merge
    COMMENT "Coverage summary"
    VERBATIM
  )
endfunction()
