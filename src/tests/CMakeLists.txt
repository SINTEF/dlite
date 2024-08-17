# -*- Mode: cmake -*-
project(dlite-src-tests C)


set(tests
  test_misc
  test_errors
  test_type
  test_storage
  test_entity
  test_property
  test_json
  test_dlitebson
  test_metamodel
  test_store
  test_triple
  test_triplestore
  test_collection
  test_schemas
  test_arrays
  test_ref
  test_getuuid
  )

list(APPEND tests test_json_entity)
list(APPEND tests test_storage_lookup)
list(APPEND tests test_mapping)
if(WITH_HDF5)
  list(APPEND tests test_datamodel)
endif()
if(WITH_POSTGRESQL)
  list(APPEND tests test_postgresql_storage)
endif()

add_definitions(
  -Ddlite_SOURCE_DIR=${dlite_SOURCE_DIR}
  -Ddlite_BINARY_DIR=${dlite_BINARY_DIR}
  -DDLITE_BINARY_ROOT=${dlite_BINARY_DIR}
  )

add_subdirectory(mappings)
if(WITH_PYTHON)
  add_subdirectory(python)
endif()

foreach(test ${tests})
  add_executable(${test} ${test}.c)
  target_link_libraries(${test}
    dlite
    dlite-utils
    ${extra_link_libraries}
  )
  target_include_directories(${test} PRIVATE
    ${dlite_SOURCE_DIR}/src
    ${dlite_BINARY_DIR}/src
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

  if(NOT WIN32)
    set_property(TEST ${test} APPEND PROPERTY
      ENVIRONMENT "LD_LIBRARY_PATH=${dlite_LD_LIBRARY_PATH_NATIVE}")
  endif()

  if(WIN32 AND PYTHON_IS_ANACONDA)
    set_property(TEST ${test} APPEND PROPERTY
      ENVIRONMENT "PYTHONHOME=${Python3_RUNTIME_LIBRARY_DIRS}")
  endif()

  set_property(TEST ${test} APPEND PROPERTY
    ENVIRONMENT "PYTHONPATH=${dlite_PYTHONPATH_NATIVE}")

  set_property(TEST ${test} APPEND PROPERTY
    ENVIRONMENT "DLITE_USE_BUILD_ROOT=YES")

  # Needed by test_mapping
  set_property(TEST ${test} APPEND PROPERTY
    ENVIRONMENT "DLITE_MAPPING_PLUGIN_DIRS=$<SHELL_PATH:$<TARGET_FILE_DIR:mapA>>")

endforeach()
