# -*- Mode: cmake -*-
#

set(tests
  test_pyembed
  test_python_mapping
  )


add_definitions(
  -DTESTDIR=${CMAKE_CURRENT_SOURCE_DIR}
  -DDLITE_ROOT=${dlite_SOURCE_DIR}
  -DDLITE_BINARY_ROOT=${dlite_BINARY_DIR}
  )

foreach(test ${tests})
  add_executable(${test} ${test}.c)
  target_link_libraries(${test}
    dlite
    dlite-utils
    dlite-pyembed
    )
  target_include_directories(${test} PRIVATE
    ${dlite-src_SOURCE_DIR}
    ${dlite-src_BINARY_DIR}
    ${dlite-src_SOURCE_DIR}/tests
    )

  add_test(
    NAME ${test}
    COMMAND ${test}
    )

  set_property(TEST ${test} PROPERTY
    ENVIRONMENT "PATH=${dlite_PATH}")
  set_property(TEST ${test} APPEND PROPERTY
    ENVIRONMENT "PYTHONPATH=${dlite_PYTHONPATH}")
  set_property(TEST ${test} APPEND PROPERTY
    ENVIRONMENT "LD_LIBRARY_PATH=${dlite_LD_LIBRARY_PATH}")
  set_property(TEST ${test} APPEND PROPERTY
    ENVIRONMENT "DLITE_STORAGE_PLUGIN_DIRS=${dlite_STORAGE_PLUGINS}")
  set_property(TEST ${test} APPEND PROPERTY
    ENVIRONMENT "DLITE_MAPPING_PLUGIN_DIRS=${dlite_MAPPING_PLUGINS}")
  set_property(TEST ${test} APPEND PROPERTY ENVIRONMENT
    "DLITE_PYTHON_STORAGE_PLUGIN_DIRS=${dlite_PYTHON_STORAGE_PLUGINS}")
  set_property(TEST ${test} APPEND PROPERTY ENVIRONMENT
     "DLITE_PYTHON_MAPPING_PLUGIN_DIRS=${dlite_PYTHON_MAPPING_PLUGINS}")
  set_property(TEST ${test} APPEND PROPERTY
    ENVIRONMENT "DLITE_TEMPLATE_DIRS=${dlite_TEMPLATES}")
  set_property(TEST ${test} APPEND PROPERTY
    ENVIRONMENT "DLITE_STORAGES=${dlite_STORAGES}")

endforeach()
