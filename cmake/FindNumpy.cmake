# - Finds the Python module NumPy
#
# This module defines:
#   NUMPY_INCLUDE_DIR    include path for arrayobject.h
#   NUMPY_VERSION        numpy version string
#   NUMPY_FOUND          whether NumPy headers are found
#
# We assume that you include
#
#     find_package(PythonInterp REQUIRED)
#     find_package(PythonLibs REQUIRED)
#
# before finding this package.  Verbose output can be suppressed by setting
# FIND_NUMPY_QUIETLY to TRUE.
#


if(NOT FIND_NUMPY_QUIETLY)
  message("-- Checking for NumPy")
endif()

execute_process(
  COMMAND "${PYTHON_EXECUTABLE}" -c "import numpy; print(numpy.get_include()); print(numpy.version.version)"
  OUTPUT_VARIABLE numpy_output
  ERROR_VARIABLE numpy_error)

if(numpy_error)
  set(NUMPY_FOUND FALSE)
else()
  set(NUMPY_FOUND TRUE)
  string(REPLACE "\n" ";" numpy_output ${numpy_output})
  list(GET numpy_output 0 NUMPY_INCLUDE_DIR)
  list(GET numpy_output 1 NUMPY_VERSION)
endif()

if(NOT FIND_NUMPY_QUIETLY)
  if(NUMPY_FOUND)
    message("-- Checking for NumPy - found version ${NUMPY_VERSION}")
  else()
    message("-- Checking for NumPy - not found")
  endif()
endif()


include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
  NumPy DEFAULT_MSG NUMPY_VERSION NUMPY_INCLUDE_DIR)
mark_as_advanced(INCLUDE_FOUND NUMPY_VERSION NUMPY_INCLUDE_DIR)
