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
  message(STATUS "-- Checking for NumPy")
endif()

set(script "
import os
from pathlib import Path
import numpy as np

def basepath(path):
    return Path(os.path.splitdrive(path)[1])

incdir = basepath(np.get_include())
coredir = basepath(np.__file__).parent / 'core'

print(incdir.as_posix())
print(np.version.version)
print(coredir.as_posix())
")

execute_process(
  COMMAND "${PYTHON_EXECUTABLE}" -c "${script}"
  RESULT_VARIABLE numpy_retval
  OUTPUT_VARIABLE numpy_output
  ERROR_VARIABLE numpy_error
  )

if(${numpy_retval})
  set(NUMPY_FOUND FALSE)
  message(${numpy_error})
else()
  set(NUMPY_FOUND TRUE)
  string(REPLACE "\n" ";" numpy_output ${numpy_output})
  list(GET numpy_output 0 NUMPY_INCLUDE_DIR)
  list(GET numpy_output 1 NUMPY_VERSION)
  list(GET numpy_output 2 NUMPY_LIBRARY_DIR)
endif()

#message("*** NUMPY_INCLUDE_DIR ${NUMPY_INCLUDE_DIR}")
#message("*** NUMPY_VERSION ${NUMPY_VERSION}")
#message("*** NUMPY_LIBRARY_DIR ${NUMPY_LIBRARY_DIR}")

if(NOT FIND_NUMPY_QUIETLY)
  if(NUMPY_FOUND)
    message(STATUS "-- Checking for NumPy - found version ${NUMPY_VERSION}")
  else()
    message(STATUS "-- Checking for NumPy - not found")
  endif()
endif()


include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
  NumPy DEFAULT_MSG NUMPY_VERSION NUMPY_INCLUDE_DIR NUMPY_LIBRARY_DIR)
mark_as_advanced(NUMPY_FOUND NUMPY_VERSION NUMPY_INCLUDE_DIR NUMPY_LIBRARY_DIR)
