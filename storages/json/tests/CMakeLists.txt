# -*- Mode: cmake -*-
#

set(tests
  test_json_utils
  test_json_storage
  )

add_definitions(-DDLITE_ROOT=${dlite_SOURCE_DIR})

# We are linking to dlite-plugins-json DLL - this require that this
# DLL is in the PATH on Windows. Copying the DLL to the current
# BINARY_DIR is a simple way to ensure this.
add_custom_target(
  copy-dlite-plugins-json
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:dlite-plugins-json>
    ${dlite_BINARY_DIR}/storages/json/tests
  )

foreach(test ${tests})
  add_executable(${test} ${test}.c)
  target_link_libraries(${test} dlite-plugins-json)
  target_include_directories(${test} PRIVATE
    ${dlite_SOURCE_DIR}/storages/json
    ${dlite-src_SOURCE_DIR}/tests
    )
  add_dependencies(${test} copy-dlite-plugins-json)

  add_test(
    NAME ${test}
    COMMAND ${RUNNER} ${test}
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
