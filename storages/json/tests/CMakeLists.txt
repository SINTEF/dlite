# -*- Mode: cmake -*-
#

set(tests
  test_json_storage
  )

add_definitions(-DDLITE_ROOT=${dlite_SOURCE_DIR})

# We are linking to dlite-plugins-json DLL - this require that this
# DLL is in the PATH on Windows. Copying the DLL to the current
# BINARY_DIR is a simple way to ensure this.
add_custom_target(
  copy-dlite-plugins-json
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:dlite-plugins-json>
    ${dlite_BINARY_DIR}/storages/json/tests
  )

set(dlite_PATH
  $<TARGET_FILE_DIR:dlite>
  $<TARGET_FILE_DIR:dlite-utils>
  $<TARGET_FILE_DIR:dlite-plugins-json>
  )
if(WITH_PYTHON)
  get_filename_component(Python3_LIBDIR ${Python3_LIBRARY_DIRS} DIRECTORY)
  list(APPEND dlite_PATH ${Python3_LIBDIR})
endif()

foreach(test ${tests})
  add_executable(${test} ${test}.c)
  target_link_libraries(${test}
    dlite
    dlite-plugins-json
    ${extra_link_libraries}
  )

  target_include_directories(${test} PRIVATE
    ${dlite_SOURCE_DIR}/storages/json
    ${dlite-src_SOURCE_DIR}/tests
    )
  add_dependencies(${test} copy-dlite-plugins-json)
  
# TEST RPATH/ RUNPATH
# ===================
# 
# These tests will be run from the Build directory only.
# Set absolute RPATHS to the other libraries.
# TODO: Change all paths to generators e.g. for dlite, dlite-utils etc as easier to maintain

  set_property(TARGET ${test} PROPERTY
    BUILD_WITH_INSTALL_RPATH FALSE
    BUILD_RPATH "${dlite_BINARY_DIR}/src;${dlite_BINARY_DIR}/src/utils;$<TARGET_FILE_DIR:dlite-plugins-json>"
    )

  add_test(
    NAME ${test}
    COMMAND ${RUNNER} ${test}
    )

  set_property(TEST ${test} PROPERTY
    ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<JOIN:${dlite_PATH_NATIVE},\\\;>")
  if (WIN32 AND PYTHON_IS_ANACONDA)
    set_property(TEST ${test} APPEND PROPERTY
      ENVIRONMENT "PYTHONHOME=${Python3_RUNTIME_LIBRARY_DIRS}")
  endif()
  if(MINGW)
    set_property(TEST ${test} APPEND PROPERTY
      ENVIRONMENT "WINEPATH=${dlite_WINEPATH_NATIVE}")
  endif()
  set_property(TEST ${test} APPEND PROPERTY
    ENVIRONMENT "PYTHONPATH=${dlite_PYTHONPATH_NATIVE}")
  if (UNIX AND NOT APPLE)
    set_property(TEST ${test} APPEND PROPERTY
    ENVIRONMENT "LD_LIBRARY_PATH=${dlite_LD_LIBRARY_PATH_NATIVE}")
  endif()
  set_property(TEST ${test} APPEND PROPERTY
    ENVIRONMENT "DLITE_USE_BUILD_ROOT=YES")
  set_property(TEST ${test} APPEND PROPERTY
    ENVIRONMENT "DLITE_STORAGES=$<SHELL_PATH:${dlite-src-tests_SOURCE_DIR}/*.json>")

endforeach()
