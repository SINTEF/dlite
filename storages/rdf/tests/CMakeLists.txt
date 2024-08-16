# -*- Mode: cmake -*-
#

set(tests
  test_rdf
)

add_definitions(
  -Ddlite_SOURCE_DIR=${dlite_SOURCE_DIR}
  -Ddlite_BINARY_DIR=${dlite_BINARY_DIR}
  -DDLITE_BINARY_ROOT=${dlite_BINARY_DIR}
)

foreach(test ${tests})
  add_executable(${test} ${test}.c)
  target_link_libraries(${test}
    dlite
    dlite-utils
    ${extra_link_libraries}
  )
  target_include_directories(${test} PRIVATE
    ${dlite_SOURCE_DIR}/src
    ${dlite_SOURCE_DIR}/src/tests
    ${dlite_BINARY_DIR}/src
  )

  add_test(
    NAME ${test}
    COMMAND ${test}
  )

  set_property(TEST ${test} PROPERTY
    ENVIRONMENT "PATH=${dlite_PATH_NATIVE}")
  set_property(TEST ${test} APPEND PROPERTY
    ENVIRONMENT "PYTHONPATH=${dlite_PYTHONPATH_NATIVE}")
  set_property(TEST ${test} APPEND PROPERTY
    ENVIRONMENT "LD_LIBRARY_PATH=${dlite_LD_LIBRARY_PATH_NATIVE}")
  set_property(TEST ${test} APPEND PROPERTY
    ENVIRONMENT "DLITE_USE_BUILD_ROOT=YES")

endforeach()
