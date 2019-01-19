# -- Macro for generate .def files for MSVC
#
# generate_def(target sources defFile)
#
#   Generate .def file for MSVC.
#
# Arguments:
#   target: name of target library for which the .def files should be
#           created
#   sources: name of source files
#   defFile: variable that will be assigned to the generated .def file
#
if(MSVC)
  include(FindPythonInterp)
  find_package(PythonInterp)
endif()


function(generate_def target sources defFile)

  if(MSVC)
    set(output ${CMAKE_CURRENT_BINARY_DIR}/${target}.def)
    set(xmlfile ${dlite_BINARY_DIR}/doc/xml/index.xml)
    set(output ${CMAKE_CURRENT_BINARY_DIR}/${target}.def)
    add_custom_command(
      OUTPUT ${output}
      COMMAND
        ${PYTHON_EXECUTABLE} ${dlite_SOURCE_DIR}/cmake/GenerateDef.py
        --output ${output} ${target} ${xmlfile} ${sources}
      MAIN_DEPENDENCY ${dlite_SOURCE_DIR}/cmake/GenerateDef.py
      DEPENDS
        ${xmlfile}
        ${dlite_SOURCE_DIR}/cmake/GenerateDef.cmake
        COMMENT "Generating ${target}.def"
      )
    set(${defFile} "${output}" PARENT_SCOPE)
  endif()

endfunction()
