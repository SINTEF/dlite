# -*- Mode: cmake -*-
#

#-----------------------------------------------------
# System tests
#-----------------------------------------------------

# Generate boolean.h
# ------------------
include(CheckIncludeFile)
check_include_file(stdbool.h HAVE_STDBOOL_H)

include(CheckTypeSize)
check_type_size(_Bool _BOOL)

configure_file(boolean.h.in boolean.h)


# Generate floats.h
# ------------------
check_type_size(float         SIZEOF_FLOAT)
check_type_size(double        SIZEOF_DOUBLE)
check_type_size("long double" SIZEOF_LONG_DOUBLE)

configure_file(floats.h.in floats.h)


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

add_subdirectory(utils)

set(sources
  dlite-utils.c
  dlite-store.c
  dlite-type.c
  dlite-storage.c
  dlite-datamodel.c
  dlite-schemas.c
  dlite-entity.c
  dlite-collection.c
  dlite-codegen.c
  getuuid.c
  triplestore.c
  )

add_definitions(
  -DHAVE_CONFIG_H
  #-DWITH_DEBUG
  )

link_directories(${uuid_BINARY_DIR})

add_library(dlite ${sources})
target_link_libraries(dlite
  plugins-hdf5
  plugins-json
  uuid
  ${HDF5_LIBRARIES}
  ${JANSSON_LIBRARIES}
  utils
  )
target_include_directories(dlite PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/utils
  ${uuid_SOURCE_DIR}
  ${uuid_BINARY_DIR}
  ${HDF5_INCLUDE_DIRS}
  ${JANSSON_INCLUDE_DIRS}
  )
add_dependencies(dlite
  uuidProj
  ${HDF5_DEPENDENCIES}
  ${JANSSON_DEPENDENCIES}
  )

install(
  TARGETS dlite
  DESTINATION lib
)

install(
  DIRECTORY .
  DESTINATION include/dlite
  FILES_MATCHING PATTERN "dlite-*.h"
  PATTERN tests EXCLUDE
  PATTERN dlite-macros.h EXCLUDE
)

install(FILES
  ${uuid_BINARY_DIR}/integers.h
  ${uuid_BINARY_DIR}/byteorder.h
  ${CMAKE_CURRENT_BINARY_DIR}/boolean.h
  ${CMAKE_CURRENT_BINARY_DIR}/floats.h
  DESTINATION include/dlite
)
# Tests
add_subdirectory(tests)
