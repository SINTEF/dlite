project(dlite-bindings-python C)

# Python sources
set(py_sources
  __init__.py
  factory.py
  options.py
  utils.py
  )

configure_file(paths.py.in paths.py)

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
# create a target named "dlite" which would be in conflict with the target
# of src/dlite.so.
# TODO: Check if it is possible to rename the target of src/dlite.so
# using a property

# https://cmake.org/cmake/help/latest/module/UseSWIG.html
# Changed in version 3.13: This command creates a target with the specified <name> when policy CMP0078 is set to NEW.
# Otherwise, the legacy behavior will choose a different target name and store it in the SWIG_MODULE_<name>_REAL_NAME variable.

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
  ${Python3_NumPy_INCLUDE_DIRS}
  ${dlite-src_SOURCE_DIR}
  ${dlite-src_BINARY_DIR}
  ${dlite-src-utils_BINARY_DIR}
  )

set_source_files_properties(dlite.i
  PROPERTIES
    SWIG_FLAGS "-py3;-Wall;-DNPY_SIZE_T=${NPY_SIZE_T}"
    SWIG_MODULE_NAME "dlite"
    )

#SET(CMAKE_SKIP_BUILD_RPATH TRUE)
swig_add_library(dlite
  TYPE SHARED
  LANGUAGE python
  OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/dlite
  OUTFILE_DIR ${CMAKE_CURRENT_BINARY_DIR}
  SOURCES dlite.i
  )


# Convince _dlite.so that it may search for whatever libraries it depends on in its own directory
set_target_properties(${SWIG_MODULE_dlite_REAL_NAME} PROPERTIES
  BUILD_RPATH "$ORIGIN/"
  INSTALL_RPATH "$ORIGIN/"
  )

swig_link_libraries(dlite
  dlite-utils
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

list(APPEND abs_sources ${CMAKE_CURRENT_BINARY_DIR}/paths.py)

set(abs_targets)
foreach(source ${py_sources})
  list(APPEND abs_targets ${pkgdir}/${source})
endforeach()

# On windows, also copy all dlite DLLs to our Python package
set(dlls)
if(WINDOWS OR MINGW)
  foreach(dll_target ${dll_targets})
    list(APPEND dlls $<TARGET_FILE:${dll_target}>)
  endforeach()
endif()

add_custom_command(
  TARGET ${SWIG_MODULE_dlite_REAL_NAME}
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory ${pkgdir}
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:${SWIG_MODULE_dlite_REAL_NAME}>
    ${dlls}
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
    dlite-plugins-json
  )

#
# Add the extension-libraries (*.dll, *.so) to the package directory
# effectively producing a binary distribution ready to be run.
# This can be picked up by a setup.py script with ""python setup.py install"" (or bdist_wheel)
#

#add_custom_command(
#  TARGET ${SWIG_MODULE_dlite_REAL_NAME}
#  POST_BUILD
#  COMMAND ${CMAKE_COMMAND} -E make_directory ${pkgdir}/share/dlite/storage-plugins
#  COMMAND ${CMAKE_COMMAND} -E copy_if_different
#    "$<TARGET_FILE:dlite>"
#    ${pkgdir}
#  COMMAND ${CMAKE_COMMAND} -E copy_if_different
#    "$<TARGET_FILE:dlite-utils>"
#    ${pkgdir}
#  COMMAND ${CMAKE_COMMAND} -E copy_if_different
#    "$<TARGET_FILE:dlite-plugins-python>"
#    ${pkgdir}/share/dlite/storage-plugins
#  COMMAND ${CMAKE_COMMAND} -E copy_if_different
#    "$<TARGET_FILE:dlite-plugins-json>"
#    ${pkgdir}/share/dlite/storage-plugins
#  DEPENDS
#    python_package
#  )
#

#
# Build a python wheel from the bdist (optional)
#
# WITH_PYTHON_BUILD_WHEEL=ON
# Generate a Python virtual environment for the base interpreter
# (and eventually for other Python versions with CMake > 3.17.0), Configure a setup.py
# script for testing in a virtual enviroment, build the wheel in the venev and
# (optionally) install the wheel(s) to ./dist
#
# WITH_PYTHON_TEST_WHEEL=ON
# Install the wheel in the venv and run python-tests in the virtual environment.
# Will only install the wheel(s) if WITH_PYTHON_BUILD_WHEEL=ON
#

if(WITH_PYTHON_BUILD_WHEEL)

  find_package(Python3 COMPONENTS Interpreter REQUIRED)

  if(NOT Python3_FOUND)
    message(WARNING "Python executable not found, dlite-python wheel could not be built.")
  else()

    configure_file(setup.py-testenv.in setup.py)

    # Setup a virtual environment
    find_program(VIRTUALENV virtualenv)
    if(NOT VIRTUALENV)
      message(FATAL_ERROR "Could not find `virtualenv` in PATH. pip install virtualenv.")
    endif()
    message(STATUS "Python base Interpreter: ${Python3_VERSION} path=${Python3_EXECUTABLE}")

    # ZIP_LISTS Requires cmake > 3.17
    if(${CMAKE_VERSION} VERSION_LESS "3.17.0")
      message(WARNING "Please consider to switch to CMake > 3.17.0 - Building the wheel only for your base Python version.")
      # CMake > 3.17.0
      #set(python_versions_minor
      #  ${Python3_VERSION_MINOR}
      #)
      #set(python_versions_patch
      #  ${Python3_VERSION_PATCH}
      #)
      #foreach(python3_version_minor python3_version_patch IN ZIP_LISTS python_versions_minor python_versions_patch)
      # ...
    endif()

    set(PYTHON_VENV_VERSION "3.${Python3_VERSION_MINOR}.${Python3_VERSION_PATCH}")
    set(PYTHON_VENV_NAME "venv${PYTHON_VENV_VERSION}")

    if(WIN32)
      set(PYTHON_VENV_PYTHON ${CMAKE_CURRENT_BINARY_DIR}/${PYTHON_VENV_NAME}/Scripts/python)
      set(PYTHON_VENV_PIP ${CMAKE_CURRENT_BINARY_DIR}/${PYTHON_VENV_NAME}/Scripts/pip)
      set(WHEEL_BDIST_EXT win_amd64.whl)
    else()
      set(PYTHON_VENV_PYTHON ${CMAKE_CURRENT_BINARY_DIR}/${PYTHON_VENV_NAME}/bin/python)
      set(PYTHON_VENV_PIP ${CMAKE_CURRENT_BINARY_DIR}/${PYTHON_VENV_NAME}/bin/pip)
      set(WHEEL_BDIST_EXT linux_x86_64.whl)
    endif()

    set(CURRENT_WHEEL_NAME dlite_python-${dlite_VERSION}-cp3${Python3_VERSION_MINOR}-cp3${Python3_VERSION_MINOR}-${WHEEL_BDIST_EXT})

    add_custom_target(dlite-python-wheel3${Python3_VERSION_MINOR} ALL DEPENDS python_package dlite-plugins-python
      COMMAND ${VIRTUALENV} -p python${PYTHON_VERSION} ${PYTHON_VENV_NAME}
      COMMAND ${PYTHON_VENV_PIP} install wheel
      COMMAND ${PYTHON_VENV_PYTHON} ${CMAKE_CURRENT_BINARY_DIR}/setup.py bdist_wheel
    )

    #if(WITH_HDF5)
    #  add_dependencies ?
    #endif()
    #if(HAVE_REDLAND)
    #  add_dependencies ?
    #endif()

    if(WITH_PYTHON_TEST_WHEEL)

      add_custom_target(dlite-python-wheel3${Python3_VERSION_MINOR}_install ALL DEPENDS dlite-python-wheel3${Python3_VERSION_MINOR}
        COMMAND ${PYTHON_VENV_PIP} install ${CMAKE_CURRENT_BINARY_DIR}/dist/${CURRENT_WHEEL_NAME}
      )

      set(tests
      test_python_bindings
      test_collection
      test_entity
      test_factory
      test_misc
      test_python_storage
      test_storage
      )

      foreach(test ${tests})
        set(name ${test}-py_venv_python${PYTHON_VENV_VERSION})
        add_test(
            NAME ${name}
            COMMAND ${RUNNER} ${PYTHON_VENV_PYTHON} ${CMAKE_CURRENT_SOURCE_DIR}/../../bindings/python/tests/${test}.py
            )
      endforeach()

      # More tests in ./examples, add them here

      set(additional_tests
        read-csv
      )

      foreach(test ${additional_tests})
        set(name ${test}-py_venv_python${PYTHON_VENV_VERSION})
        add_test(
            NAME ${name}
            COMMAND ${RUNNER} ${PYTHON_VENV_PYTHON} ${CMAKE_CURRENT_SOURCE_DIR}/../../examples/${test}/main.py
            )
      endforeach()
    endif() # WITH_PYTHON_TEST_WHEEL
  endif() # Python3_FOUND

  if(WITH_PYTHON_BUILD_WHEEL)
     # Skip in case we only have WITH_PYTHON_TEST_WHEEL=ON
      install(
          DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/dist
          DESTINATION .
      )
  endif()
endif() # WITH_PYTHON_BUILD_WHEEL OR WITH_PYTHON_TEST_WHEEL


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
  ${RUNNER} ${Python3_EXECUTABLE} -c "from distutils import sysconfig; print(sysconfig.get_python_lib(plat_specific=True, prefix='${CMAKE_INSTALL_PREFIX}'))"
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
