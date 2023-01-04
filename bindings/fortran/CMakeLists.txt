project(dlite-bindings-fortran Fortran)

configure_file(dlite_config.f90.in dlite_config.f90)

set(sources
  c_interface.f90
  ${CMAKE_CURRENT_BINARY_DIR}/dlite_config.f90
  dlite.f90
  )

add_library(dlite-fortran SHARED ${sources})

target_link_libraries(dlite-fortran
  dlite
  dlite-utils
  )


## create list of modules to install from ${sources}
set(modules "")
foreach(source ${sources})
  string(REPLACE ".f90" ".mod" module ${source})
  if(EXISTS ${CMAKE_CURRENT_BINARY_DIR}/${module})
      list(APPEND modules ${CMAKE_CURRENT_BINARY_DIR}/${module})
  endif()
endforeach()
set_target_properties(dlite-fortran PROPERTIES PUBLIC_HEADER "${modules}")

## Install modules in 
install(TARGETS dlite-fortran
  PUBLIC_HEADER DESTINATION include/dlite
)


add_subdirectory(tests)
