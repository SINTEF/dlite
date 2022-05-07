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
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/fix_readme.cmake
    )

  set(dependencies
    ${CMAKE_CURRENT_BINARY_DIR}/README.md
    ${CMAKE_CURRENT_SOURCE_DIR}/fix_index.py

    ${dlite_SOURCE_DIR}/cmake/README.md
    ${dlite_SOURCE_DIR}/examples/ex1/README.md
    ${dlite_SOURCE_DIR}/examples/ex2/README.md
    ${dlite_SOURCE_DIR}/examples/read-csv/README.md
    ${dlite_SOURCE_DIR}/examples/ex3/README.md
    ${dlite_SOURCE_DIR}/examples/ex4/README.md
    ${dlite_SOURCE_DIR}/examples/ex5d/README.md
    ${dlite_SOURCE_DIR}/examples/dehydrogenation/README.md
    ${dlite_SOURCE_DIR}/examples/mappings/README.md
    ${dlite_SOURCE_DIR}/src/tests/minunit/README.md
    ${dlite_SOURCE_DIR}/src/utils/README.md
    ${dlite_SOURCE_DIR}/src/pyembed/README.md
    ${dlite_SOURCE_DIR}/storages/python/README.md
    ${dlite_SOURCE_DIR}/storages/python/python-storage-plugins/README.md
    ${dlite_SOURCE_DIR}/storages/python/tests-python/README.md

    ${dlite_SOURCE_DIR}/doc/Doxyfile.in
    ${dlite_SOURCE_DIR}/doc/SOFT-metadata-structure.png
    ${dlite_SOURCE_DIR}/doc/build_with_vs.md
    ${dlite_SOURCE_DIR}/doc/code_generation.md
    ${dlite_SOURCE_DIR}/doc/collections.md
    ${dlite_SOURCE_DIR}/doc/concepts.md
    ${dlite_SOURCE_DIR}/doc/environment_variables.md
    ${dlite_SOURCE_DIR}/doc/features.md
    ${dlite_SOURCE_DIR}/doc/mappings.md
    ${dlite_SOURCE_DIR}/doc/storage_plugins.md
    ${dlite_SOURCE_DIR}/doc/tools.md
    ${dlite_SOURCE_DIR}/doc/transactions.md
    ${dlite_SOURCE_DIR}/doc/figs/transactions.png

    ${dlite_SOURCE_DIR}/src/dlite.h
    ${dlite_SOURCE_DIR}/src/dlite-arrays.h
    ${dlite_SOURCE_DIR}/src/dlite-codegen.h
    ${dlite_SOURCE_DIR}/src/dlite-collection.h
    ${dlite_SOURCE_DIR}/src/dlite-datamodel.h
    ${dlite_SOURCE_DIR}/src/dlite-entity.h
    ${dlite_SOURCE_DIR}/src/dlite-errors.h
    ${dlite_SOURCE_DIR}/src/dlite-getlicense.h
    ${dlite_SOURCE_DIR}/src/dlite-json.h
    ${dlite_SOURCE_DIR}/src/dlite-macros.h
    ${dlite_SOURCE_DIR}/src/dlite-mapping.h
    ${dlite_SOURCE_DIR}/src/dlite-mapping-plugins.h
    ${dlite_SOURCE_DIR}/src/dlite-misc.h
    ${dlite_SOURCE_DIR}/src/dlite-schemas.h
    ${dlite_SOURCE_DIR}/src/dlite-storage.h
    ${dlite_SOURCE_DIR}/src/dlite-storage-plugins.h
    ${dlite_SOURCE_DIR}/src/dlite-store.h
    ${dlite_SOURCE_DIR}/src/dlite-type-cast.h
    ${dlite_SOURCE_DIR}/src/dlite-type.h
    ${dlite_SOURCE_DIR}/src/getuuid.h
    ${dlite_SOURCE_DIR}/src/pathshash.h
    ${dlite_SOURCE_DIR}/src/triple.h
    ${dlite_SOURCE_DIR}/src/triplestore.h

    ${dlite_SOURCE_DIR}/src/utils/compat.h
    ${dlite_SOURCE_DIR}/src/utils/dsl.h
    ${dlite_SOURCE_DIR}/src/utils/err.h
    ${dlite_SOURCE_DIR}/src/utils/execprocess.h
    ${dlite_SOURCE_DIR}/src/utils/fileinfo.h
    ${dlite_SOURCE_DIR}/src/utils/fileutils.h
    ${dlite_SOURCE_DIR}/src/utils/globmatch.h
    ${dlite_SOURCE_DIR}/src/utils/infixcalc.h
    ${dlite_SOURCE_DIR}/src/utils/jsmn.h
    ${dlite_SOURCE_DIR}/src/utils/jsmnx.h
    ${dlite_SOURCE_DIR}/src/utils/jstore.h
    ${dlite_SOURCE_DIR}/src/utils/map.h
    ${dlite_SOURCE_DIR}/src/utils/md5.h
    ${dlite_SOURCE_DIR}/src/utils/plugin.h
    ${dlite_SOURCE_DIR}/src/utils/session.h
    ${dlite_SOURCE_DIR}/src/utils/sha1.h
    ${dlite_SOURCE_DIR}/src/utils/sha3.h
    ${dlite_SOURCE_DIR}/src/utils/strtob.h
    ${dlite_SOURCE_DIR}/src/utils/strutils.h
    ${dlite_SOURCE_DIR}/src/utils/tgen.h
    ${dlite_SOURCE_DIR}/src/utils/tmpfileplus.h
    ${dlite_SOURCE_DIR}/src/utils/uuid4.h
    ${dlite_SOURCE_DIR}/src/utils/uuid.h

    ${dlite_SOURCE_DIR}/src/utils/tests/tgen_example.c
    )

  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
    ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)

  add_custom_command(OUTPUT html/index.html
    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    COMMAND ${CMAKE_COMMAND}
      -E make_directory html/src
    COMMAND ${CMAKE_COMMAND}
      -E copy ${dlite_SOURCE_DIR}/src/dlite-storage.h html/src/dlite-storage.h
    COMMAND ${CMAKE_COMMAND}
      -E copy ${dlite_SOURCE_DIR}/src/dlite-entity.h html/src/dlite-entity.h
    COMMAND ${CMAKE_COMMAND}
      -E copy ${dlite_SOURCE_DIR}/src/dlite-type.h html/src/dlite-type.h
    COMMAND ${CMAKE_COMMAND}
      -E copy ${dlite_SOURCE_DIR}/doc/SOFT-metadata-structure.png
         html/SOFT-metadata-structure.png
    COMMAND ${CMAKE_COMMAND}
      -E make_directory html/figs
    COMMAND ${CMAKE_COMMAND}
      -E copy ${dlite_SOURCE_DIR}/doc/figs/transactions.png
         html/figs/transactions.png
    COMMAND ${CMAKE_COMMAND}
      -E make_directory html/python-storage-plugins
    COMMAND ${CMAKE_COMMAND}
      -E copy ${dlite_SOURCE_DIR}/storages/python/python-storage-plugins/yaml.py
         html/python-storage-plugins/yaml.py
    COMMAND ${CMAKE_COMMAND}
      -Dscript=${CMAKE_CURRENT_SOURCE_DIR}/fix_index.py
      -Dhtmlfile=${CMAKE_CURRENT_BINARY_DIR}/html/index.html
      -P ${CMAKE_CURRENT_SOURCE_DIR}/fix_index.cmake
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


  # LaTeX documentation
  add_custom_command(OUTPUT latex/refman.pdf
    COMMAND make
    DEPENDS html/index.html
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/latex
    COMMENT "Generating LaTeX documentation"
    VERBATIM
    )

  add_custom_target(latex
    DEPENDS latex/refman.pdf
    )

  install(
    DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html
    DESTINATION share/dlite
    )

endif(DOXYGEN_FOUND)
