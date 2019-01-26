# -*- Mode: cmake -*-
project(dlite-src C)

# Workaround for CMake not setting the correct binary path for MSVC
if(MSVC AND ${CMAKE_VERSION} VERSION_LESS "3.12.10")
  set(dlite-src_BINARY_DIR ${dlite-src_BINARY_DIR}/$<CONFIG>)
endif()


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
  dlite-misc.c
  dlite-store.c
  dlite-type.c
  dlite-arrays.c
  dlite-storage.c
  dlite-storage-plugins.c
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

#link_directories(${uuid_BINARY_DIR})

if(WITH_JSON)
  set(json_link_libraries dlite-plugins-json ${JSON_LIBRARIES})
endif()
if(WITH_HDF5)
  set(hdf5_link_libraries dlite-plugins-hdf5 ${HDF5_LIBRARIES})
endif()

add_library(dlite SHARED ${sources})
add_library(dlite-static STATIC ${sources})

target_link_libraries(dlite
  ${UUID_LIBRARIES}
  dlite-utils
  ${CMAKE_DL_LIBS}
  )
target_link_libraries(dlite-static
  ${UUID_STATIC_LIBRARIES}
  dlite-utils-static
  ${CMAKE_DL_LIBS}
  )

target_include_directories(dlite PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}
  ${UUID_INCLUDE_DIRS}
  )
target_include_directories(dlite-static PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}
  ${UUID_INCLUDE_DIRS}
  )

add_dependencies(dlite
  uuidProj
  #${HDF5_DEPENDENCIES}
  #${JANSSON_DEPENDENCIES}
  )
add_dependencies(dlite-static
  uuidProj
  #${HDF5_DEPENDENCIES}
  #${JANSSON_DEPENDENCIES}
  )

install(
  TARGETS dlite dlite-static
  DESTINATION lib
)

# create list of headers to install from ${sources}
set(headers "")
foreach(source ${sources})
  string(REPLACE ".c" ".h" header ${source})
  list(APPEND headers ${header})
endforeach()

install(FILES
  dlite.h
  ${headers}
  #${uuid_SOURCE_DIR}/uuid.h
  #${uuid_BINARY_DIR}/integers.h
  #${uuid_BINARY_DIR}/byteorder.h
  ${CMAKE_CURRENT_BINARY_DIR}/boolean.h
  ${CMAKE_CURRENT_BINARY_DIR}/floats.h
  DESTINATION include/dlite
)
# Tests
add_subdirectory(tests)
