cmake_minimum_required(VERSION 3.7)
# - Check if we are running anaconda python
#   https://stackoverflow.com/questions/47608532/how-to-detect-from-within-python-whether-packages-are-managed-with-conda
#
# We assume that you include
#
#  include(FindPythonAnaconda)
#
# Once done this will define
#
#  PYTHON_IS_ANACONDA      - True in case we are running anaconda Python
#  PYTHON_IS_ANACONDA_BASE - True in case we are running anaconda Python and are using the base environment
#

  set(script "
import sys, os
try:
  import conda
except:
  is_conda_base = False
  is_conda = os.path.exists(os.path.join(sys.prefix, 'conda-meta'))
else:
  assert os.path.exists(os.path.join(sys.prefix, 'conda-meta'))
  is_conda_base = True
  is_conda = True

print(is_conda_base)
print(is_conda)
")

  execute_process(
    COMMAND ${RUNNER} "${Python3_EXECUTABLE}" -c "${script}"
    RESULT_VARIABLE Python_is_anaconda_retval
    OUTPUT_VARIABLE Python_is_anaconda_output
    ERROR_VARIABLE Python_is_anaconda_error
    )

  if(${Python_is_anaconda_retval})
    message(${Python_is_anaconda_error})
  else()
    string(REPLACE "\n" ";" lst ${Python_is_anaconda_output})
    list(GET lst 0 PYTHON_IS_ANACONDA_BASE)
    list(GET lst 1 PYTHON_IS_ANACONDA)
  endif()
