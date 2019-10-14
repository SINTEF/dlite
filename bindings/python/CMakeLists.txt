project(dlite-bindings-python C)

# We are not ready to support new policies in cmake-3.13 yet...
if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.14")
  cmake_policy(SET CMP0078 OLD)
  cmake_policy(SET CMP0086 OLD)
endif()


# Try to require correct version of swig depending on Python version
if(${PYTHON_VERSION_STRING} VERSION_LESS 3.2)
  find_package(SWIG REQUIRED 1.3.37)
elseif(${PYTHON_VERSION_STRING} VERSION_LESS 3.3)
  find_package(SWIG REQUIRED 2.0.4)
elseif(${PYTHON_VERSION_STRING} VERSION_LESS 3.5)
  find_package(SWIG REQUIRED 3.0.0)
else()
  find_package(SWIG REQUIRED 3.0.8)
endif()

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
include(${SWIG_USE_FILE})


#
# Create Python library
# ---------------------
set(CMAKE_SWIG_OUTDIR ${CMAKE_CURRENT_BINARY_DIR})

include_directories(
  ${PYTHON_INCLUDE_DIRS}
  ${NUMPY_INCLUDE_DIR}
  #${CMAKE_CURRENT_SOURCE_DIR}
  ${dlite-src_SOURCE_DIR}
  ${dlite-src_BINARY_DIR}
  ${dlite-src-utils_BINARY_DIR}
  )

set_source_files_properties(dlite.i PROPERTIES
  SWIG_FLAGS "-py3;-Wall;-DNPY_SIZE_T=${NPY_SIZE_T}"
  )

swig_add_library(dlite
  LANGUAGE python
  #OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/dlite   # new in CMake 3.12
  SOURCES dlite.i
  )

swig_link_libraries(dlite
  dlite
  dlite-utils
  dlite-pyembed
  ${PYTHON_LIBRARIES}
  )


#
# Create a working Python package in binary dir
# ---------------------------------------------
set(pkgdir ${CMAKE_CURRENT_BINARY_DIR}/dlite)
set(abs_sources)
foreach(source ${py_sources})
  list(APPEND abs_sources ${CMAKE_CURRENT_SOURCE_DIR}/${source})
endforeach()

set(abs_targets)
foreach(source ${py_sources})
  list(APPEND abs_targets ${pkgdir}/${source})
endforeach()

add_custom_target(python_package ALL
  DEPENDS
    ${pkgdir}/dlite.py
    ${abs_targets}
  )

add_custom_command(
  TARGET ${SWIG_MODULE_dlite_REAL_NAME}
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory
    ${pkgdir}
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:${SWIG_MODULE_dlite_REAL_NAME}>
    ${CMAKE_CURRENT_BINARY_DIR}/dlite.py
    ${pkgdir}
  BYPRODUCTS
    ${pkgdir}
    ${pkgdir}/dlite.py
  DEPENDS
    $<TARGET_FILE:${SWIG_MODULE_dlite_REAL_NAME}>
    ${CMAKE_CURRENT_BINARY_DIR}/dlite.py
    ${abs_sources}
  )

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

#add_custom_command(
#  TARGET ${SWIG_MODULE_dlite_REAL_NAME}
#  POST_BUILD
#  COMMAND ${CMAKE_COMMAND} -E make_directory
#    ${CMAKE_CURRENT_BINARY_DIR}/dlite
#  COMMAND ${CMAKE_COMMAND} -E copy_if_different
#    $<TARGET_FILE:${SWIG_MODULE_dlite_REAL_NAME}>
#    ${CMAKE_CURRENT_BINARY_DIR}/dlite.py
#    ${abs_sources}
#    ${CMAKE_CURRENT_BINARY_DIR}/dlite
#  DEPENDS
#    $<TARGET_FILE:${SWIG_MODULE_dlite_REAL_NAME}>
#    ${CMAKE_CURRENT_BINARY_DIR}/dlite.py
#    ${abs_sources}
#  )

# Copy to binary dir again...
#file(COPY ${py_sources} DESTINATION dlite)




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
  FILES ${py_sources}
  DESTINATION ${PYTHON_MODULE_PATH}
)


#
# Tests
# -----
add_subdirectory(tests)
