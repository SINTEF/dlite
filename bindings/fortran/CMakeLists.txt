project(dlite-bindings-fortran Fortran)

set(sources
  dlite.f90
  )

add_library(dlite-fortran SHARED ${sources})

target_link_libraries(dlite-fortran
  dlite
  dlite-utils
  )
