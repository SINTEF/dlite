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
    ENVIRONMENT "PATH=${dlite_PATH_NATIVE}")
  set_property(TEST ${test} APPEND PROPERTY
    ENVIRONMENT "WINEPATH=${dlite_WINEPATH_NATIVE}")
  set_property(TEST ${test} APPEND PROPERTY
    ENVIRONMENT "PYTHONPATH=${dlite_PYTHONPATH_NATIVE}")
  set_property(TEST ${test} APPEND PROPERTY
    ENVIRONMENT "LD_LIBRARY_PATH=${dlite_LD_LIBRARY_PATH_NATIVE}")
  set_property(TEST ${test} APPEND PROPERTY
    ENVIRONMENT "DLITE_USE_BUILD_ROOT=YES")

endforeach()
