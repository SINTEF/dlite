# -*- Mode: cmake -*-

#
# Add test to build the individual examples
# -----------------------------------------

# ex1
add_test(
  NAME ex1-mkdir
  COMMAND ${CMAKE_COMMAND} -E make_directory ex1
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )
add_test(
  NAME ex1-config
  COMMAND ${CMAKE_COMMAND} ${CMAKE_CURRENT_SOURCE_DIR}/ex1
    -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/ex1
  )
add_test(
  NAME ex1-build
  COMMAND ${CMAKE_COMMAND} --build .
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/ex1
  )
add_test(
  NAME ex1-install
  COMMAND ${CMAKE_COMMAND} --build . --target install
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/ex1
  )
add_test(
  NAME ex1-test
  COMMAND ${CMAKE_COMMAND} --build . --target test
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/ex1
  )
set_property(TEST ex1-config  PROPERTY DEPENDS ex1-mkdir)
set_property(TEST ex1-build   PROPERTY DEPENDS ex1-config)
set_property(TEST ex1-install PROPERTY DEPENDS ex1-build)
set_property(TEST ex1-test    PROPERTY DEPENDS ex1-install)
