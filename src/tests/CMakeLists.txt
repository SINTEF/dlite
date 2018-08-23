# -*- Mode: cmake -*-
#

include_directories(
  ${dlite_BINARY_DIR}/src
  ${uuid_BINARY_DIR}
  )

set(tests
  test_dlite
  test_datamodel
  test_entity
  test_json_storage
  test_json_utils
  )

foreach(test ${tests})
  add_executable(${test} ${test}.c)
  target_link_libraries(${test} dlite plugins-hdf5 plugins-json uuid)
  target_include_directories(${test} PRIVATE
    ${dlite_SOURCE_DIR}/src
    ${dlite_SOURCE_DIR}/plugins/json  # FIXME - remove this dependency
    ${JANSSON_INCLUDE_DIRS}
    )
  add_dependencies(${test} dlite)

  add_test(
    NAME ${test}
    COMMAND ${test}
    )
endforeach()
