# Generates a script or header with dlite environment
#
# Arguments
#   - output: Name of generated filename.  The input file is expected to
#         have '.in' appended to it.
#   - newline_style: Either "UNIX" (LF), "DOS" (CRLF) or NATIVE
#
function(dlite_genenv output newline_style)

  get_filename_component(filename ${output} NAME)

  set(input ${CMAKE_CURRENT_SOURCE_DIR}/${filename}.in)
  set(script ${dlite_SOURCE_DIR}/cmake/dliteGenEnv-sub.cmake)

  if(${newline_style} STREQUAL "NATIVE")
    if(WIN32)
      set(_newline_style "CRLF")
    else()
      set(_newline_style "LF")
    endif()
  else()
    set(_newline_style ${newline_style})
  endif()


  # Convert path lists to bar-separated strings - otherwise
  # add_custom_command() will convert them to space, making it
  # impossible to distinguish them from spaces embedded in paths...
  string(REPLACE ";" "|" PATH                   "${dlite_PATH}")
  string(REPLACE ";" "|" LD_LIBRARY_PATH        "${dlite_LD_LIBRARY_PATH}")
  string(REPLACE ";" "|" PYTHONPATH             "${dlite_PYTHONPATH}")
  string(REPLACE ";" "|" STORAGE_PLUGINS        "${dlite_STORAGE_PLUGINS}")
  string(REPLACE ";" "|" MAPPING_PLUGINS        "${dlite_MAPPING_PLUGINS}")
  string(REPLACE ";" "|" PYTHON_STORAGE_PLUGINS "${dlite_PYTHON_STORAGE_PLUGINS}")
  string(REPLACE ";" "|" PYTHON_MAPPING_PLUGINS "${dlite_PYTHON_MAPPING_PLUGINS}")
  string(REPLACE ";" "|" PYTHON_PROTOCOL_PLUGINS "${dlite_PYTHON_PROTOCOL_PLUGINS}")
  string(REPLACE ";" "|" TEMPLATES              "${dlite_TEMPLATES}")


  add_custom_command(
    OUTPUT ${output}
    COMMAND
      ${CMAKE_COMMAND}
        -Dinput=${input}
        -Doutput=${output}
        -Dnewline_style=${_newline_style}
        -DCMAKE_INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}"
        -Ddlite_SOURCE_DIR="${dlite_SOURCE_DIR}"
        -Ddlite_BINARY_DIR="${dlite_BINARY_DIR}"
        -Ddlite_PATH="${PATH}"
        -Ddlite_PATH_EXTRA="${dlite_PATH_EXTRA}"
        -Ddlite_LD_LIBRARY_PATH="${LD_LIBRARY_PATH}"
        -Ddlite_PYTHONPATH="${PYTHONPATH}"
        -Ddlite_STORAGE_PLUGINS="${STORAGE_PLUGINS}"
        -Ddlite_MAPPING_PLUGINS="${MAPPING_PLUGINS}"
        -Ddlite_PYTHON_STORAGE_PLUGINS="${PYTHON_STORAGE_PLUGINS}"
        -Ddlite_PYTHON_MAPPING_PLUGINS="${PYTHON_MAPPING_PLUGINS}"
        -Ddlite_PYTHON_PROTOCOL_PLUGINS="${PYTHON_PROTOCOL_PLUGINS}"
        -Ddlite_TEMPLATES="${TEMPLATES}"
        -Ddlite_STORAGES="${dlite_STORAGES}"
        -P ${script}
    DEPENDS ${input} ${script}
    COMMENT "Generate ${output}"
  )

endfunction()
