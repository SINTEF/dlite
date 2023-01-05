find_package(Sphinx REQUIRED)
find_package(Doxygen REQUIRED)

file(GLOB_RECURSE DLITE_PUBLIC_HEADERS ${dlite_SOURCE_DIR}/src/*.h)

set(SPHINX_SOURCE ${CMAKE_CURRENT_SOURCE_DIR})
set(SPHINX_BUILD ${CMAKE_CURRENT_BINARY_DIR}/docs/sphinx)

set(DOXYGEN_INPUT_DIR ${dlite_SOURCE_DIR}/src)
set(DOXYGEN_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/doxygen)
set(DOXYGEN_INDEX_FILE ${DOXYGEN_OUTPUT_DIR}/xml/index.xml)
set(DOXYFILE_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
set(DOXYFILE_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)

file(MAKE_DIRECTORY ${DOXYGEN_OUTPUT_DIR})

add_custom_command(OUTPUT ${DOXYGEN_INDEX_FILE}
                   DEPENDS ${DLITE_PUBLIC_HEADERS}
                   COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_OUT}
                   MAIN_DEPENDENCY ${DOXYFILE_OUT} ${DOXYFILE_IN}
                   COMMENT "Generating docs"
                   VERBATIM)

# Nice named target so we can run the job easily
add_custom_target(Doxygen ALL DEPENDS ${DOXYGEN_INDEX_FILE})

set(SPHINX_SOURCE ${CMAKE_CURRENT_SOURCE_DIR})
set(SPHINX_BUILD ${CMAKE_CURRENT_BINARY_DIR}/sphinx)
set(SPHINX_INDEX_FILE ${SPHINX_BUILD}/index.html)


add_custom_command(OUTPUT ${SPHINX_INDEX_FILE}
                   COMMAND
                     ${SPHINX_EXECUTABLE} -b html
                     # Tell Breathe where to find the Doxygen output
                     -Dbreathe_projects.dlite=${DOXYGEN_OUTPUT_DIR}/xml
                   ${SPHINX_SOURCE} ${SPHINX_BUILD}
                   WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                   DEPENDS
                   # Other docs files you want to track should go here (or in some variable)
                   ${SPHINX_SOURCE}/**/*
                   ${DOXYGEN_INDEX_FILE}
                   MAIN_DEPENDENCY ${SPHINX_SOURCE}/conf.py
                   COMMENT "Generating documentation with Sphinx")

add_custom_target(Sphinx ALL DEPENDS ${SPHINX_INDEX_FILE})

# Add an install target to install the docs
include(GNUInstallDirs)
install(DIRECTORY ${SPHINX_BUILD}
DESTINATION ${CMAKE_INSTALL_DOCDIR})
