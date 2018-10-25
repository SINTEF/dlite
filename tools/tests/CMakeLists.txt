# -*- Mode: cmake -*-
#

find_package(PythonInterp)

add_definitions(-DDLITE_ROOT=${dlite_SOURCE_DIR})


add_custom_command(
  OUTPUT chemistry.h
  COMMAND ${dlite_BINARY_DIR}/tools/dlite-codegen -o chemistry.h -u ${CMAKE_CURRENT_SOURCE_DIR}/Chemistry-0.1.json
  MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/Chemistry-0.1.json
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


#add_custom_command(
#  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/chemistry.h
#  COMMAND ${PYTHON_EXECUTABLE} ${dlite_SOURCE_DIR}/tools/codegen.py --destination ${CMAKE_CURRENT_BINARY_DIR} -H chemistry.h ${CMAKE_CURRENT_SOURCE_DIR}/Chemistry-0.1.json
#  MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/Chemistry-0.1.json
#  DEPENDS ${dlite_SOURCE_DIR}/tools/codegen.py
#          ${dlite_SOURCE_DIR}/tools/basegen.py
#          ${dlite_SOURCE_DIR}/tools/cgen.py
#  COMMENT "Generating C header for Chemistry-0.1.json"
#)
#
#add_executable(test_codegen test_codegen.c)
#target_link_libraries(test_codegen dlite uuid)
#target_include_directories(test_codegen PRIVATE
#  ${dlite_SOURCE_DIR}/src
#  ${dlite_BINARY_DIR}/src
#  ${uuid_BINARY_DIR}
#  ${CMAKE_CURRENT_BINARY_DIR}
#  )
#add_dependencies(test_codegen dlite uuidProj)
#
#add_test(
#  NAME test-codegen
#  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test_codegen
#)
#set_tests_properties(test-codegen PROPERTIES
#  DEPENDS test_codegen)
#

# Old generator
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/chemistry_old.c
         ${CMAKE_CURRENT_BINARY_DIR}/chemistry_old.h
  COMMAND ${PYTHON_EXECUTABLE} ${dlite_SOURCE_DIR}/tools/codegen_c_old.py --destination ${CMAKE_CURRENT_BINARY_DIR} -c chemistry_old.c -H chemistry_old.h ${CMAKE_CURRENT_SOURCE_DIR}/Chemistry-0.1.json
  MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/Chemistry-0.1.json
  DEPENDS ${dlite_SOURCE_DIR}/tools/codegen_old.py
          ${dlite_SOURCE_DIR}/tools/codegen_c_old.py
  COMMENT "Generating C source and header for Chemistry-0.1.json (old generator)"
)

add_executable(test_codegen_old
  test_codegen_old.c ${CMAKE_CURRENT_BINARY_DIR}/chemistry_old.c)
target_link_libraries(test_codegen_old dlite uuid)
target_include_directories(test_codegen_old PRIVATE
  ${dlite_SOURCE_DIR}/src
  ${dlite_BINARY_DIR}/src
  ${uuid_BINARY_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  )
add_dependencies(test_codegen_old dlite uuidProj)

add_test(
  NAME test-codegen_old
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test_codegen_old
)
set_tests_properties(test-codegen_old PROPERTIES
  DEPENDS test_codegen_old)
