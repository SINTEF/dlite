# -*- Mode: cmake -*-
#

set(tests
  test_python_bindings
  test_collection
  test_entity
  test_factory
  test_misc
  test_python_storage
  test_storage
  )

foreach(test ${tests})
  set(name ${test}-py)
  add_test(
    NAME ${name}
    COMMAND ${RUNNER} ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/${test}.py
    )

  set_property(TEST ${name} PROPERTY
    ENVIRONMENT "PATH=${dlite_PATH_NATIVE}")
  set_property(TEST ${name} APPEND PROPERTY
    ENVIRONMENT "PYTHONPATH=${dlite_PYTHONPATH_NATIVE}")
  set_property(TEST ${name} APPEND PROPERTY
    ENVIRONMENT "LD_LIBRARY_PATH=${dlite_LD_LIBRARY_PATH_NATIVE}")
  set_property(TEST ${name} APPEND PROPERTY
    ENVIRONMENT "DLITE_USE_BUILD_ROOT=YES")

endforeach()
