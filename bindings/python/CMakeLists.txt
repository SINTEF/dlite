project(dlite-bindings-python C)

# Python sources
set(py_sources
  __init__.py
  factory.py
  options.py
  utils.py
  )

# Determine size of size_t
include(CheckTypeSize)
check_type_size(size_t SIZEOF_SIZE_T)
if(${SIZEOF_SIZE_T} EQUAL 4)
  set(NPY_SIZE_T NPY_UINT32)
elseif(${SIZEOF_SIZE_T} EQUAL 8)
  set(NPY_SIZE_T NPY_UINT64)
else()
  message(FATAL "Unexpected size of size_t: ${SIZEOF_SIZE_T}")
endif()

# Add macros for use with swig
# It seems that we cannot set policy  CMP0078 to NEW, since that would
# create a target named "dlite" chich would be in conflict with the target
# of src/dlite.so.
# TODO: Check if it is possible to rename the target of src/dlite.so
# using a property
cmake_policy(SET CMP0078 OLD)
set(SWIG_MODULE_pydlite_REAL_NAME "_dlite")
find_package(SWIG)
include(UseSWIG)

#
# Create Python library
# ---------------------
set(CMAKE_SWIG_OUTDIR ${CMAKE_CURRENT_BINARY_DIR})

include_directories(
  ${Python3_INCLUDE_DIRS}
  ${NUMPY_INCLUDE_DIR}
  ${dlite-src_SOURCE_DIR}
  ${dlite-src_BINARY_DIR}
  ${dlite-src-utils_BINARY_DIR}
  )

set_source_files_properties(dlite.i
  PROPERTIES
    SWIG_FLAGS "-py3;-Wall;-DNPY_SIZE_T=${NPY_SIZE_T}"
    SWIG_MODULE_NAME "dlite"
    )

SET(CMAKE_SKIP_BUILD_RPATH TRUE)
swig_add_library(dlite
  TYPE SHARED
  LANGUAGE python
  OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/dlite
  OUTFILE_DIR ${CMAKE_CURRENT_BINARY_DIR}
  SOURCES dlite.i
  )

swig_link_libraries(dlite
  dlite-utils
  dlite-pyembed
  dlite
  ${Python3_LIBRARIES}
  )


##
## Create a working Python package in binary dir
## ---------------------------------------------
set(pkgdir ${CMAKE_CURRENT_BINARY_DIR}/dlite)
set(abs_sources)
foreach(source ${py_sources})
  list(APPEND abs_sources ${CMAKE_CURRENT_SOURCE_DIR}/${source})
endforeach()

set(abs_targets)
foreach(source ${py_sources})
  list(APPEND abs_targets ${pkgdir}/${source})
endforeach()

add_custom_command(
  TARGET ${SWIG_MODULE_dlite_REAL_NAME}
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory ${pkgdir}
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:${SWIG_MODULE_dlite_REAL_NAME}>
    ${pkgdir}
  BYPRODUCTS
    ${pkgdir}
    ${pkgdir}/dlite.py
  )

add_custom_target(python_package ALL DEPENDS ${abs_targets})
add_custom_command(
  OUTPUT
    ${abs_targets}
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${abs_sources}
    ${pkgdir}
  DEPENDS
    ${pkgdir}
    ${abs_sources}
  )


#
# Install
# -------
# Based on: http://developer.intra2net.com/git/?p=libftdi;a=blob;f=python/CMakeLists.txt;h=93410544fac2a45eec51cb8dd344708b3bbdfe67;hb=HEAD
if(UNIX)
  set(pyext_ext ".so")
else()
  set(pyext_ext ".pyd")
endif()

execute_process(COMMAND
  ${Python3_EXECUTABLE} -c "from distutils import sysconfig; print(sysconfig.get_python_lib(plat_specific=True, prefix='${CMAKE_INSTALL_PREFIX}'))"
  OUTPUT_VARIABLE _ABS_Python3_MODULE_PATH
  OUTPUT_STRIP_TRAILING_WHITESPACE)
get_filename_component(_ABS_Python3_MODULE_PATH
  ${_ABS_Python3_MODULE_PATH} ABSOLUTE)
file(RELATIVE_PATH _REL_Python3_MODULE_PATH
  ${CMAKE_INSTALL_PREFIX} ${_ABS_Python3_MODULE_PATH})
set(Python3_MODULE_PATH
  ${_REL_Python3_MODULE_PATH}
)

install(
  DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/dlite
  DESTINATION ${Python3_MODULE_PATH}
  )

install(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/python-mapping-plugins
  DESTINATION share/dlite
  PATTERN ".gitignore" EXCLUDE
  PATTERN "*~" EXCLUDE
  )


#
# Tests
# -----
add_subdirectory(tests)
