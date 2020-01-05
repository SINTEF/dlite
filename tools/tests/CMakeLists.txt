# -*- Mode: cmake -*-
#

add_definitions(-DDLITE_ROOT=${dlite_SOURCE_DIR})

if(WINDOWS)
  set(exeext .exe)
endif()

if(WITH_JSON)
  add_custom_command(
    OUTPUT chemistry.h
    COMMAND $<TARGET_FILE:dlite-codegen>
    COMMAND
      ${CMAKE_COMMAND} -E env
      PATH="${dlite_PATH}"
      LD_LIBRARY_PATH="${dlite_LD_LIBRARY_PATH}"
      DLITE_STORAGE_PLUGIN_DIRS="${dlite_STORAGE_PLUGINS}"
      DLITE_MAPPING_PLUGIN_DIRS="${dlite_MAPPING_PLUGINS}"
      DLITE_TEMPLATE_DIRS="${dlite_TEMPLATES}"
      ${dlite-tools_BINARY_DIR}/dlite-codegen
      --template ${dlite_SOURCE_DIR}/tools/templates/c-header.txt
      --output chemistry.h
      --storage-plugins=${dlite_BINARY_DIR}/storages/json
      json://${CMAKE_CURRENT_SOURCE_DIR}/Chemistry-0.1.json
    MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/Chemistry-0.1.json
    DEPENDS ${dlite_SOURCE_DIR}/tools/templates/c-header.txt
    DEPENDS dlite-codegen
    COMMENT "Generating C header for Chemistry-0.1.json"
  )

  add_executable(test_codegen
    test_codegen.c ${CMAKE_CURRENT_BINARY_DIR}/chemistry.h)
  target_link_libraries(test_codegen
    dlite-static
    )
  target_include_directories(test_codegen PRIVATE
    ${dlite_SOURCE_DIR}/src
    ${dlite_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    )

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
    ENVIRONMENT "DLITE_STORAGE_PLUGIN_DIRS=${dlite_STORAGE_PLUGINS}")
  set_property(TEST ${test} APPEND PROPERTY
    ENVIRONMENT "DLITE_MAPPING_PLUGIN_DIRS=${dlite_MAPPING_PLUGINS}")
  set_property(TEST ${test} APPEND PROPERTY
    ENVIRONMENT "DLITE_TEMPLATE_DIRS=${dlite_TEMPLATES}")
  set_property(TEST ${test} APPEND PROPERTY
    ENVIRONMENT "DLITE_STORAGES=${dlite_STORAGES}")

endif()


install(
  FILES Chemistry-0.1.json
  DESTINATION share/dlite/examples
  )
