# -*- Mode: cmake -*-
#

set(tests
  test_compat
  test_snprintf
  test_err
  test_map
  test_clp2
  test_unicode
  test_strutils
  test_urlsplit
  test_tmpfileplus
  test_strtob
  test_fileinfo
  test_fileutils
  test_execprocess
  test_globmatch
  test_dsl
  test_plugin
  test_tgen
  test_sha3
  #test_sha3_slow
  test_uuid
  test_uuid2
  test_uuid4
  test_infixcalc
  test_jsmnx
  test_jstore
  test_bson
  test_session
  test_rng
  test_uri_encode

  tgen_example
  )

link_directories(
  ${dlite_BINARY_DIR}/src/utils
  )


foreach(test ${tests})
  add_executable(${test} ${test}.c)
  target_link_libraries(${test} dlite-utils-static)
  #target_link_libraries(${test} dlite-utils)
  target_include_directories(${test} PRIVATE
    ${dlite_SOURCE_DIR}/src/tests
    ${dlite-src-utils_SOURCE_DIR}
    ${dlite-src-utils_BINARY_DIR}
    )
  target_compile_definitions(${test} PUBLIC
     -DTESTDIR=${CMAKE_CURRENT_SOURCE_DIR}
     -DBINDIR=$<TARGET_FILE_DIR:${test}>
     -DBINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
   )
  add_dependencies(${test} dlite-utils-static)
  #add_dependencies(${test} dlite-utils)

  add_test(
    NAME ${test}
    COMMAND ${RUNNER} ${test}
    )
  set_property(TEST ${test} PROPERTY
    ENVIRONMENT "PATH=${dlite_PATH_NATIVE}")
  if (NOT WIN32)
  set_property(TEST ${test} APPEND PROPERTY
    ENVIRONMENT "LD_LIBRARY_PATH=${dlite_LD_LIBRARY_PATH_NATIVE}")
  endif()

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
target_link_libraries(test_dsl ${CMAKE_DL_LIBS})


add_library(test_plugin_lib SHARED test_plugin_lib.c)
target_include_directories(test_plugin_lib PRIVATE
  ${dlite-src-utils_SOURCE_DIR}
  ${dlite-src-utils_BINARY_DIR}
  )
target_link_libraries(test_plugin ${CMAKE_DL_LIBS})
