find_package(Sphinx REQUIRED)
find_package(Doxygen REQUIRED)
project(dlite-pydoc)

# configured documentation tools and intermediate build results
set(BINARY_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/_build")

# Sphinx cache with pickled ReST documents
set(SPHINX_CACHE_DIR "${CMAKE_CURRENT_BINARY_DIR}/_doctrees")

# HTML output directory
set(SPHINX_HTML_DIR "${CMAKE_CURRENT_BINARY_DIR}/html")

set(SPHINX_INDEX_FILE "${SPHINX_HTML_DIR}/index.html")

# Doxygen intput source directory
set(DOXYGEN_INPUT_DIRS
  ${dlite_SOURCE_DIR}/src/utils
  ${dlite_SOURCE_DIR}/src/utils/compat-src
  ${dlite_SOURCE_DIR}/src
  ${dlite_SOURCE_DIR}/src/pyembed
  ${dlite_SOURCE_DIR}/src/tests/minunit
)
string(REPLACE ";" " " DOXYGEN_INPUT "${DOXYGEN_INPUT_DIRS}")


# Doxygen output directory
set(DOXYGEN_OUTPUT_XML_DIR "${CMAKE_CURRENT_BINARY_DIR}/xml")

set(DOXYGEN_DOXYFILE_IN "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in")
set(DOXYGEN_DOXYFILE_OUT "${BINARY_BUILD_DIR}/Doxyfile")

set(DOXYGEN_INDEX_FILE ${DOXYGEN_OUTPUT_XML_DIR}/index.xml)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/conf.py.in"
    "${BINARY_BUILD_DIR}/conf.py"
    @ONLY)

file(COPY
  "${CMAKE_SOURCE_DIR}/LICENSE"
  "${CMAKE_CURRENT_SOURCE_DIR}/license.md"
  "${CMAKE_CURRENT_SOURCE_DIR}/index.rst"
  "${CMAKE_CURRENT_SOURCE_DIR}/api.rst"
  "${CMAKE_CURRENT_SOURCE_DIR}/respirator.py"
  "${CMAKE_CURRENT_SOURCE_DIR}/_templates"
  "${CMAKE_CURRENT_SOURCE_DIR}/_static"
  "${CMAKE_CURRENT_SOURCE_DIR}/contributors_guide"
  "${CMAKE_CURRENT_SOURCE_DIR}/getting_started"
  "${CMAKE_CURRENT_SOURCE_DIR}/user_guide"
  DESTINATION
  "${BINARY_BUILD_DIR}"
)

configure_file(
    ${DOXYGEN_DOXYFILE_IN}
    ${DOXYGEN_DOXYFILE_OUT}
    @ONLY)


# Doxygen command
add_custom_command(OUTPUT ${DOXYGEN_INDEX_FILE}
                   DEPENDS ${DLITE_PUBLIC_HEADERS}
                   COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_OUT}
                   MAIN_DEPENDENCY ${DOXYFILE_OUT}
		   WORKING_DIRECTORY ${BINARY_BUILD_DIR}
                   COMMENT "Generating docs"
                   VERBATIM)

# Named constom target
add_custom_target(Doxygen ALL DEPENDS ${DOXYGEN_INDEX_FILE})

# Sphinx command
add_custom_command(OUTPUT ${SPHINX_INDEX_FILE}
  COMMAND
  ${SPHINX_EXECUTABLE}
        -q -b html
        -c "${BINARY_BUILD_DIR}"
        -d "${SPHINX_CACHE_DIR}"
        ${BINARY_BUILD_DIR}
        "${SPHINX_HTML_DIR}"
  DEPENDS
  ${DOXYGEN_INDEX_FILE}
  ${BINARY_BUILD_DIR}/respirator.py
  MAIN_DEPENDENCY ${BINARY_BUILD_DIR}/conf.py
  COMMENT "Building HTML documentation with Sphinx")

add_custom_target(Sphinx ALL DEPENDS ${SPHINX_INDEX_FILE})
