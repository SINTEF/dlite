# -*- Mode: cmake -*-
#

#-----------------------------------------------------
# System tests
#-----------------------------------------------------

# Generate config.h
# -----------------
include(CheckSymbolExists)
check_symbol_exists(strdup       string.h   HAVE_STRDUP)
check_symbol_exists(_strdup      string.h   HAVE__STRDUP)
check_symbol_exists(strcasecmp   string.h   HAVE_STRCASECMP)
check_symbol_exists(_stricmp     string.h   HAVE__STRICMP)
check_symbol_exists(strncasecmp  string.h   HAVE_STRNCASECMP)
check_symbol_exists(_strnicmp    string.h   HAVE__STRNICMP)

include(CheckCSourceCompiles)

check_c_source_compiles("
__thread int tls;
int main(void) { return 0; }" HAVE_GCC_THREAD_LOCAL_STORAGE)


check_c_source_compiles("
__declspec(thread) int tls;
int main(void) { return 0; }" HAVE_WIN32_THREAD_LOCAL_STORAGE)

check_c_source_compiles("
#include <stdio.h>
#define f(msg, ...) printf(msg, __VA_ARGS__)
int main(void) {
  f(\"x=%g, y=%d\", 3.14, 42);
  return 0;
}" HAVE_VA_ARGS)


configure_file(config.h.in config.h)


#-----------------------------------------------------
# Compile library
#-----------------------------------------------------
set(sources
  compat.c
  err.c
  map.c
  strtob.c
  fileutils.c
  tmpfileplus.c
  globmatch.c
  )

add_definitions(
  -DHAVE_CONFIG_H
  )

add_library(utils SHARED ${sources})
#target_link_libraries(utils)
target_include_directories(utils PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}
  )
#add_dependencies(utils)

install(
  TARGETS utils
  DESTINATION lib
)

install(
  DIRECTORY .
  DESTINATION include/dlite/utils
  FILES_MATCHING PATTERN "*.h"
  PATTERN tests EXCLUDE
)

# Tests
add_subdirectory(tests)
