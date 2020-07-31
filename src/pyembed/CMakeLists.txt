# -*- Mode: cmake -*-
project(dlite-src-pyembed C)

set(sources
  dlite-pyembed.c
  dlite-python-storage.c
  dlite-python-mapping.c
  )

add_definitions(
  -DHAVE_CONFIG_H
  )

add_library(dlite-pyembed SHARED ${sources})
add_library(dlite-pyembed-static STATIC ${sources})

target_link_libraries(dlite-pyembed
  dlite
  dlite-utils
  ${PYTHON_LIBRARIES}
  )
target_link_libraries(dlite-pyembed-static
  dlite-static
  dlite-utils-static
  ${PYTHON_LIBRARIES}
  )

target_include_directories(dlite-pyembed PUBLIC
  ${PYTHON_INCLUDE_DIRS}
  $<BUILD_INTERFACE:${dlite-src_SOURCE_DIR}>
  $<BUILD_INTERFACE:${dlite-src_BINARY_DIR}>
  $<BUILD_INTERFACE:${dlite-src-pyembed_SOURCE_DIR}>
  #$<BUILD_INTERFACE:${dlite-src-utils_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include/dlite>
  $<INSTALL_INTERFACE:include/dlite/utils>
  $<INSTALL_INTERFACE:include/dlite/pyembed>
  )
target_include_directories(dlite-pyembed-static PUBLIC
  ${PYTHON_INCLUDE_DIRS}
  $<BUILD_INTERFACE:${dlite-src_SOURCE_DIR}>
  $<BUILD_INTERFACE:${dlite-src_BINARY_DIR}>
  $<BUILD_INTERFACE:${dlite-src-pyembed_SOURCE_DIR}>
  #$<BUILD_INTERFACE:${dlite-src-utils_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include/dlite>
  $<INSTALL_INTERFACE:include/dlite/utils>
  $<INSTALL_INTERFACE:include/dlite/pyembed>
  )

set_target_properties(dlite-pyembed PROPERTIES
  WINDOWS_EXPORT_ALL_SYMBOLS ON
  BUILD_RPATH "${dlite-bindings-python_BINARY_DIR}"
  PUBLIC_HEADER "dlite-pyembed.h;dlite-python-mapping.h"
  )
set_target_properties(dlite-pyembed-static PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  BUILD_RPATH "${dlite-bindings-python_BINARY_DIR}"
  PUBLIC_HEADER "dlite-pyembed.h;dlite-python-mapping.h"
  )

#set_target_properties(_dlite PROPERTIES
#  BUILD_RPATH ${dlite-bindings-python_BINARY_DIR}
#  )

install(
  TARGETS dlite-pyembed dlite-pyembed-static
  EXPORT  dliteTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  PUBLIC_HEADER DESTINATION include/dlite/pyembed
  INCLUDES DESTINATION include/dlite/pyembed

)

install(
  DIRECTORY python-mapping-plugins
  DESTINATION share/dlite
  PATTERN "*~" EXCLUDE
  )


add_subdirectory(tests)
