# -*- Mode: cmake -*-
#

set(tests
  test_person
  )


add_library(Person Person.f90)
target_link_libraries(Person
  dlite
  dlite-utils
  dlite-fortran
  )
target_include_directories(Person PRIVATE
  ${dlite-bindings-fortran_BINARY_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  )


foreach(test ${tests})
  add_executable(${test} ${test}.f90)
  target_link_libraries(${test}
    dlite
    dlite-utils
    dlite-fortran
    Person
    )
  target_include_directories(${test} PRIVATE
    ${dlite-bindings-fortran_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    )

  set(name ${test}-f90)
  add_test(
    NAME ${name}
    COMMAND ${test}
    )

  set_property(TEST ${name} PROPERTY
    ENVIRONMENT "PATH=${dlite_PATH}")
  set_property(TEST ${name} APPEND PROPERTY
    ENVIRONMENT "PYTHONPATH=${dlite_PYTHONPATH}")
  set_property(TEST ${name} APPEND PROPERTY
    ENVIRONMENT "LD_LIBRARY_PATH=${dlite_LD_LIBRARY_PATH}")
  set_property(TEST ${name} APPEND PROPERTY
    ENVIRONMENT "DLITE_STORAGE_PLUGIN_DIRS=${dlite_STORAGE_PLUGINS}")
  set_property(TEST ${name} APPEND PROPERTY
    ENVIRONMENT "DLITE_MAPPING_PLUGIN_DIRS=${dlite_MAPPING_PLUGINS}")
  set_property(TEST ${name} APPEND PROPERTY ENVIRONMENT
    "DLITE_PYTHON_STORAGE_PLUGIN_DIRS=${dlite_PYTHON_STORAGE_PLUGINS}")
  set_property(TEST ${name} APPEND PROPERTY ENVIRONMENT
     "DLITE_PYTHON_MAPPING_PLUGIN_DIRS=${dlite_PYTHON_MAPPING_PLUGINS}")
  set_property(TEST ${name} APPEND PROPERTY
    ENVIRONMENT "DLITE_TEMPLATE_DIRS=${dlite_TEMPLATES}")
  set_property(TEST ${name} APPEND PROPERTY
    ENVIRONMENT "DLITE_STORAGES=${dlite_STORAGES}")

endforeach()
