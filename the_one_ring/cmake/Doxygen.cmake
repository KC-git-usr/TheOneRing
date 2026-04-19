# cmake/Doxygen.cmake
#
# Doxygen documentation generation. Enable with -DENABLE_DOXYGEN=ON.
#
# Produces HTML docs in <build>/docs/html/index.html. Requires: doxygen, graphviz (for call graphs).
# Install:  sudo apt install doxygen graphviz

include_guard(GLOBAL)

# Generate Doxygen HTML documentation for the project.
function(generate_doxygen)
  #[[Generate Doxygen HTML documentation for the project.
  Requires: doxygen (and optionally graphviz for call graphs).]]
  find_package(Doxygen REQUIRED)

  # ── Project metadata ────────────────────────────────────────────────────────
  set(DOXYGEN_PROJECT_NAME "${PROJECT_NAME}")
  set(DOXYGEN_PROJECT_VERSION "${PROJECT_VERSION}")
  set(DOXYGEN_PROJECT_BRIEF "4000 Hz PREEMPT_RT real-time timer — C++20")

  # ── Output ──────────────────────────────────────────────────────────────────
  set(DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/docs")
  set(DOXYGEN_GENERATE_HTML YES)
  set(DOXYGEN_GENERATE_LATEX NO)
  set(DOXYGEN_HTML_DYNAMIC_MENUS YES)
  set(DOXYGEN_GENERATE_TREEVIEW YES)

  # ── Extraction ──────────────────────────────────────────────────────────────
  set(DOXYGEN_EXTRACT_ALL YES)
  set(DOXYGEN_EXTRACT_PRIVATE YES)
  set(DOXYGEN_EXTRACT_STATIC YES)
  set(DOXYGEN_BUILTIN_STL_SUPPORT YES)

  # ── README as landing page ────────────────────────────────────────────────── Resolve relative to
  # the git root (one level above the package source dir).
  set(DOXYGEN_USE_MDFILE_AS_MAINPAGE "${CMAKE_CURRENT_SOURCE_DIR}/../README.md")

  # ── Call / caller graphs (need Graphviz / dot) ─────────────────────────────
  find_program(
    DOT_EXE
    NAMES dot
    DOC "Graphviz dot (optional, for graphs)"
  )
  if(DOT_EXE)
    set(DOXYGEN_HAVE_DOT YES)
    set(DOXYGEN_CALL_GRAPH YES)
    set(DOXYGEN_CALLER_GRAPH YES)
    set(DOXYGEN_CLASS_DIAGRAMS YES)
    set(DOXYGEN_INCLUDE_GRAPH YES)
    set(DOXYGEN_COLLABORATION_GRAPH YES)
    set(DOXYGEN_DOT_MULTI_TARGETS YES)
  else()
    message(STATUS "Graphviz 'dot' not found — call/caller graphs disabled. "
                   "Install with: sudo apt install graphviz"
    )
  endif()

  # ── Clang-assisted parsing (accurate C++20 template / concept resolution) ──
  set(DOXYGEN_CLANG_ASSISTED_PARSING YES)
  set(DOXYGEN_CLANG_ADD_INC_PATHS YES)
  set(DOXYGEN_CLANG_OPTIONS "-std=c++20")

  # ── Quality enforcement ─────────────────────────────────────────────────────
  set(DOXYGEN_WARN_IF_UNDOCUMENTED YES)
  set(DOXYGEN_WARN_IF_DOC_ERROR YES)
  set(DOXYGEN_WARN_NO_PARAMDOC YES)
  # Set to YES to make undocumented symbols a build error:
  set(DOXYGEN_WARN_AS_ERROR NO)

  # ── Input ───────────────────────────────────────────────────────────────────
  doxygen_add_docs(
    doxygen "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/../README.md"
    COMMENT "Generating Doxygen HTML documentation → ${CMAKE_BINARY_DIR}/docs/html/index.html"
  )
endfunction()
