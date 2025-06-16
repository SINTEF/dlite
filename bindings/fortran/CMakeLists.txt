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

# LIBRARY RPATH/ RUNPATH: dlite-fortran
# =====================================
# 
# At build, set absolute RPATHS to the other libraries.
# 
# At install the library will be loaded from the directory:
# 
#   ${CMAKE_INSTALL_PREFIX}/lib/python${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}/site-packages/dlite
#   - OR - using CMake dlite directory variables:
#   ${DLITE_PYTHONPATH}/dlite
# 
# The two linked libraries will be copied into the above noted folder at install.

if(APPLE)
  set_target_properties(dlite-fortran PROPERTIES
    BUILD_WITH_INSTALL_RPATH FALSE
    BUILD_RPATH "${dlite_BINARY_DIR}/src;${dlite_BINARY_DIR}/src/utils"
    # TODO: The linker seems to include "@loader_path" itself; can't determine how, not in link.txt
    # Not sure if the linker brings in RPATHS from linked libraries or not
    # INSTALL_RPATH "@loader_path"
    )
else()
  set_target_properties(dlite-fortran PROPERTIES
    BUILD_WITH_INSTALL_RPATH FALSE
    BUILD_RPATH "${dlite_BINARY_DIR}/src;${dlite_BINARY_DIR}/src/utils"
    INSTALL_RPATH "$ORIGIN"
    )
endif()

## create list of modules to install from ${sources}
set(modules "")
foreach(source ${sources})
    get_filename_component(barename ${source} NAME)
    string(REPLACE ".f90" ".mod" module ${barename})
    list(APPEND modules ${CMAKE_CURRENT_BINARY_DIR}/${module})
endforeach()
set_target_properties(dlite-fortran PROPERTIES PUBLIC_HEADER "${modules}")

## Install modules in 
install(TARGETS dlite-fortran
  PUBLIC_HEADER DESTINATION include/dlite
)


add_subdirectory(tests)
