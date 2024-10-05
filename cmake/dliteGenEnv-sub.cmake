# Sub-cmake file for running configure_file() during build time

# Append our cmake-modules to CMAKE_MODULE_PATH
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${dlite_SOURCE_DIR}/cmake)


# Convert from pipe-separated paths to lists
set(dlite_BUILD_ROOT ${dlite_BINARY_DIR})
string(REPLACE "|" ";" dlite_PATH                    "${dlite_PATH}")
string(REPLACE "|" ";" dlite_LD_LIBRARY_PATH         "${dlite_LD_LIBRARY_PATH}")
string(REPLACE "|" ";" dlite_PYTHONPATH              "${dlite_PYTHONPATH}")
string(REPLACE "|" ";" dlite_STORAGE_PLUGINS         "${dlite_STORAGE_PLUGINS}")
string(REPLACE "|" ";" dlite_MAPPING_PLUGINS         "${dlite_MAPPING_PLUGINS}")
string(REPLACE "|" ";" dlite_PYTHON_STORAGE_PLUGINS  "${dlite_PYTHON_STORAGE_PLUGINS}")
string(REPLACE "|" ";" dlite_PYTHON_MAPPING_PLUGINS  "${dlite_PYTHON_MAPPING_PLUGINS}")
string(REPLACE "|" ";" dlite_PYTHON_PROTOCOL_PLUGINS "${dlite_PYTHON_PROTOCOL_PLUGINS}")
string(REPLACE "|" ";" dlite_TEMPLATES               "${dlite_TEMPLATES}")
string(REPLACE "|" ";" dlite_STORAGES                "${dlite_STORAGES}")


# Add platform-specific paths
include(MakePlatformPaths)
make_platform_paths(
  PATHS
    dlite_BUILD_ROOT
    dlite_BINARY_DIR
    dlite_SOURCE_DIR
    dlite_LD_LIBRARY_PATH
    dlite_PYTHONPATH
    dlite_PYTHON_STORAGE_PLUGINS
    dlite_PYTHON_MAPPING_PLUGINS
    dlite_PYTHON_PROTOCOL_PLUGINS
    dlite_TEMPLATES
  MULTI_CONFIG_PATHS
    dlite_PATH
    dlite_STORAGE_PLUGINS
    dlite_MAPPING_PLUGINS
  ESCAPE_WIN32_DIRSEP
  )

# Make dlite_STORAGES pipe-separated
string(REPLACE ";" "|" dlite_STORAGES         "${dlite_STORAGES}")
string(REPLACE ";" "|" dlite_STORAGES_NATIVE  "${dlite_STORAGES_NATIVE}")
string(REPLACE ";" "|" dlite_STORAGES_UNIX    "${dlite_STORAGES_UNIX}")
string(REPLACE ";" "|" dlite_STORAGES_WINDOWS "${dlite_STORAGES_WINDOWS}")

configure_file(${input} ${output}
  @ONLY
  NEWLINE_STYLE ${newline_style}
  )
