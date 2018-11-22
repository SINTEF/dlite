# -*- Mode: cmake -*-
#

find_package(PythonInterp)

add_definitions(-DDLITE_ROOT=${dlite_SOURCE_DIR})


add_custom_command(
  OUTPUT chemistry.h
  COMMAND ${dlite_BINARY_DIR}/tools/dlite-codegen
    --template ${dlite_SOURCE_DIR}/tools/templates/c-header.txt
    --uri ${CMAKE_CURRENT_SOURCE_DIR}/Chemistry-0.1.json
    --output chemistry.h
  MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/Chemistry-0.1.json
  DEPENDS ${dlite_SOURCE_DIR}/tools/templates/c-header.txt
  DEPENDS ${dlite_BINARY_DIR}/tools/dlite-codegen
  COMMENT "Generating C header for Chemistry-0.1.json"
)

add_executable(test_codegen
  test_codegen.c ${CMAKE_CURRENT_BINARY_DIR}/chemistry.h)
target_link_libraries(test_codegen dlite uuid)
target_include_directories(test_codegen PRIVATE
  ${dlite_SOURCE_DIR}/src
  ${dlite_BINARY_DIR}/src
  ${uuid_BINARY_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  )
add_dependencies(test_codegen dlite uuidProj)

add_test(
  NAME test-codegen
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test_codegen
)
set_tests_properties(test-codegen PROPERTIES
  DEPENDS test_codegen)
