# -*- Mode: cmake -*-
#

set(tests
  test_codegen
  test_ext_header
  test_c_source
  )

add_definitions(-DDLITE_ROOT=${dlite_SOURCE_DIR} -DHAVE_DLITE)

if(WINDOWS)
  set(exeext .exe)
endif()

if(WITH_JSON)
  add_custom_command(
    OUTPUT chemistry.h
    COMMAND
      ${CMAKE_COMMAND} -E env
      PATH="${dlite_PATH}"
      LD_LIBRARY_PATH="${dlite_LD_LIBRARY_PATH}"
      DLITE_STORAGE_PLUGIN_DIRS="${dlite_STORAGE_PLUGINS}"
      DLITE_MAPPING_PLUGIN_DIRS="${dlite_MAPPING_PLUGINS}"
      DLITE_TEMPLATE_DIRS="${dlite_TEMPLATES}"
      $<TARGET_FILE:dlite-codegen>
        --format=c-data-header
        --output=chemistry.h
        --storage-plugins=${dlite_BINARY_DIR}/storages/json
        json://${CMAKE_CURRENT_SOURCE_DIR}/Chemistry-0.1.json
    MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/Chemistry-0.1.json
    DEPENDS ${dlite_SOURCE_DIR}/tools/templates/c-data-header.txt
    DEPENDS dlite-codegen dlite-plugins-json dlite
    COMMENT "Generating C header for Chemistry-0.1.json"
  )

  #add_custom_command(
  #  OUTPUT chemistry-ext.h
  #  COMMAND
  #    ${CMAKE_COMMAND} -E env
  #    PATH="${dlite_PATH}"
  #    LD_LIBRARY_PATH="${dlite_LD_LIBRARY_PATH}"
  #    DLITE_STORAGE_PLUGIN_DIRS="${dlite_STORAGE_PLUGINS}"
  #    DLITE_MAPPING_PLUGIN_DIRS="${dlite_MAPPING_PLUGINS}"
  #    DLITE_TEMPLATE_DIRS="${dlite_TEMPLATES}"
  #    $<TARGET_FILE:dlite-codegen>
  #      --format=c-data-header
  #      --output=chemistry-ext.h
  #      --storage-plugins=${dlite_BINARY_DIR}/storages/json
  #      json://${CMAKE_CURRENT_SOURCE_DIR}/Chemistry-0.1.json
  #  MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/Chemistry-0.1.json
  #  DEPENDS ${dlite_SOURCE_DIR}/tools/templates/c-data-header.txt
  #  DEPENDS dlite-codegen dlite-plugins-json dlite
  #  COMMENT "Generating extended C header for Chemistry-0.1.json"
  #)

  add_custom_command(
    OUTPUT chemistry_schema.h
    COMMAND
      ${CMAKE_COMMAND} -E env
      PATH="${dlite_PATH}"
      LD_LIBRARY_PATH="${dlite_LD_LIBRARY_PATH}"
      DLITE_STORAGE_PLUGIN_DIRS="${dlite_STORAGE_PLUGINS}"
      DLITE_MAPPING_PLUGIN_DIRS="${dlite_MAPPING_PLUGINS}"
      DLITE_TEMPLATE_DIRS="${dlite_TEMPLATES}"
      $<TARGET_FILE:dlite-codegen>
        --format=c-metadata-header
        --output=chemistry_schema.h
        --storage-plugins=${dlite_BINARY_DIR}/storages/json
        json://${CMAKE_CURRENT_SOURCE_DIR}/Chemistry-0.1.json
    MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/Chemistry-0.1.json
    DEPENDS ${dlite_SOURCE_DIR}/tools/templates/c-metadata-header.txt
    DEPENDS dlite-codegen dlite-plugins-json dlite
    COMMENT "Generating C header entity_schema.h"
  )

  add_custom_command(
    OUTPUT chemistry.c
    COMMAND
      ${CMAKE_COMMAND} -E env
      PATH="${dlite_PATH}"
      LD_LIBRARY_PATH="${dlite_LD_LIBRARY_PATH}"
      DLITE_STORAGE_PLUGIN_DIRS="${dlite_STORAGE_PLUGINS}"
      DLITE_MAPPING_PLUGIN_DIRS="${dlite_MAPPING_PLUGINS}"
      DLITE_TEMPLATE_DIRS="${dlite_TEMPLATES}"
      $<TARGET_FILE:dlite-codegen>
        --format=c-source
        --output=chemistry.c
        --storage-plugins=${dlite_BINARY_DIR}/storages/json
        json://${CMAKE_CURRENT_SOURCE_DIR}/Chemistry-0.1.json
    MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/Chemistry-0.1.json
    DEPENDS ${dlite_SOURCE_DIR}/tools/templates/c-source.txt
    DEPENDS dlite-codegen dlite-plugins-json dlite
    COMMENT "Generating C sources for Chemistry-0.1.json"
  )

  add_executable(test_codegen
    test_codegen.c ${CMAKE_CURRENT_BINARY_DIR}/chemistry.h)

  add_executable(test_ext_header
    test_ext_header.c ${CMAKE_CURRENT_BINARY_DIR}/chemistry-ext.h)

  add_executable(test_c_source
    test_c_source.c chemistry.c
    ${CMAKE_CURRENT_BINARY_DIR}/chemistry.h
    ${CMAKE_CURRENT_BINARY_DIR}/chemistry_schema.h
    )

  foreach(test ${tests})

    target_link_libraries(${test} dlite)
    target_include_directories(${test} PRIVATE
      ${dlite_SOURCE_DIR}/src
      ${dlite_BINARY_DIR}/src
      ${CMAKE_CURRENT_BINARY_DIR}
    )

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
  endforeach()

endif()


install(
  FILES Chemistry-0.1.json
  DESTINATION share/dlite/examples
  )
