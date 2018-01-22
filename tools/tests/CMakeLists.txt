# -*- Mode: cmake -*-
#

find_package(PythonInterp)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/chemistry.c
         ${CMAKE_CURRENT_BINARY_DIR}/chemistry.h
  COMMAND ${PYTHON_EXECUTABLE} ${dlite_SOURCE_DIR}/tools/codegen_c.py --destination ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/Chemistry-0.1.json
  MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/Chemistry-0.1.json
  DEPENDS ${dlite_SOURCE_DIR}/tools/codegen.py
          ${dlite_SOURCE_DIR}/tools/codegen_c.py
  COMMENT "Generating C source and header for Chemistry-0.1.json"
)

add_executable(main main.c ${CMAKE_CURRENT_BINARY_DIR}/chemistry.c)
target_link_libraries(main dlite uuid)
target_include_directories(main PRIVATE
  ${dlite_SOURCE_DIR}/src
  ${dlite_BINARY_DIR}/src
  ${uuid_BINARY_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  )
add_dependencies(main dlite uuidProj)

add_test(
  NAME test_chemistry
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/main
)
set_tests_properties(test_chemistry PROPERTIES
  DEPENDS main)
