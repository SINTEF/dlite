# -*- Mode: cmake -*-
#

set(tests
  test_python_bindings
  test_collection
  test_entity
  test_factory
  test_misc
  test_python_storage
  test_storage
  )

foreach(test ${tests})
  set(name ${test}-py)
  add_test(
    NAME ${name}
    COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/${test}.py
    )

  set_property(TEST ${name} PROPERTY
    ENVIRONMENT "PATH=${dlite_PATH}")
  set_property(TEST ${name} APPEND PROPERTY
    ENVIRONMENT "PYTHONPATH=${dlite_PYTHONPATH}")
  set_property(TEST ${name} APPEND PROPERTY
    ENVIRONMENT "LD_LIBRARY_PATH=${dlite_LD_LIBRARY_PATH}")
  set_property(TEST ${name} APPEND PROPERTY
    ENVIRONMENT "DLITE_STORAGE_PLUGIN_DIRS=${dlite_STORAGE_PLUGINS}")
  set_property(TEST ${name} APPEND PROPERTY
    ENVIRONMENT "DLITE_MAPPING_PLUGIN_DIRS=${dlite_MAPPING_PLUGINS}")
  set_property(TEST ${name} APPEND PROPERTY ENVIRONMENT
    "DLITE_PYTHON_STORAGE_PLUGIN_DIRS=${dlite_PYTHON_STORAGE_PLUGINS}")
  set_property(TEST ${name} APPEND PROPERTY ENVIRONMENT
     "DLITE_PYTHON_MAPPING_PLUGIN_DIRS=${dlite_PYTHON_MAPPING_PLUGINS}")
  set_property(TEST ${name} APPEND PROPERTY
    ENVIRONMENT "DLITE_TEMPLATE_DIRS=${dlite_TEMPLATES}")
  set_property(TEST ${name} APPEND PROPERTY
    ENVIRONMENT "DLITE_STORAGES=${dlite_STORAGES}")

endforeach()
