# - Finds Python modules and packages
#
# Defines PY_<module> if the module or package is found.
#
# We assume that you include
#
#     find_package(Python3 REQUIRED)
#
# before finding this package.
#
function(find_python_module module)
  string(TOUPPER ${module} module_upper)
  if(NOT PY_${module_upper})
    if(ARGC GREATER 1 AND ARGV1 STREQUAL "REQUIRED")
      set(${module}_FIND_REQUIRED TRUE)
    endif()
    # A package's location is usually a directory, but for modules
    # it's a .py or .so file.
    execute_process(COMMAND ${RUNNER} "${Python3_EXECUTABLE}" "-c"
      "import re, ${module}; print(re.compile('/__init__.(py.*)|(so)').sub('',${module}.__file__))"
      RESULT_VARIABLE _${module}_status
      OUTPUT_VARIABLE _${module}_location
      ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
      )
    if(NOT _${module}_status)
      set(PY_${module_upper} ${_${module}_location} CACHE STRING
	"Location of Python module ${module}")
    endif(NOT _${module}_status)
  endif(NOT PY_${module_upper})
  find_package_handle_standard_args(PY_${module} DEFAULT_MSG PY_${module_upper})
endfunction(find_python_module)
