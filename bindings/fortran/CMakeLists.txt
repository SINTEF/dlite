project(dlite-bindings-fortran Fortran)

add_executable(class_circle class_circle.f90)



set(sources
  dlite.f90
  )

add_library(dlite-fortran SHARED ${sources})

target_link_libraries(dlite-fortran
  dlite
  dlite-utils
  )

add_subdirectory(tests)
