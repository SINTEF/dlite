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


# Generate config.h
# -----------------
include(CheckFunctionExists)
check_function_exists(strdup HAVE_STRDUP)
check_function_exists(_strdup _HAVE_STRDUP)

include(CheckCSourceCompiles)

check_c_source_compiles("
__thread int tls;
int main(void) { return 0; }" HAVE_GCC_THREAD_LOCAL_STORAGE)


check_c_source_compiles("
__declspec(thread) int tls;
int main(void) { return 0; }" HAVE_WIN32_THREAD_LOCAL_STORAGE)

configure_file(config.h.in config.h)


#-----------------------------------------------------
# Compile library
#-----------------------------------------------------
set(sources
  dlite.c
  dh5lite.c
  getuuid.c
  err.c
  compat.c
  )

add_definitions(-DHAVE_CONFIG_H)

link_directories(${hdf5_LIBRARY_DIRS} ${uuid_BINARY_DIR})

if(WITH_LIB)
  add_library(dlite SHARED ${sources})
  target_link_libraries(dlite uuid ${hdf5_LIBRARIES})
  target_include_directories(dlite PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${uuid_SOURCE_DIR}
    ${uuid_BINARY_DIR}
    ${hdf5_INCLUDE_DIRS}
    )
  add_dependencies(dlite uuidProj ${hdf5_NAME})

  install(
    TARGETS dlite
    DESTINATION lib)
endif()


if(WITH_STATIC_LIB)
  add_library(dlite-static STATIC ${sources})
  target_link_libraries(dlite-static uuid ${hdf5_LIBRARIES})
  target_include_directories(dlite-static PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${uuid_SOURCE_DIR}
    ${uuid_BINARY_DIR}
    ${hdf5_INCLUDE_DIRS}
    )
  add_dependencies(dlite-static uuidProj ${hdf5_NAME})

  install(
    TARGETS dlite-static
    DESTINATION lib)
endif()


# dgetuuid
add_executable(dgetuuid dgetuuid.c)
target_link_libraries(dgetuuid dlite uuid)
target_include_directories(dgetuuid PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}
  ${uuid_SOURCE_DIR}
  ${uuid_BINARY_DIR}
  )
install(
  TARGETS dgetuuid
  DESTINATION bin)


# Tests
add_subdirectory(tests)
