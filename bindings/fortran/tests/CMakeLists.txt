# -*- Mode: cmake -*-
#

include(dliteCodeGen)

add_compile_options("-DDLITE_ROOT=${DLITE_ROOT}")

dlite_codegen(
  ${CMAKE_CURRENT_BINARY_DIR}/Person.f90
  fortran-module
  json://${CMAKE_CURRENT_SOURCE_DIR}/Person.json?mode=r
  ENV_OPTIONS --build
  --build-root
  )

dlite_codegen(
  ${CMAKE_CURRENT_BINARY_DIR}/Scan3D.f90
  fortran-module
  json://${CMAKE_CURRENT_SOURCE_DIR}/Scan3D.json?mode=r
  ENV_OPTIONS --build
  --build-root
  )

set(tests
  test_person
  )

add_library(Person ${CMAKE_CURRENT_BINARY_DIR}/Person.f90)
target_link_libraries(Person
  dlite
  dlite-utils
  dlite-fortran
  )
target_include_directories(Person PRIVATE
  ${dlite-bindings-fortran_BINARY_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  )

add_library(Scan3D ${CMAKE_CURRENT_BINARY_DIR}/Scan3D.f90)
target_link_libraries(Scan3D
  dlite
  dlite-utils
  dlite-fortran
  )
target_include_directories(Scan3D PRIVATE
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
    Scan3D
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
    ENVIRONMENT "PATH=${dlite_PATH_NATIVE}")
  set_property(TEST ${name} APPEND PROPERTY
    ENVIRONMENT "PYTHONPATH=${dlite_PYTHONPATH_NATIVE}")
  set_property(TEST ${name} APPEND PROPERTY
    ENVIRONMENT "LD_LIBRARY_PATH=${dlite_LD_LIBRARY_PATH_NATIVE}")
  set_property(TEST ${name} APPEND PROPERTY
    ENVIRONMENT "DLITE_USE_BUILD_ROOT=YES")

endforeach()
