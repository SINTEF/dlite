# -*- Mode: cmake -*-
#

set(python-tests
  bson_test
  yaml_test
  postgresql_test
  )

include(FindPythonModule)

# Disable YAML test if PyYAML is not in installed
find_python_module(yaml)
if(NOT PY_YAML)
  add_test(NAME yaml_test
    COMMAND ${CMAKE_COMMAND} -E echo "disabled (no PyYAML)")
  set_property(TEST yaml_test PROPERTY DISABLED True)
  list(REMOVE_ITEM python-tests yaml_test)
endif()

# Disable BSON test if pymongo is not installed
find_python_module(pymongo)
if(NOT PY_PYMONGO)
  add_test(NAME bson_test
    COMMAND ${CMAKE_COMMAND} -E echo "disabled (no pymongo)")
  set_property(TEST bson_test PROPERTY DISABLED True)
  list(REMOVE_ITEM python-tests bson_test)
endif()

# Disable PostgreSQL test if psycopg2 is not installed
find_python_module(psycopg2)
if(NOT PY_PSYCOPG2)
  add_test(NAME postgresql_test
    COMMAND ${CMAKE_COMMAND} -E echo "disabled (no psycopg2)")
  set_property(TEST postgresql_test PROPERTY DISABLED True)
  list(REMOVE_ITEM python-tests postgresql_test)
endif()

# Add tests
foreach(test ${python-tests})
  add_test(
    NAME ${test}
    COMMAND ${RUNNER} ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/${test}.py
    )

  set_property(TEST ${test} PROPERTY
    ENVIRONMENT "PATH=${dlite_PATH_NATIVE}")
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
  set_property(TEST ${test} APPEND PROPERTY
    ENVIRONMENT "DLITE_USE_BUILD_ROOT=YES")
  if (NOT WIN32)
  set_property(TEST ${test} APPEND PROPERTY
    ENVIRONMENT "LD_LIBRARY_PATH=${dlite_LD_LIBRARY_PATH_NATIVE}")
  endif()

endforeach()
