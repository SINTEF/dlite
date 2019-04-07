# -*- Mode: cmake -*-
project(dlite-src-pyembed C)

set(sources
  dlite-pyembed.c
  dlite-python-mapping.c
  #dlite-pyembed-driver.c
  )

find_package(PythonInterp 3 REQUIRED)
find_package(PythonLibs REQUIRED)

add_definitions(
  -DHAVE_CONFIG_H
  )

add_library(dlite-pyembed SHARED ${sources})

target_link_libraries(dlite-pyembed
  dlite
  dlite-utils
  ${PYTHON_LIBRARIES}
  )

target_include_directories(dlite-pyembed PUBLIC
  ${PYTHON_INCLUDE_DIRS}
  ${dlite-src_SOURCE_DIR}
  ${dlite-src_BINARY_DIR}
  ${dlite-src-pyembed_SOURCE_DIR}
  #${dlite-src-utils_SOURCE_DIR}
  )

set_target_properties(dlite-pyembed PROPERTIES
  WINDOWS_EXPORT_ALL_SYMBOLS ON
  )

install(
  TARGETS dlite-pyembed
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
)

install(DIRECTORY
  mappings
  #drivers
  DESTINATION share/dlite
  #FILES_MATCHING *.py
  )


add_subdirectory(tests)
