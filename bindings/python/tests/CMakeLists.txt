# -*- Mode: cmake -*-
#

set(tests
  test_python_bindings
  test_collection
  test_collection_load
  test_entity
  test_factory
  test_misc
  test_python_storage
  test_storage
  test_storage_plugins
  test_paths
  test_utils
  test_global_dlite_state
  test_property_mappings
  test_datamodel
  test_transaction
  test_mapping
  test_rdf
  test_postgresql1_write
  test_postgresql2_read
  test_ref_type
  test_pydantic
  test_relation
  test_iri
  test_dataset1_save
  test_dataset2_load
  test_protocol
  test_isolated_plugins
  test_options
  test_plugin
)

foreach(test ${tests})
  set(name ${test}-py)
  add_test(
    NAME ${name}
    COMMAND ${RUNNER} ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/${test}.py
    )

  set_property(TEST ${name} PROPERTY
    ENVIRONMENT "PATH=${dlite_PATH_NATIVE}")
  set_property(TEST ${name} APPEND PROPERTY
    ENVIRONMENT "PYTHONPATH=${dlite_PYTHONPATH_NATIVE}")
  if (NOT WIN32)
  set_property(TEST ${name} APPEND PROPERTY
    ENVIRONMENT "LD_LIBRARY_PATH=${dlite_LD_LIBRARY_PATH_NATIVE}")
  endif()
  set_property(TEST ${name} APPEND PROPERTY
    ENVIRONMENT "DLITE_USE_BUILD_ROOT=YES")

  # Skip tests that exit with return code 44
  set_property(TEST ${name} PROPERTY SKIP_RETURN_CODE 44)

endforeach()

# Additional properties
set_property(TEST test_postgresql2_read-py PROPERTY
  DEPENDS test_postgresql1_write-py)
set_property(TEST test_collection_load-py APPEND PROPERTY
  DEPENDS test_collection-py)
