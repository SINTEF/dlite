# -*- Mode: cmake -*-
project(dlite-examples)

# Python examples
if(WITH_PYTHON)
set(python_examples
  read-csv
  dehydrogenation
  mappings
  storage_plugin
  entity_service
  minio_storage
  datamodel_as_rdf
  TEM_data
  )
endif()


# CMake toolchain option
if(CMAKE_TOOLCHAIN_FILE)
  get_filename_component(toolchain_file ${CMAKE_TOOLCHAIN_FILE} REALPATH)
  set(toolchain_opt "-DCMAKE_TOOLCHAIN_FILE=${toolchain_file}")
endif()

#
# Macro for testing examples
# --------------------------
# It adds some tests for each example that essentially runs the
# standard configure/build/test sequence of commands (here shown using
# unix makefiles):
#
#      mkdir build                            (ex-mkdir)
#      cd build
#      cmake <conf> <source_dir>              (ex-config)
#      make                                   (ex-build)
#      make test                              (ex-test)
#
# Here <conf> is the configurations that dlite is build with.
macro(test_example example)
  add_test(
    NAME ${example}-mkdir
    COMMAND ${CMAKE_COMMAND}
      -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/${example}
    )
  add_test(
    NAME ${example}-config
    COMMAND ${CMAKE_COMMAND}
      -G ${CMAKE_GENERATOR}
      -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
      -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
      -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
      -DDLITE_ROOT=${CMAKE_INSTALL_PREFIX}
      ${toolchain_opt}
      ${CMAKE_CURRENT_SOURCE_DIR}/${example}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${example}
    )
  add_test(
    NAME ${example}-build
    COMMAND ${CMAKE_COMMAND} --build .
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${example}
    )
  add_test(
    NAME ${example}-test
    COMMAND ${CMAKE_COMMAND} --build . --target test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${example}
    )
  set_property(TEST ${example}-config  PROPERTY DEPENDS ${example}-mkdir)
  set_property(TEST ${example}-build   PROPERTY DEPENDS ${example}-config)
  set_property(TEST ${example}-test    PROPERTY DEPENDS ${example}-build)
endmacro()


# Only run the examples if dlite has been installed
#
# This is not perfect, since the if-statement below is evaluated at
# configure time.  However, in practice this should be fine...
#
# The FORCE_EXAMPLES option gives us the option to bypass testing examples
# if dlite hasn't been installed.  This may be useful in CI frameworks
if(FORCE_EXAMPLES OR EXISTS ${CMAKE_INSTALL_PREFIX}/share/dlite/examples)
  if(WITH_HDF5)
    test_example(ex1)
    test_example(ex2)
    test_example(ex3)
    if(WITH_FORTRAN)
      test_example(ex4)
    endif()
  endif()
  #test_example(ex5d)

  # Run python examples
  foreach(name ${python_examples})
    add_test(
      NAME ${name}
      COMMAND ${RUNNER} ${Python3_EXECUTABLE}
          ${CMAKE_CURRENT_SOURCE_DIR}/${name}/main.py
      )
    set_property(TEST ${name} PROPERTY
      ENVIRONMENT "PATH=${dlite_PATH_NATIVE}${PATHSEP}$ENV{PATH}")
    set_property(TEST ${name} APPEND PROPERTY
      ENVIRONMENT "PYTHONPATH=${dlite_PYTHONPATH_NATIVE}")
    set_property(TEST ${name} APPEND PROPERTY
      ENVIRONMENT "USER=$ENV{USER}")
    if(NOT WIN32)
      set_property(TEST ${name} APPEND PROPERTY
      ENVIRONMENT "LD_LIBRARY_PATH=${dlite_LD_LIBRARY_PATH_NATIVE}")
    endif()
    set_property(TEST ${name} APPEND PROPERTY
      ENVIRONMENT "DLITE_USE_BUILD_ROOT=YES")
    set_property(TEST ${name} APPEND PROPERTY
      ENVIRONMENT "DLITE_IMPORTSKIP_EXITCODE=$ENV{DLITE_IMPORTSKIP_EXITCODE}")

    set_property(TEST ${name} PROPERTY
      SKIP_RETURN_CODE 44)
  endforeach()


  # Special properties on minio_storage for not hanging when the MinIO
  # playground is not responding or when the storage is full
  set_property(TEST minio_storage APPEND PROPERTY
    ENVIRONMENT "TIMEOUT=5"
  )
  set_property(TEST minio_storage PROPERTY
    SKIP_REGULAR_EXPRESSION "(XMinioStorageFull)|(subprocess.TimeoutExpired)"
  )

endif()





install(
  DIRECTORY ex1 ex2 ex3 ex4 ex5d read-csv dehydrogenation
  DESTINATION share/dlite/examples
  PATTERN "*~" EXCLUDE
  PATTERN build EXCLUDE
  )
