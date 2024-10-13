# -*- Mode: cmake -*-
#

set(sources
  dlite-plugins-python.c
  )

#add_definitions(-DHAVE_CONFIG_H)

add_library(dlite-plugins-python SHARED ${sources})
target_link_libraries(dlite-plugins-python
  dlite
  dlite-utils
  )
target_include_directories(dlite-plugins-python PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}
  ${dlite-src_SOURCE_DIR}
  ${dlite-src_BINARY_DIR}
  ${Python3_INCLUDE_DIRS}
  )
set_target_properties(dlite-plugins-python PROPERTIES
  WINDOWS_EXPORT_ALL_SYMBOLS ON
  BUILD_RPATH "$ORIGIN/../../../"       # library is in ./dlite/share/dlite/storage-plugins, find libdlite-pyembed.so, libdlite.so, libdlite-utils.so in ./dlite
  INSTALL_RPATH "$ORIGIN/../../../"
  )


install(
  TARGETS dlite-plugins-python
  DESTINATION ${DLITE_STORAGE_PLUGIN_DIRS}
  #LIBRARY DESTINATION lib
)

install(
  DIRECTORY python-storage-plugins
  DESTINATION share/dlite
  PATTERN "*~" EXCLUDE
  PATTERN ".gitignore" EXCLUDE
  )

install(
  FILES
    python-storage-plugins/blob.json
    python-storage-plugins/Image.json
  DESTINATION share/dlite/storages
  )

# tests
# For now don't compile json tests if linking with static Python
# There is an issue with library order
if(NOT WITH_STATIC_PYTHON)
  add_subdirectory(tests-c)
endif()
add_subdirectory(tests-python)
