# cmake/CodeCoverage.cmake
#
# LLVM source-based code coverage (not gcov). Enable with -DENABLE_COVERAGE=ON.
#
# Workflow: 1.  cmake -DENABLE_COVERAGE=ON -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Debug .. 2.  cmake
# --build . 3.  LLVM_PROFILE_FILE=default.profraw ctest --output-on-failure 4.  cmake --build .
# --target coverage-html   # opens build/coverage/html/index.html cmake --build . --target
# coverage-summary # prints per-file % to stdout
#
# Requires: llvm-profdata, llvm-cov (same major version as your clang). Install:  sudo apt install
# llvm

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
    NAMES llvm-profdata
    DOC "llvm-profdata (part of the LLVM toolchain)"
  )
  find_program(
    LLVM_COV_EXE
    NAMES llvm-cov
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

  # Step 1: merge all .profraw files into a single .profdata
  add_custom_target(
    coverage-merge
    COMMAND ${CMAKE_COMMAND} -E make_directory "${COV_DIR}"
    COMMAND ${LLVM_PROFDATA_EXE} merge --sparse "${CMAKE_BINARY_DIR}/default.profraw" -o
            "${PROFDATA_FILE}"
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    COMMENT "Merging raw coverage profiles → ${PROFDATA_FILE}"
    VERBATIM
  )

  # Step 2: HTML report (browse at coverage/html/index.html)
  add_custom_target(
    coverage-html
    COMMAND
      ${LLVM_COV_EXE} show $<TARGET_FILE:${test_executable}> --instr-profile="${PROFDATA_FILE}"
      --format=html --output-dir="${COV_DIR}/html" --show-line-counts-or-regions --show-expansions
      --ignore-filename-regex=".*/tests/.*" --ignore-filename-regex=".*/googletest/.*"
    DEPENDS coverage-merge
    COMMENT "HTML coverage report → ${COV_DIR}/html/index.html"
    VERBATIM
  )

  # Step 3: text summary (prints % covered per file)
  add_custom_target(
    coverage-summary
    COMMAND
      ${LLVM_COV_EXE} report $<TARGET_FILE:${test_executable}> --instr-profile="${PROFDATA_FILE}"
      --ignore-filename-regex=".*/tests/.*" --ignore-filename-regex=".*/googletest/.*"
    DEPENDS coverage-merge
    COMMENT "Coverage summary"
    VERBATIM
  )
endfunction()
