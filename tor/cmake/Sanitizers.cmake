# cmake-format: off
# cmake/Sanitizers.cmake
#
# Clang sanitizer support.  All are off by default and toggled via -D flags.
#
# Available flags:
#   -DENABLE_ASAN=ON    AddressSanitizer          — heap/stack/global memory errors
#   -DENABLE_UBSAN=ON   UndefinedBehaviorSanitizer — signed overflow, bad casts, etc.
#   -DENABLE_TSAN=ON    ThreadSanitizer            — data races
#   -DENABLE_RTSAN=ON   RealtimeSanitizer          — RT-unsafe calls in [[clang::nonblocking]]
#                                                    functions (Clang 18+)
#
# RTSan usage:
#   Mark your real-time functions with [[clang::nonblocking]].
#   RTSan will abort at runtime when those functions perform:
#     - heap allocation (malloc / new)
#     - locking (mutex, futex)
#     - blocking syscalls (read, write, sleep, ...)
#   Example:
#     void rt_cycle() [[clang::nonblocking]] {
#       // RTSan watches everything called from here
#     }
#
# For best stack traces set ASAN_SYMBOLIZER_PATH / UBSAN_SYMBOLIZER_PATH to
# the path of llvm-symbolizer before running the binary.
# cmake-format: on

include_guard(GLOBAL)

# ── Combination validation (call once at configure time) ─────────────────────
function(validate_sanitizer_combinations)
  if(ENABLE_ASAN AND ENABLE_TSAN)
    message(FATAL_ERROR "ASan and TSan are mutually exclusive — they both intercept memory "
                        "allocations and cannot coexist in the same binary."
    )
  endif()

  if(ENABLE_TSAN AND ENABLE_RTSAN)
    message(FATAL_ERROR "TSan and RTSan are mutually exclusive.")
  endif()

  if(ENABLE_COVERAGE
     AND (ENABLE_ASAN
          OR ENABLE_TSAN
          OR ENABLE_UBSAN
          OR ENABLE_RTSAN
         )
  )
    message(WARNING "Mixing LLVM coverage instrumentation with sanitizers can produce "
                    "misleading results. Use separate build directories for each."
    )
  endif()
endfunction()

# ── Per-target application ───────────────────────────────────────────────────
function(apply_sanitizers target)
  set(_flags "")

  if(ENABLE_ASAN)
    list(APPEND _flags -fsanitize=address -fsanitize-address-use-after-scope)
    message(STATUS "[${target}] AddressSanitizer ON")
  endif()

  if(ENABLE_UBSAN)
    list(
      APPEND
      _flags
      -fsanitize=undefined
      -fsanitize=float-divide-by-zero
      -fsanitize=unsigned-integer-overflow
      -fsanitize=implicit-conversion
      -fsanitize=local-bounds
      -fsanitize=nullability
      -fno-sanitize-recover=undefined
    ) # Abort on UB instead of continuing
    message(STATUS "[${target}] UndefinedBehaviorSanitizer ON")
  endif()

  if(ENABLE_TSAN)
    list(APPEND _flags -fsanitize=thread)
    message(STATUS "[${target}] ThreadSanitizer ON")
  endif()

  if(ENABLE_RTSAN)
    list(APPEND _flags -fsanitize=realtime)
    message(
      STATUS "[${target}] RealtimeSanitizer ON  (annotate RT functions with [[clang::nonblocking]])"
    )
  endif()

  if(_flags)
    # -g ensures symbolised stack traces; -fno-omit-frame-pointer keeps them accurate.
    target_compile_options(${target} PRIVATE ${_flags} -g -fno-omit-frame-pointer)
    target_link_options(${target} PRIVATE ${_flags})
  endif()
endfunction()
