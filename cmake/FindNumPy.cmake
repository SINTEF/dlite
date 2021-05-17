# - Finds the Python module NumPy
#
# This module defines:
#   Python3_NumPy_FOUND
#   Python3_NumPy_VERSION
#   Python3_NumPy_INCLUDE_DIRS
#   Python3_NumPy_LIBRARY_DIRS
#
# We assume that you include
#
#     find_package(Python3 REQUIRED Interpreter)
#
# before finding this package.  Verbose output can be suppressed by setting
# FIND_NumPy_QUIETLY to TRUE.
#
if(Python3_NumPy_INCLUDE_DIRS AND Python3_NumPy_LIBRARY_DIRS)
  # in cache already
  set(Python3_NumPy_FOUND TRUE)
else()

  if(NOT FIND_NumPy_QUIETLY)
    message(STATUS "Looking for NumPy")
  endif()

  set(script "
import os
from pathlib import Path
import numpy as np

def basepath(path):
    return Path(os.path.splitdrive(path)[1])

incdir = basepath(np.get_include())
coredir = basepath(np.__file__).parent / 'core'

print(np.version.version)
print(incdir.as_posix())
print(coredir.as_posix())
")

  execute_process(
    COMMAND "${Python3_EXECUTABLE}" -c "${script}"
    RESULT_VARIABLE numpy_retval
    OUTPUT_VARIABLE numpy_output
    ERROR_VARIABLE numpy_error
    )

  if(${numpy_retval})
    message(${numpy_error})
  else()
    string(REPLACE "\n" ";" lst ${numpy_output})
    list(GET lst 0 Python3_NumPy_VERSION)
    list(GET lst 1 Python3_NumPy_INCLUDE_DIRS)
    list(GET lst 2 Python3_NumPy_LIBRARY_DIRS)
  endif()

  if(NOT FIND_NumPy_QUIETLY)
    if(Python3_NumPy_VERSION)
      message(STATUS "Looking for NumPy - found version ${Python3_NumPy_VERSION}")
    else()
      message(STATUS "Looking for NumPy - not found")
    endif()
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(NumPy DEFAULT_MSG
    Python3_NumPy_VERSION
    Python3_NumPy_INCLUDE_DIRS
    Python3_NumPy_LIBRARY_DIRS
    )
  mark_as_advanced(
    Python3_NumPy_VERSION
    Python3_NumPy_INCLUDE_DIRS
    Python3_NumPy_LIBRARY_DIRS
    )

endif()
