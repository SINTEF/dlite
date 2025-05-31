# -*- Mode: cmake -*-
#

set(tests
  test_codegen
  test_ext_header
  test_c_source
  )

add_definitions(-DDLITE_ROOT=${dlite_SOURCE_DIR} -DHAVE_DLITE)

if(WIN32)
  set(exeext .exe)
endif()

if(WITH_JSON)
  include(dliteCodeGen)

  dlite_codegen(
    ${CMAKE_CURRENT_BINARY_DIR}/chemistry.h
    c-header
    json://${CMAKE_CURRENT_SOURCE_DIR}/Chemistry-0.1.json
    ENV_OPTIONS --build
    --build-root
    )

  dlite_codegen(
    ${CMAKE_CURRENT_BINARY_DIR}/chemistry_schema.h
    c-meta-header
    json://${CMAKE_CURRENT_SOURCE_DIR}/Chemistry-0.1.json
    ENV_OPTIONS --build
    --build-root
    )

  dlite_codegen(
    ${CMAKE_CURRENT_BINARY_DIR}/chemistry.c
    c-source
    json://${CMAKE_CURRENT_SOURCE_DIR}/Chemistry-0.1.json
    ENV_OPTIONS --build
    --build-root
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

    target_link_libraries(${test}
      dlite
      ${extra_link_libraries}
    )
    target_include_directories(${test} PRIVATE
      ${dlite_SOURCE_DIR}/src
      ${dlite_BINARY_DIR}/src
      ${CMAKE_CURRENT_BINARY_DIR}
    )

    add_test(
      NAME ${test}
      COMMAND ${RUNNER} test_codegen
      )

    set_property(TEST ${test} PROPERTY
      ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<JOIN:${dlite_PATH_NATIVE},\\\;>")
    if(MINGW)
      set_property(TEST ${test} APPEND PROPERTY
        ENVIRONMENT "WINEPATH=${dlite_WINEPATH_NATIVE}")
    endif()
    if (UNIX AND NOT APPLE)
    set_property(TEST ${test} APPEND PROPERTY
      ENVIRONMENT "LD_LIBRARY_PATH=${dlite_LD_LIBRARY_PATH_NATIVE}")
    endif()
    set_property(TEST ${test} APPEND PROPERTY
      ENVIRONMENT "DLITE_USE_BUILD_ROOT=YES")

  endforeach()

endif()


install(
  FILES Chemistry-0.1.json
  DESTINATION share/dlite/examples
  )
