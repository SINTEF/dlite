# -*- Mode: cmake -*-
project(dlite-src-pyembed C)

set(sources
  dlite-pyembed.c
  dlite-python-storage.c
  dlite-python-mapping.c
  )

#find_package(PythonInterp 3 REQUIRED)
#find_package(PythonLibs REQUIRED)

add_definitions(
  -DHAVE_CONFIG_H
  )

add_library(dlite-pyembed SHARED ${sources})

target_link_libraries(dlite-pyembed
  dlite
  dlite-utils
  #$<TARGET_FILE:_dlite>
  #_dlite
  #_dlite-static
  ${PYTHON_LIBRARIES}
  #$<TARGET_FILE:${SWIG_MODULE_dlite_REAL_NAME}>
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

set_target_properties(dlite-pyembed PROPERTIES
  WINDOWS_EXPORT_ALL_SYMBOLS ON
  BUILD_RPATH ${dlite-bindings-python_BINARY_DIR}
  PUBLIC_HEADER "dlite-pyembed.h;dlite-python-mapping.h"
  )
#set_target_properties(_dlite PROPERTIES
#  BUILD_RPATH ${dlite-bindings-python_BINARY_DIR}
#  )

install(
  TARGETS dlite-pyembed
  EXPORT  dliteTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  PUBLIC_HEADER DESTINATION include/dlite/pyembed
  INCLUDES DESTINATION include/dlite/pyembed

)

install(
  DIRECTORY python-mappings
  DESTINATION share/dlite
  #FILES_MATCHING *.py
  )


add_subdirectory(tests)
