# -*- Mode: cmake -*-
#

set(tests
  test_misc
  test_type
  test_storage
  test_entity
  test_store
  test_triplestore
  test_collection
  test_schemas
  test_arrays
  )
if(WITH_JSON)
  list(APPEND tests test_json_entity)
  list(APPEND tests test_storage_lookup)
  list(APPEND tests test_mapping)
endif()
if(WITH_HDF5)
  list(APPEND tests test_datamodel)
endif()
if(WITH_POSTGRESQL)
  list(APPEND tests test_postgresql_storage)
endif()


add_definitions(
  -Ddlite_SOURCE_DIR=${dlite_SOURCE_DIR}
  -Ddlite_BINARY_DIR=${dlite_BINARY_DIR}
  -DDLITE_BINARY_ROOT=${dlite_BINARY_DIR}
  )

add_subdirectory(mappings)

foreach(test ${tests})
  add_executable(${test} ${test}.c)
  target_link_libraries(${test}
    #dlite-static
    #dlite-utils-static
    dlite
    dlite-utils
    )
  target_include_directories(${test} PRIVATE
    ${dlite_SOURCE_DIR}/src
    ${dlite_BINARY_DIR}/src
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
