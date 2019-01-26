# -*- Mode: cmake -*-
#

set(tests
  test_err
  test_tmpfileplus
  test_fileutils
  test_globmatch
  test_dsl
  test_plugin
  test_tgen

  tgen_example
  )

# Workaround for old CMake adding CONFIG to binary path for MSVC
if(MSVC AND ${CMAKE_VERSION} VERSION_LESS "3.12.10")
  set(LIBDIR ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>)
else()
  set(LIBDIR ${CMAKE_CURRENT_BINARY_DIR})
endif()

add_definitions(
  -DTESTDIR=${CMAKE_CURRENT_SOURCE_DIR}
  -DLIBDIR=${LIBDIR}
  )

link_directories(
  ${dlite_BINARY_DIR}/src/utils
  )


foreach(test ${tests})
  add_executable(${test} ${test}.c)
  target_link_libraries(${test} dlite-utils-static)
  target_include_directories(${test} PRIVATE
    ${dlite_SOURCE_DIR}/src/tests
    ${dlite-src-utils_SOURCE_DIR}
    ${dlite-src-utils_BINARY_DIR}
    )
  add_dependencies(${test} dlite-utils-static)

  add_test(
    NAME ${test}
    COMMAND ${test}
    )
  #set_tests_properties(${test} PROPERTIES
  #  ENVIRONMENT "PATH=$<TARGET_FILE_DIR:dlite-utils>;$ENV{PATH}"
  #  )
endforeach()


# Create shared library for test_dsl
#
# We use a trick from Bjørn Tore and Olav.  Instead of poluting the C
# header with windows export declarations, we put them in a separate
# .def file which we, for Windows builds, simply pass to add_library()
# togheter with the sources.
if(WIN32)
  set(defs test_dsl_lib.def)
endif()
add_library(test_dsl_lib SHARED test_dsl_lib.c ${defs})
target_include_directories(test_dsl_lib PRIVATE
    ${dlite-src-utils_SOURCE_DIR}
    ${dlite-src-utils_BINARY_DIR}
  )
if(UNIX)
  target_link_libraries(test_dsl -ldl)
endif()


add_library(test_plugin_lib SHARED test_plugin_lib.c)
target_include_directories(test_plugin_lib PRIVATE
    ${dlite-src-utils_SOURCE_DIR}
    ${dlite-src-utils_BINARY_DIR}
  )
if(UNIX)
  target_link_libraries(test_plugin -ldl)
endif()
