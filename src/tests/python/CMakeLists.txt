# -*- Mode: cmake -*-
#

set(tests
  test_pyembed
  test_python_mapping
  )

add_definitions(
  -DTESTDIR=${CMAKE_CURRENT_SOURCE_DIR}
  -DDLITE_ROOT=${dlite_SOURCE_DIR}
  -DDLITE_BINARY_ROOT=${dlite_BINARY_DIR}
  )

foreach(test ${tests})
  add_executable(${test} ${test}.c)
  target_link_libraries(${test}
    dlite
    dlite-utils
    ${extra_link_libraries}
  )
  target_include_directories(${test} PRIVATE
    ${dlite-src_SOURCE_DIR}
    ${dlite-src_BINARY_DIR}
    ${dlite-src_SOURCE_DIR}/pyembed
    ${dlite-src_SOURCE_DIR}/tests
  )

  # TEST RPATH/ RUNPATH
  # ===================
  # 
  # These tests will be run from the Build directory only.
  # Set absolute RPATHS to the other libraries.
  # TODO: Change all paths to generators e.g. for dlite, dlite-utils etc as easier to maintain

  set_property(TARGET ${test} PROPERTY
    BUILD_WITH_INSTALL_RPATH FALSE
    BUILD_RPATH "${dlite_BINARY_DIR}/src;${dlite_BINARY_DIR}/src/utils"
    )

  add_test(
    NAME ${test}
    COMMAND ${RUNNER} ${test}
    )

  set_property(TEST ${test} PROPERTY
    ENVIRONMENT "PATH=${dlite_PATH_NATIVE}")
  if(MINGW)
    set_property(TEST ${test} APPEND PROPERTY
      ENVIRONMENT "WINEPATH=${dlite_WINEPATH_NATIVE}")
  endif()
  if (WIN32 AND PYTHON_IS_ANACONDA_BASE)
    set_property(TEST ${test} APPEND PROPERTY
      ENVIRONMENT "PYTHONHOME=${Python3_RUNTIME_LIBRARY_DIRS}")
  endif()
  set_property(TEST ${test} APPEND PROPERTY
    ENVIRONMENT "PYTHONPATH=${dlite_PYTHONPATH_NATIVE}")
  if (NOT WIN32)
  set_property(TEST ${test} APPEND PROPERTY
    ENVIRONMENT "LD_LIBRARY_PATH=${dlite_LD_LIBRARY_PATH_NATIVE}")
  endif()
  set_property(TEST ${test} APPEND PROPERTY
    ENVIRONMENT "DLITE_USE_BUILD_ROOT=YES")

endforeach()
