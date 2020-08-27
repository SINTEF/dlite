# Generates a script or header with dlite environment
#
# Arguments
#   - filename: Name of generated filename.  The input file is expected to
#         have '.in' appended to it.
#   - output_dir: Where to place the generated script.
#   - newline_style: Either "UNIX" (LF), "DOS" (CRLF) or NATIVE
#
function(dlite_genenv filename output_dir newline_style)

  get_filename_component(basename ${filename} NAME)

  if(IS_ABSOLUTE ${filename})
    set(input ${filename}.in)
  else()
    set(input ${CMAKE_CURRENT_SOURCE_DIR}/${filename}.in)
  endif()
  set(output ${output_dir}/${basename})
  set(script ${dlite_SOURCE_DIR}/cmake/dliteGenEnv-sub.cmake)

  if(newline_style STREQUAL "NATIVE")
    if(WINDOWS)
      set(newline_style "CRLF")
    else()
      set(newline_style "LF")
    endif()
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
  string(REPLACE ";" "|" TEMPLATES              "${dlite_TEMPLATES}")


  add_custom_command(
    OUTPUT ${output}
    COMMAND
      ${CMAKE_COMMAND}
        -Dinput=${input}
        -Doutput=${output}
        -Dnewline_style=${newline_style}
        -DPATH="${PATH}"
        -DLD_LIBRARY_PATH="${LD_LIBRARY_PATH}"
        -DPYTHONPATH="${PYTHONPATH}"
        -Ddlite_SOURCE_DIR="${dlite_SOURCE_DIR}"
        -Ddlite_BINARY_DIR="${dlite_BINARY_DIR}"
        -DSTORAGE_PLUGINS="${STORAGE_PLUGINS}"
        -DMAPPING_PLUGINS="${MAPPING_PLUGINS}"
        -DPYTHON_STORAGE_PLUGINS="${PYTHON_STORAGE_PLUGINS}"
        -DPYTHON_MAPPING_PLUGINS="${PYTHON_MAPPING_PLUGINS}"
        -DTEMPLATES="${TEMPLATES}"
        -DSTORAGES="${dlite_STORAGES}"
        -P ${script}
    DEPENDS ${input} ${script}
    COMMENT "Generate ${output}"
  )

  add_custom_target(${basename}
    DEPENDS ${output}
    )

endfunction()
