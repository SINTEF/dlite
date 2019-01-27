# -*- Mode: cmake -*-
#

add_definitions(-DDLITE_ROOT=${dlite_SOURCE_DIR})

if(WITH_JSON)
  add_custom_command(
    OUTPUT chemistry.h
    COMMAND
      ${CMAKE_COMMAND} -E env "PATH=${uuid_BINARY_DIR}:$ENV{PATH}"
      ${dlite-tools_BINARY_DIR}/dlite-codegen
      --template ${dlite_SOURCE_DIR}/tools/templates/c-header.txt
      --output chemistry.h
      --storage-plugins=${dlite_BINARY_DIR}/plugins/json
      json://${CMAKE_CURRENT_SOURCE_DIR}/Chemistry-0.1.json
    MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/Chemistry-0.1.json
    DEPENDS ${dlite_SOURCE_DIR}/tools/templates/c-header.txt
    DEPENDS ${dlite_BINARY_DIR}/tools/dlite-codegen
    COMMENT "Generating C header for Chemistry-0.1.json"
  )

  add_executable(test_codegen
    test_codegen.c ${CMAKE_CURRENT_BINARY_DIR}/chemistry.h)
  target_link_libraries(test_codegen
    dlite-static
    #uuid-static
    ${UUID_STATIC_LIBRARIES}
    )
  target_include_directories(test_codegen PRIVATE
    ${dlite_SOURCE_DIR}/src
    ${dlite_BINARY_DIR}/src
    ${UUID_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}
    )
  add_dependencies(test_codegen dlite-static ${uuidProj})

  set(test test_codegen)
  add_test(
    NAME ${test}
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test_codegen
  )
  set_property(TEST ${test} PROPERTY
    ENVIRONMENT "PATH=${dlite_PATH}")
  set_property(TEST ${test} APPEND PROPERTY
    ENVIRONMENT "LD_LIBRARY_PATH=${dlite_LD_LIBRARY_PATH}")
  set_property(TEST ${test} APPEND PROPERTY
    ENVIRONMENT "DLITE_STORAGE_PLUGINS=${dlite_STORAGE_PLUGINS}")

endif()
