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

link_directories(
    ${dlite_BINARY_DIR}/src/utils
    )

foreach(test ${tests})
  add_executable(${test} ${test}.c)
  target_link_libraries(${test} dlite uuid dlite-utils)
  target_include_directories(${test} PRIVATE
    ${dlite_SOURCE_DIR}/src
    ${dlite_BINARY_DIR}/src
    ${uuid_BINARY_DIR}
    )
  add_dependencies(${test} dlite)

  add_test(
    NAME ${test}
    COMMAND ${test}
    )
endforeach()

# Add extra include dir for json tests
if(WITH_JSON)
  foreach(test test_json_storage test_json_utils)
    target_include_directories(${test} PRIVATE
      ${dlite_SOURCE_DIR}/plugins/json
      )
  endforeach()
endif()
