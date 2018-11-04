# -*- Mode: cmake -*-
#

set(tests
  test_dsl
  test_fileutils
  )

add_definitions(
  -DTESTDIR=${CMAKE_CURRENT_SOURCE_DIR}
  -DLIBDIR=${CMAKE_CURRENT_BINARY_DIR}
  )

link_directories(
  ${dlite_BINARY_DIR}/src/utils
  )


foreach(test ${tests})
  add_executable(${test} ${test}.c)
  target_link_libraries(${test} utils)
  target_include_directories(${test} PRIVATE
    ${dlite_SOURCE_DIR}/src/utils
    ${dlite_SOURCE_DIR}/src/tests
    )
  add_dependencies(${test} utils)

  add_test(
    NAME ${test}
    COMMAND ${test}
    )
endforeach()


# create shared library for test_dsl
add_library(test_dsl_lib SHARED test_dsl_lib.c)
if(UNIX)
  target_link_libraries(test_dsl -ldl)
endif()
