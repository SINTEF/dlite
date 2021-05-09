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
find_package(SWIG)
include(UseSWIG)

## We are not ready to support new policies in cmake-3.13 yet...
#if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.14")
#  cmake_policy(SET CMP0078 OLD)
#  cmake_policy(SET CMP0086 OLD)
#  #cmake_policy(SET CMP0079 NEW)
#endif()
#
## Try to require correct version of swig depending on Python version
#if(${PYTHON_VERSION_STRING} VERSION_LESS 3.5)
#  set(REQUIRED_SWIG_VERSION 3.0.0)
#else()
#  set(REQUIRED_SWIG_VERSION 3.0.8)
#endif()
#
## There is a bug in UseSWIG for cmake less than 3.14.  If our version of
## swig is less than 3.14, we use FindSWIG from cmake 3.14 included with
## dlite instead.
#if(CMAKE_VERSION VERSION_LESS 3.14)
#  find_package(SWIG314 ${REQUIRED_SWIG_VERSION} REQUIRED)
#  # SWIG_MODULE_dlite_REAL_NAME is not defined before cmake 3.14
#  set(SWIG_MODULE_dlite_REAL_NAME "_dlite")
#else()
#  find_package(SWIG ${REQUIRED_SWIG_VERSION} REQUIRED)
#endif()
#
#
#
## Add macros for use with swig
#include(UseSWIG)
##include(${SWIG_USE_FILE})

#
# Create Python library
# ---------------------
set(CMAKE_SWIG_OUTDIR ${CMAKE_CURRENT_BINARY_DIR})

include_directories(
  ${PYTHON_INCLUDE_DIRS}
  ${NUMPY_INCLUDE_DIR}
  ${dlite-src_SOURCE_DIR}
  ${dlite-src_BINARY_DIR}
  ${dlite-src-utils_BINARY_DIR}
  )

set_source_files_properties(dlite.i PROPERTIES
  SWIG_FLAGS "-py3;-Wall;-DNPY_SIZE_T=${NPY_SIZE_T}"
  )

swig_add_library(dlite
  TYPE
  LANGUAGE python
  OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/dlite
  OUTFILE_DIR ${CMAKE_CURRENT_BINARY_DIR}
  SOURCES dlite.i
  )

swig_link_libraries(dlite
  dlite-utils
  dlite-pyembed
  dlite
  ${PYTHON_LIBRARIES}
  )


##
## Create a working Python package in binary dir
## ---------------------------------------------
#set(pkgdir ${CMAKE_CURRENT_BINARY_DIR}/dlite)
#set(abs_sources)
#foreach(source ${py_sources})
#  list(APPEND abs_sources ${CMAKE_CURRENT_SOURCE_DIR}/${source})
#endforeach()
#
#set(abs_targets)
#foreach(source ${py_sources})
#  list(APPEND abs_targets ${pkgdir}/${source})
#endforeach()
#
#add_custom_command(
#  TARGET ${SWIG_MODULE_dlite_REAL_NAME}
#  POST_BUILD
#  COMMAND ${CMAKE_COMMAND} -E make_directory ${pkgdir}
#  COMMAND ${CMAKE_COMMAND} -E copy_if_different
#    $<TARGET_FILE:${SWIG_MODULE_dlite_REAL_NAME}>
#    ${CMAKE_CURRENT_BINARY_DIR}/dlite.py
#    ${pkgdir}
#  BYPRODUCTS
#    ${pkgdir}
#    ${pkgdir}/dlite.py
#  )
#
#add_custom_target(python_package ALL DEPENDS ${abs_targets})
#add_custom_command(
#  OUTPUT
#    ${abs_targets}
#  COMMAND ${CMAKE_COMMAND} -E copy_if_different
#    ${abs_sources}
#    ${pkgdir}
#  DEPENDS
#    ${pkgdir}
#    ${abs_sources}
#  )


# Again a workaround for cmake < 3.14...
if(CMAKE_VERSION VERSION_LESS 3.14)
  add_custom_command(
    OUTPUT ${pkgdir}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${pkgdir}
    )
endif()


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
  ${PYTHON_EXECUTABLE} -c "from distutils import sysconfig; print(sysconfig.get_python_lib(plat_specific=True, prefix='${CMAKE_INSTALL_PREFIX}'))"
  OUTPUT_VARIABLE _ABS_PYTHON_MODULE_PATH
  OUTPUT_STRIP_TRAILING_WHITESPACE)
get_filename_component(_ABS_PYTHON_MODULE_PATH
  ${_ABS_PYTHON_MODULE_PATH} ABSOLUTE)
file(RELATIVE_PATH _REL_PYTHON_MODULE_PATH
  ${CMAKE_INSTALL_PREFIX} ${_ABS_PYTHON_MODULE_PATH})
set(PYTHON_MODULE_PATH
  ${_REL_PYTHON_MODULE_PATH}
)

install(
  DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/dlite
  DESTINATION ${PYTHON_MODULE_PATH}
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
