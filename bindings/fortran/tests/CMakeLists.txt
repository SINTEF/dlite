# -*- Mode: cmake -*-
#

include(dliteCodeGen)

add_compile_options("-DDLITE_ROOT=${DLITE_ROOT}")

set(metadata
  Person
  Scan3D
  Animal
)
foreach(name ${metadata})
  dlite_codegen(
    ${CMAKE_CURRENT_BINARY_DIR}/${name}.f90
    fortran-module
    json://${CMAKE_CURRENT_SOURCE_DIR}/${name}.json?mode=r
    ENV_OPTIONS --build
    --build-root
    )
  add_library(${name} ${CMAKE_CURRENT_BINARY_DIR}/${name}.f90)
  target_link_libraries(${name}
    dlite
    dlite-utils
    dlite-fortran
    )
  target_include_directories(${name} PRIVATE
    ${dlite-bindings-fortran_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    )
endforeach()

set(tests
  test_person
  test_animal
  )

foreach(test ${tests})
  add_executable(${test} ${test}.f90)
  target_link_libraries(${test}
    dlite
    dlite-utils
    dlite-fortran
    Person
    Scan3D
    Animal
    )
  target_include_directories(${test} PRIVATE
    ${dlite-bindings-fortran_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    )

  # TEST RPATH/ RUNPATH
  # ===================
  # 
  # These tests will be run from the Build directory only.
  # Set absolute RPATHS to the other libraries.
  # TODO: Change all paths to generators e.g. for dlite, dlite-utils etc as easier to maintain

  set_target_properties(${test} PROPERTIES
    BUILD_WITH_INSTALL_RPATH FALSE
    BUILD_RPATH "${dlite_BINARY_DIR}/src;${dlite_BINARY_DIR}/src/utils;$<TARGET_FILE_DIR:dlite-fortran>"
    )

  set(name ${test}-f90)
  add_test(
    NAME ${name}
    COMMAND ${RUNNER} ${test}
    )

  set_property(TEST ${name} PROPERTY
    ENVIRONMENT "PATH=${dlite_PATH_NATIVE}")
  set_property(TEST ${name} APPEND PROPERTY
    ENVIRONMENT "WINEPATH=${dlite_WINEPATH_NATIVE}")
  set_property(TEST ${name} APPEND PROPERTY
    ENVIRONMENT "PYTHONPATH=${dlite_PYTHONPATH_NATIVE}")
  set_property(TEST ${name} APPEND PROPERTY
    ENVIRONMENT "LD_LIBRARY_PATH=${dlite_LD_LIBRARY_PATH_NATIVE}")
  set_property(TEST ${name} APPEND PROPERTY
    ENVIRONMENT "DLITE_USE_BUILD_ROOT=YES")

endforeach()
