# -*- Mode: cmake -*-
project(dlite-src C)

# Workaround for CMake not setting the correct binary path for MSVC
if(MSVC AND ${CMAKE_VERSION} VERSION_LESS "3.12.10")
  set(dlite-src_BINARY_DIR ${dlite-src_BINARY_DIR}/$<CONFIG>)
endif()


#-----------------------------------------------------
# Generate config headers
#-----------------------------------------------------
add_subdirectory(utils)
configure_file(config.h.in config.h)
configure_file(config-paths.h.in config-paths.h)

include(dliteGenEnv)
dlite_genenv(${CMAKE_CURRENT_BINARY_DIR}/config-paths.h NATIVE)



#-----------------------------------------------------
# Compile library
#-----------------------------------------------------

# Support driver and mapping plugins written in Python
if(WITH_PYTHON)
  add_subdirectory(pyembed)
endif()

set(sources
  dlite-errors.c
  dlite-misc.c
  dlite-store.c
  dlite-type.c
  dlite-type-cast.c
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
  dlite-json.c
  dlite-bson.c
  dlite-behavior.c
  pathshash.c
  triple.c
  triplestore.c
  ${pyembed_sources}
  )

add_definitions(
  -DHAVE_CONFIG_H
  #-DWITH_DEBUG
  )

set(include_directories_extra)
set(link_libraries_extra)

if(HAVE_REDLAND)
  list(APPEND include_directories_extra ${REDLAND_INCLUDE_DIR})
  list(APPEND link_libraries_extra ${REDLAND_LIBRARY})
endif()

if(HAVE_RASQAL)
  list(APPEND include_directories_extra ${RASQAL_INCLUDE_DIR})
  list(APPEND link_libraries_extra ${RASQAL_LIBRARIES})
endif()

if(HAVE_RAPTOR)
  list(APPEND include_directories_extra ${RAPTOR_INCLUDE_DIR})
  list(APPEND link_libraries_extra ${RAPTOR_LIBRARY})
endif()

if(WITH_PYTHON)
  list(APPEND include_directories_extra ${Python3_INCLUDE_DIRS})
  if(NOT WITH_STATIC_PYTHON)
    # Force Linux to use the full path to the Python library instead of the SONAME
    add_library(python_lib SHARED IMPORTED)
    set_target_properties(python_lib PROPERTIES
      # Unix
      IMPORTED_LOCATION ${Python3_LIBRARIES}
      IMPORTED_NO_SONAME TRUE
      # Windows
      IMPORTED_IMPLIB ${Python3_LIBRARIES} # .lib or .dll.a
      # IMPORTED_LOCATION # .dll
      )
    list(APPEND link_libraries_extra python_lib)
  endif()
  # Python uses the intl library only for the gettext & locale modules
  if(NOT APPLE)
    find_library(Intl_LIBRARY intl)
  endif()
  # macOS does not natively have gettext. It is usually supplied by Homebrew
  # or Macports. This dependancy adds complexity for the user and elevates
  # the deployment target unnecessaily for little gained functionality.
  # intl is not linked in this buiild for macOS unless Intl_LIBRARY is provided
  # at configure
  if(Intl_LIBRARY)
    if(Intl_IS_BUILT_IN)
      message(STATUS "Found intl: Contained in the C standard library")
    else()
      list(APPEND link_libraries_extra ${Intl_LIBRARY})
      message(STATUS "Found intl: ${Intl_LIBRARY}")
    endif()
  endif()
endif()


# We are building both shared and static libraries. User software will
# normally link to the shared libraries.

set(lib_args
  ${sources}
  ${CMAKE_CURRENT_BINARY_DIR}/config-paths.h
  )
add_library(dlite SHARED ${lib_args})
add_library(dlite-static STATIC ${lib_args})

set(include_args
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include/dlite>
  ${include_directories_extra}
  )
target_include_directories(dlite PUBLIC ${include_args})
target_include_directories(dlite-static PUBLIC ${include_args})

#target_include_directories(dlite PUBLIC
#  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
#  $<INSTALL_INTERFACE:include/dlite>
#  )
#target_include_directories(dlite-static PUBLIC
#  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
#  $<INSTALL_INTERFACE:include/dlite>
#  )

set(link_args
  dlite-utils
  ${link_libraries_extra}
  )
if(NOT WITH_STATIC_PYTHON)
  # CMAKE_DL_LIBS is empty on MacOS
  list(APPEND link_args ${CMAKE_DL_LIBS})
endif()

target_link_libraries(dlite ${link_args})
target_link_libraries(dlite-static ${link_args})

# LIBRARY RPATH/ RUNPATH: dlite
# =============================
# 
# At build, set absolute RPATHS to the other library.
# 
# At install the library will be loaded from the directory:
# 
#   ${CMAKE_INSTALL_PREFIX}/lib/python${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}/site-packages/dlite
#   - OR - using CMake dlite directory variables:
#   ${DLITE_PYTHONPATH}/dlite
# 
# The linked library will be copied into the above noted folder at install.

if(APPLE)
  set_target_properties(dlite PROPERTIES
    BUILD_WITH_INSTALL_RPATH FALSE
    BUILD_RPATH "${dlite_BINARY_DIR}/src/utils"
    INSTALL_RPATH "@loader_path"
    )
else()
  set_target_properties(dlite PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS ON
    BUILD_WITH_INSTALL_RPATH FALSE
    BUILD_RPATH "${dlite_BINARY_DIR}/src/utils"
    INSTALL_RPATH "$ORIGIN"
    )
endif()

set_target_properties(dlite-static PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  )

#set_property(TARGET dlite-static-pic PROPERTY POSITION_INDEPENDENT_CODE ON)
set_source_files_properties(
    dlite-misc.c
    dlite-storage.c
    dlite-storage-plugins.c
    dlite-mapping-plugins.c
    dlite-codegen.c
  PROPERTIES
    OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/config-paths.h
  )


## Generate dlite_get_license.c
add_executable(gencfile utils/gencfile.c)
target_include_directories(gencfile PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/utils)
add_custom_command(
  OUTPUT dlite-getlicense.c
  COMMAND ${RUNNER} $<TARGET_FILE:gencfile>
    dlite_get_license
    ${CMAKE_CURRENT_BINARY_DIR}/dlite-getlicense.c
    ${dlite_SOURCE_DIR}/LICENSE
    ${dlite-src-utils_SOURCE_DIR}/LICENSES.txt
  DEPENDS
    gencfile
    ${dlite_SOURCE_DIR}/LICENSE
    ${dlite-src-utils_SOURCE_DIR}/LICENSES.txt
  COMMENT "Generating dlite-getlicense.c"
)


## create list of headers to install from ${sources}
set(headers dlite.h dlite-errors.h)
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


# Tests
add_subdirectory(tests)
