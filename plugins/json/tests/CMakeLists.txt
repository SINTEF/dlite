# -*- Mode: cmake -*-
#

set(tests
  test_json_utils
  test_json_storage
  )

add_definitions(-DDLITE_ROOT=${dlite_SOURCE_DIR})

foreach(test ${tests})
  add_executable(${test} ${test}.c)
  target_link_libraries(${test} dlite-plugins-json)
  target_include_directories(${test} PRIVATE
    ${dlite_SOURCE_DIR}/plugins/json
    ${dlite-src_SOURCE_DIR}/tests
    )

  add_test(
    NAME ${test}
    COMMAND ${test}
    )

  set(path ${dlite_BINARY_DIR}/plugins/json ${dlite_PATH})
  if(UNIX)
    string(REPLACE ";" ":" path ${path})
  endif()

  set_property(TEST ${test} PROPERTY
    ENVIRONMENT "PATH=${dlite_PATH}")
  set_property(TEST ${test} APPEND PROPERTY
    ENVIRONMENT "LD_LIBRARY_PATH=${dlite_LD_LIBRARY_PATH}")
  set_property(TEST ${test} APPEND PROPERTY
    ENVIRONMENT "DLITE_STORAGE_PLUGINS=${dlite_STORAGE_PLUGINS}")
  set_property(TEST ${test} APPEND PROPERTY
    ENVIRONMENT "DLITE_TRANSLATOR_PLUGINS=${dlite_TRANSLATOR_PLUGINS}")



endforeach()
