# -- Functions for enabling additional compiler flag if it is supported
#
# Provided functions
#   enable_c_compiler_flag_if_supported(flag)
#   enable_cxx_compiler_flag_if_supported(flag)
#   enable_fortran_compiler_flag_if_supported(flag)
#

if(SetCompilerFlagsIncluded)
  return()
endif()
set(SetCompilerFlagsIncluded TRUE)


include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
include(CheckFortranCompilerFlag)


function(enable_c_compiler_flag_if_supported flag)
  string(FIND "${CMAKE_C_FLAGS}" "${flag}" flag_already_set)
  if(flag_already_set EQUAL -1)
    check_c_compiler_flag("${flag}" flag_supported)
    if(flag_supported)
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${flag}" PARENT_SCOPE)
    endif()
  endif()
endfunction()


function(enable_cxx_compiler_flag_if_supported flag)
  string(FIND "${CMAKE_CXX_FLAGS}" "${flag}" flag_already_set)
  if(flag_already_set EQUAL -1)
    check_cxx_compiler_flag("${flag}" flag_supported)
    if(flag_supported)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}" PARENT_SCOPE)
    endif()
  endif()
endfunction()


function(enable_fortran_compiler_flag_if_supported flag)
  string(FIND "${CMAKE_Fortran_FLAGS}" "${flag}" flag_already_set)
  if(flag_already_set EQUAL -1)
    check_fortran_compiler_flag("${flag}" flag_supported)
    if(flag_supported)
      set(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} ${flag}" PARENT_SCOPE)
    endif()
  endif()
endfunction()
