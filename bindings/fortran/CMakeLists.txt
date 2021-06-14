project(dlite-bindings-fortran Fortran)

configure_file(dlite_config.f90.in dlite_config.f90)

set(sources
  c_interface.f90
  ${CMAKE_CURRENT_BINARY_DIR}/dlite_config.f90
  dlite.f90
  )

if(WITH_PYTHON)
  set(pyembed_libs dlite-pyembed)
endif()

add_library(dlite-fortran SHARED ${sources})

target_link_libraries(dlite-fortran
  dlite
  dlite-utils
  ${pyembed_libs}
  )




add_subdirectory(tests)
