# -- Macro for generate .def files for MSVC
#
# generate_defs(target sources defsVar)
#
#   Converts *.def.in files in source dir to *.def files in binary dir.
#
# Arguments:
#   target: name of target library for which the .def files should be
#           created
#   sources: name of source files
#   defsVar: variable that will be assigned to the generated files
#
macro(generate_defs target sources varName)

  set(defs )
  if(MSVC)
    set(scriptfile "${CMAKE_CURRENT_BINARY_DIR}/gen_deps.cmake")
    file(WRITE ${scriptfile} "configure_file(\${infile} \${outfile})")
    foreach(source ${sources})
      get_filename_component(basename ${source} NAME_WE)
      set(infile "${CMAKE_CURRENT_SOURCE_DIR}/${basename}.def.in")
      set(outfile "${CMAKE_CURRENT_BINARY_DIR}/${basename}.def")
      if(EXISTS ${infile})
        add_custom_command(
          OUTPUT ${outfile}
          COMMAND ${CMAKE_COMMAND}
            -DLIBRARY=$<TARGET_FILE_NAME:${target}>
            -Dinfile=${infile}
            -Doutfile=${outfile}
            -P ${scriptfile}
          DEPENDS
            ${scriptfile}
            ${infile}
          COMMENT "Generating ${basename}.def"
          )
        list(APPEND defs ${outfile})
      endif()
    endforeach()
  endif()

  set(${defsVar} "${defs}" PARENT_SCOPE)

endmacro()
