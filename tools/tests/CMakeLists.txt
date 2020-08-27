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
  include(dliteCodeGen)

  dlite_codegen(
    chemistry.h
    c-header
    json://${CMAKE_CURRENT_SOURCE_DIR}/Chemistry-0.1.json
    )

  dlite_codegen(
    chemistry_schema.h
    c-meta-header
    json://${CMAKE_CURRENT_SOURCE_DIR}/Chemistry-0.1.json
    )

  dlite_codegen(
    chemistry.c
    c-source
    json://${CMAKE_CURRENT_SOURCE_DIR}/Chemistry-0.1.json
    )

  add_executable(test_codegen
    test_codegen.c ${CMAKE_CURRENT_BINARY_DIR}/chemistry.h)

  add_executable(test_ext_header
    test_ext_header.c
    ${CMAKE_CURRENT_BINARY_DIR}/chemistry.h
    )

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
    set_property(TEST ${test} PROPERTY
      ENVIRONMENT "DLITE_USE_BUILDROOT=YES")
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
