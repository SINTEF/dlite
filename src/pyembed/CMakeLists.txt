# -*- Mode: cmake -*-
project(dlite-src-pyembed C)

set(sources
  dlite-pyembed.c
  #dlite-python-storage.c
  dlite-python-mapping.c
  )

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
  ${dlite-src_SOURCE_DIR}
  ${dlite-src_BINARY_DIR}
  ${dlite-src-pyembed_SOURCE_DIR}
  #${dlite-src-utils_SOURCE_DIR}
  #${dlite-bindings-python_SOURCE_DIR}
  )

set_target_properties(dlite-pyembed PROPERTIES
  WINDOWS_EXPORT_ALL_SYMBOLS ON
  BUILD_RPATH ${dlite-bindings-python_BINARY_DIR}
  )
#set_target_properties(_dlite PROPERTIES
#  BUILD_RPATH ${dlite-bindings-python_BINARY_DIR}
#  )


install(
  TARGETS dlite-pyembed
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
)

install(DIRECTORY
  mappings
  storages
  DESTINATION share/dlite
  #FILES_MATCHING *.py
  )


add_subdirectory(tests)
