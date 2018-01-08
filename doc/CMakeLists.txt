find_package(Doxygen)

if (DOXYGEN_FOUND)
  set(dependencies "")

  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
    ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)

  add_custom_command(OUTPUT html/index.html
    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    COMMAND ${CMAKE_COMMAND} -E make_directory html/src
    COMMAND ${CMAKE_COMMAND} -E copy ${dlite_SOURCE_DIR}/src/dlite.h html/src/dlite.h
    DEPENDS ${dependencies}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating API documentation with Doxygen" VERBATIM
    )

  add_custom_target(doc
    DEPENDS html/index.html
    )

endif (DOXYGEN_FOUND)
