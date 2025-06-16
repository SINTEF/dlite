# Make platform-specific path names
#
# make_platform_paths(
#     [PREFIX <prefix>]
#     [PATHS <paths_var> [...]]
#     [MULTI_CONFIG_PATHS <multi_paths_var> [...]]
#     [CONFIG_DIRS <name> [...]]
#     [ESCAPE_WIN32_DIRSEP]
#     [PATHSEP0]
# )
#
# Arguments
#   - PREFIX: Optional prefix to prepended to new variables.
#   - PATHS: Name of a variable holding a list of paths.
#         New variables will be created with the following postfixes
#         appended to the variable name:
#           - <var>_NATIVE: paths are converted to native paths
#           - <var>_UNIX: paths are converted to Unix paths
#           - <var>_WINDOWS: paths are converted to Windows paths
#           - <var>_CMAKE: paths are converted to CMake paths
#   - MULTI_CONFIG_PATHS: Like `paths_var` but also appends config
#         directories (specified with CONFIG_DIRS) to the value of
#         `multipaths_var`.
#   - CONFIG_DIRS: Names of config-specific directories appended to
#         each path of the value of `multi_paths_var`.  Defaults to
#         `${CMAKE_CONFIGURATION_TYPES}` for multi-configuration
#         generators and nothing otherwise.
#   - ESCAPE_WIN32_DIRSEP: Whether to escape Windows directory separators.
#         This is typically needed when the created variables are
#         used in configure_file().
#   - PATHSEP0: Whether to create three new variables; `<var>_NATIVE0`,
#         `<var>_UNIX0` and `<var>_WINDOWS0`, for each input variable,
#         with the path separator set to the pipe '|' symbol.
#
function(make_platform_paths)
  cmake_parse_arguments(
    "PP"
    "ESCAPE_WIN32_DIRSEP;PATHSEP0"
    "PREFIX"
    "PATHS;MULTI_CONFIG_PATHS;CONFIG_DIRS"
    ${ARGN}
    )

  set(prefix "${PP_PREFIX}")
  set(paths_vars "${PP_PATHS}")
  set(multi_paths_vars "${PP_MULTI_CONFIG_PATHS}")
  set(names "${PP_CONFIG_DIRS}")
  if(PP_ESCAPE_WIN32_DIRSEP)
    set(win_dirsep "\\\\")
  else()
    set(win_dirsep "\\")
  endif()

  if(NOT names AND GENERATOR_IS_MULTI_CONFIG)
    set(names "${CMAKE_CONFIGURATION_TYPES}")
  endif()

  set(conversions NATIVE UNIX WINDOWS CMAKE)

  # Create new variables for platform-specific paths
  set(vars ${paths_vars} ${multi_paths_vars})
  foreach(var ${vars})
    list(REMOVE_DUPLICATES ${var})

    list(TRANSFORM "${var}" REPLACE "^([a-zA-Z]):[/\\]" "/\\1/" OUTPUT_VARIABLE ${var}_UNIX) # C:\other/sub -> /C/other/sub
    string(REPLACE "\\" "/" ${var}_UNIX "${${var}_UNIX}")
    string(REPLACE ";" ":" ${var}_UNIX "${${var}_UNIX}")

    list(TRANSFORM "${var}" REPLACE "^/([a-zA-Z])/" "\\1:\\\\" OUTPUT_VARIABLE ${var}_WINDOWS) # /C/other/sub -> C:\other/sub
    string(REPLACE "/" "${win_dirsep}" ${var}_WINDOWS "${${var}_WINDOWS}")

    # If compiling using Windows, could be using bash or other Unix shells or generators which would want native Unix paths
    # The SHELL_PATH genex appears to deal with the Windows to Unix drive conversion for MSYS2
    # if(WIN32 AND NOT ${NATIVE_PATH_TYPE_UNIX})
    if(WIN32)
      set(${var}_NATIVE "${${var}_WINDOWS}")
    else()
      set(${var}_NATIVE "${${var}_UNIX}")
    endif()
    cmake_path(CONVERT "${${var}_NATIVE}" TO_CMAKE_PATH_LIST ${var}_CMAKE)
  endforeach()

  # Append config dirs to `multi_paths_vars`
  # TODO: Does the list need ; changed to : on Unix? CONFIG_DIRS is not currently used
  foreach(var ${multi_paths_vars})
    foreach(conv ${conversions})
      foreach(name ${names})
        list(APPEND ${var}_${conv} "${${var}_${conv}}/${name}")
      endforeach()
    endforeach()
  endforeach()

  # Assign all new variables in parent scope
  foreach(var ${vars})
    foreach(conv ${conversions})
      set(${prefix}${var}_${conv} "${${var}_${conv}}" PARENT_SCOPE)
      #message("*** adding ${var}_${conv}=${${var}_${conv}}")  # XXX
    endforeach()
  endforeach()

  # PATHSEP0
  if(PP_PATHSEP0)
    foreach(var ${vars})
      string(REPLACE ":" "|" ${var}_UNIX0 "${${var}_UNIX}")
      string(REPLACE ";" "|" ${var}_WINDOWS0 "${${var}_WINDOWS}")
      set(${prefix}${var}_UNIX0 "${${var}_UNIX0}" PARENT_SCOPE)
      set(${prefix}${var}_WINDOWS0 "${${var}_WINDOWS0}" PARENT_SCOPE)
      if(WIN32)
        set(${prefix}${var}_NATIVE0 "${${var}_WINDOWS0}" PARENT_SCOPE)
      else()
        set(${prefix}${var}_NATIVE0 "${${var}_UNIX0}" PARENT_SCOPE)
      endif()

      # XXX
      #foreach(conv ${conversions})
      #  message("*** adding ${var}_${conv}0=${${var}_${conv}0}")
      #endforeach()

    endforeach()
  endif()

endfunction()
