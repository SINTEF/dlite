# -*- Mode: cmake -*-

#
# Macro for testing examples
# --------------------------
# It adds a set for tests for each example that essentially runs the
# following set of commands (here shown using unix makefiles):
#
#      mkdir build                            (ex-mkdir)
#      cd build
#      cmake <conf> <source_dir>              (ex-config)
#      make                                   (ex-build)
#      make test                              (ex-test)
#
# Here <conf> is the configurations that dlite is build with.
macro(test_example example)
  set(conf
    -G ${CMAKE_GENERATOR}
    -DDLITE_ROOT=${CMAKE_INSTALL_PREFIX}
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
    $<$<BOOL:${CMAKE_TOOLCHAIN_FILE}>:${CMAKE_TOOLCHAIN_FILE}>
    )
  add_test(
    NAME ${example}-mkdir
    COMMAND ${CMAKE_COMMAND} -E make_directory ${example}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
  add_test(
    NAME ${example}-config
    COMMAND ${CMAKE_COMMAND} ${conf} ${CMAKE_CURRENT_SOURCE_DIR}/${example}
      -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
      -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${example}
    )
  add_test(
    NAME ${example}-build
    COMMAND ${CMAKE_COMMAND} --build .
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${example}
    )
  add_test(
    NAME ${example}-test
    COMMAND ${CMAKE_COMMAND} --build . --target test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${example}
    )
  set_property(TEST ${example}-config  PROPERTY DEPENDS ${example}-mkdir)
  set_property(TEST ${example}-build   PROPERTY DEPENDS ${example}-config)
  set_property(TEST ${example}-test    PROPERTY DEPENDS ${example}-build)
endmacro()


# Only run the examples if dlite has been installed
#
# This is not perfect, since the if-statement below is evaluated at
# configure time.  However, in practice this should be fine...
if(EXISTS ${CMAKE_INSTALL_PREFIX}/share/dlite/examples)
  test_example(ex1)
  test_example(ex2)
  test_example(ex3)
  test_example(ex4)
  #test_example(ex5d)
endif()


install(
  DIRECTORY ex1 ex2 ex3 ex4 ex5d
  DESTINATION share/dlite/examples
  )
