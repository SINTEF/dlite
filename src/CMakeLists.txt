# -*- Mode: cmake -*-
project(dlite-src C)

# Workaround for CMake not setting the correct binary path for MSVC
if(MSVC AND ${CMAKE_VERSION} VERSION_LESS "3.12.10")
  set(dlite-src_BINARY_DIR ${dlite-src_BINARY_DIR}/$<CONFIG>)
endif()


# Generate config.h
# -----------------
configure_file(config.h.in config.h)


#-----------------------------------------------------
# Compile library
#-----------------------------------------------------

add_subdirectory(utils)

set(sources
  dlite-misc.c
  dlite-store.c
  dlite-type.c
  dlite-arrays.c
  dlite-datamodel.c
  dlite-schemas.c
  dlite-entity.c
  dlite-collection.c
  dlite-storage.c
  dlite-storage-plugins.c
  dlite-mapping.c
  dlite-mapping-plugins.c
  dlite-codegen.c
  dlite-getlicense.c
  getuuid.c
  triplestore.c
  )

add_definitions(
  -DHAVE_CONFIG_H
  #-DWITH_DEBUG
  )


# We are building both shared and static libraries.  User software will
# normally link to the shared libraries.  However, to simplify the
# library search (especially on Windows), the storage plugins will use
# the static libraries.  For this to work, the static libraries will be
# build position independent.
add_library(dlite SHARED ${sources})
add_library(dlite-static STATIC ${sources})

target_link_libraries(dlite
  dlite-utils
  ${CMAKE_DL_LIBS}
  )
target_link_libraries(dlite-static
  dlite-utils-static
  ${CMAKE_DL_LIBS}
  )

target_include_directories(dlite PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  $<INSTALL_INTERFACE:include/dlite>
  )
target_include_directories(dlite-static PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  $<INSTALL_INTERFACE:include/dlite>
  )

set_target_properties(dlite PROPERTIES
  WINDOWS_EXPORT_ALL_SYMBOLS ON
  )
set_target_properties(dlite-static PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  )

#set_property(TARGET dlite-static-pic PROPERTY POSITION_INDEPENDENT_CODE ON)


## Generate dlite_get_license.c
add_executable(gencfile utils/gencfile.c)
add_custom_command(
  OUTPUT dlite-getlicense.c
  COMMAND $<TARGET_FILE:gencfile>
    dlite_get_license
    ${CMAKE_CURRENT_BINARY_DIR}/dlite-getlicense.c
    ${dlite_SOURCE_DIR}/LICENSE.txt
    ${dlite-src-utils_SOURCE_DIR}/LICENSES.txt
  DEPENDS
    gencfile
    ${dlite_SOURCE_DIR}/LICENSE.txt
    ${dlite-src-utils_SOURCE_DIR}/LICENSES.txt
  COMMENT "Generating dlite-getlicense.c"
)


## create list of headers to install from ${sources}
set(headers dlite.h)
foreach(source ${sources})
  string(REPLACE ".c" ".h" header ${source})
  list(APPEND headers ${header})
endforeach()
set_target_properties(dlite PROPERTIES PUBLIC_HEADER "${headers}")


install(
  TARGETS dlite dlite-static
  EXPORT  dliteTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  PUBLIC_HEADER DESTINATION include/dlite
  INCLUDES DESTINATION include/dlite
)


# Install empty directories
install(DIRECTORY DESTINATION share/dlite/storages)
install(DIRECTORY DESTINATION share/dlite/mapping-plugins)


# install .pdb files - seems to be needed on Windows if we are
# installing dlite compiled in debug mode
if(WIN32 AND MSVC AND (NOT MSVC_VERSION LESS 1600) AND
    (NOT CMAKE_VERSION VERSION_LESS "3.1"))
  install(FILES $<TARGET_PDB_FILE:dlite> DESTINATION bin OPTIONAL)
endif()


# Support driver and mapping plugins written in Python
if(WITH_PYTHON)
  add_subdirectory(pyembed)
endif()

# Tests
add_subdirectory(tests)
