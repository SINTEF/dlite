# -*- Mode: cmake -*-
#

set(tests
  test_utils
  test_entity
  test_store
  test_triplestore
  test_collection
  test_schemas
  test_arrays
  )
if(WITH_JSON)
  list(APPEND tests
    test_json_storage
    test_json_utils
    test_json_entity
    )
endif()
if(WITH_HDF5)
  list(APPEND tests
    test_datamodel
    )
endif()


add_definitions(-DDLITE_ROOT=${dlite_SOURCE_DIR})

foreach(test ${tests})
  add_executable(${test} ${test}.c)
  target_link_libraries(${test}
    dlite-static
    dlite-utils-static
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
    ENVIRONMENT "LD_LIBRARY_PATH=${dlite_LD_LIBRARY_PATH}")
  set_property(TEST ${test} APPEND PROPERTY
    ENVIRONMENT "DLITE_STORAGE_PLUGINS=${dlite_STORAGE_PLUGINS}")

endforeach()

# Add extra include dir for json tests
if(WITH_JSON)
  foreach(test test_json_storage test_json_utils)
    target_include_directories(${test} PRIVATE
      ${dlite_SOURCE_DIR}/plugins/json
      )
  endforeach()
endif()
