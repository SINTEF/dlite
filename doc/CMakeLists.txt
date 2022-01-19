find_package(Doxygen REQUIRED)
project(dlite-doc)

if(DOXYGEN_FOUND)

  # Fix links in README.md such that doxygen understand them.
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/README.md
    COMMAND ${CMAKE_COMMAND}
      -Dinfile=${dlite_SOURCE_DIR}/README.md
      -Doutfile=${CMAKE_CURRENT_BINARY_DIR}/README.md
      -P ${CMAKE_CURRENT_SOURCE_DIR}/fix_readme.cmake
    MAIN_DEPENDENCY ${dlite_SOURCE_DIR}/README.md
    )

  set(dependencies
    ${CMAKE_CURRENT_BINARY_DIR}/README.md
    #${dlite_SOURCE_DIR}/README.md
    ${dlite_SOURCE_DIR}/doc/Doxyfile.in
    ${dlite_SOURCE_DIR}/doc/concepts.md
    ${dlite_SOURCE_DIR}/doc/SOFT-metadata-structure.png

    ${dlite_SOURCE_DIR}/src/dlite.h
    ${dlite_SOURCE_DIR}/src/dlite-misc.h
    ${dlite_SOURCE_DIR}/src/dlite-store.h
    ${dlite_SOURCE_DIR}/src/dlite-type.h
    ${dlite_SOURCE_DIR}/src/dlite-arrays.h
    ${dlite_SOURCE_DIR}/src/dlite-datamodel.h
    ${dlite_SOURCE_DIR}/src/dlite-schemas.h
    ${dlite_SOURCE_DIR}/src/dlite-entity.h
    ${dlite_SOURCE_DIR}/src/dlite-collection.h
    ${dlite_SOURCE_DIR}/src/dlite-storage.h
    ${dlite_SOURCE_DIR}/src/dlite-storage-plugins.h
    ${dlite_SOURCE_DIR}/src/dlite-mapping.h
    ${dlite_SOURCE_DIR}/src/dlite-mapping-plugins.h
    ${dlite_SOURCE_DIR}/src/getuuid.h
    ${dlite_SOURCE_DIR}/src/triplestore.h
    ${dlite_SOURCE_DIR}/src/triple.h

    ${dlite_SOURCE_DIR}/src/utils/compat.h
    ${dlite_SOURCE_DIR}/src/utils/dsl.h
    ${dlite_SOURCE_DIR}/src/utils/err.h
    ${dlite_SOURCE_DIR}/src/utils/fileutils.h
    ${dlite_SOURCE_DIR}/src/utils/globmatch.h
    ${dlite_SOURCE_DIR}/src/utils/map.h
    ${dlite_SOURCE_DIR}/src/utils/plugin.h
    ${dlite_SOURCE_DIR}/src/utils/strtob.h
    ${dlite_SOURCE_DIR}/src/utils/tgen.h
    ${dlite_SOURCE_DIR}/src/utils/tmpfileplus.h
    ${dlite_SOURCE_DIR}/src/utils/infixcalc.h
    ${dlite_SOURCE_DIR}/src/utils/uuid.h
    ${dlite_SOURCE_DIR}/src/utils/tests/tgen_example.c

    ${dlite_SOURCE_DIR}/storages/python/README.md
    ${dlite_SOURCE_DIR}/storages/python/python-storage-plugins/README.md
    ${dlite_SOURCE_DIR}/src/tests/minunit/README.md
    )

  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
    ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)

  add_custom_command(OUTPUT html/index.html
    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    COMMAND ${CMAKE_COMMAND} -E make_directory html/src
    COMMAND ${CMAKE_COMMAND} -E copy ${dlite_SOURCE_DIR}/src/dlite-storage.h html/src/dlite-storage.h
    COMMAND ${CMAKE_COMMAND} -E copy ${dlite_SOURCE_DIR}/src/dlite-entity.h html/src/dlite-entity.h
    COMMAND ${CMAKE_COMMAND} -E copy ${dlite_SOURCE_DIR}/src/dlite-type.h html/src/dlite-type.h
    COMMAND ${CMAKE_COMMAND} -E copy ${dlite_SOURCE_DIR}/doc/SOFT-metadata-structure.png html/SOFT-metadata-structure.png
    COMMAND ${CMAKE_COMMAND} -E make_directory html/python-storage-plugins
    COMMAND ${CMAKE_COMMAND} -E copy ${dlite_SOURCE_DIR}/storages/python/python-storage-plugins/yaml.py html/python-storage-plugins/yaml.py
    DEPENDS ${dependencies}
    #BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/xml/index.xml
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM
    )

  # Unfortunately dependencies for build-in target "install" is not
  # yet supported.  When that is supported, we can remove ALL from
  # add_custom_target() and uncommend add_dependencies().
  add_custom_target(doc ALL
    DEPENDS html/index.html
    )

  #add_dependencies(install doc)

  install(
    DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html
    DESTINATION share/dlite
    )

endif(DOXYGEN_FOUND)
