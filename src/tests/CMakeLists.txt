# -*- Mode: cmake -*-
#

set(tests
  test_utils
  test_entity
  test_store
  test_triplestore
  test_collection
  test_schemas
  test_arrays
  )
if(WITH_JSON)
  list(APPEND tests
    test_json_storage
    test_json_utils
    test_json_entity
    )
endif()
if(WITH_HDF5)
  list(APPEND tests
    test_datamodel
    )
endif()


add_definitions(-DDLITE_ROOT=${dlite_SOURCE_DIR})

#link_directories(
#  ${dlite_BINARY_DIR}/src/utils
#    )

foreach(test ${tests})
  add_executable(${test} ${test}.c)
  add_dependencies(${test} dlite-static ${uuidProj})
  target_link_libraries(${test}
    dlite-static
    #uuid-static
    dlite-utils-static
    ${UUID_STATIC_LIBRARIES}
    )
  target_include_directories(${test} PRIVATE
    ${dlite_SOURCE_DIR}/src
    ${dlite_BINARY_DIR}/src
    #${uuid_BINARY_DIR}
    ${UUID_INCLUDE_DIRS}
    )
  #set_target_properties(${test} PROPERTIES
  #  IMPORTED_LOCATION ${UUID_STATIC_LIBRARIES}
  #  )

  add_test(
    NAME ${test}
    COMMAND ${test}
    )
<<<<<<< ALP-42-tmp
  # Hmm, for some reason the tests seems to link the shared uuid library...
  # On Windows we must therefore add uuid.dll to PATH.
  if(MSVC)
    set_tests_properties(${test} PROPERTIES
      ENVIRONMENT "PATH=${uuid_BINARY_DIR};$ENV{PATH}"
      )
  endif()
=======
  set_property(TEST ${test} PROPERTY
    ENVIRONMENT "PATH=${dlite_PATH}")
  set_property(TEST ${test} APPEND PROPERTY
    ENVIRONMENT "LD_LIBRARY_PATH=${dlite_LD_LIBRARY_PATH}")
  set_property(TEST ${test} APPEND PROPERTY
    ENVIRONMENT "DLITE_STORAGE_PLUGINS=${dlite_STORAGE_PLUGINS}")

>>>>>>> local
endforeach()

# Add extra include dir for json tests
if(WITH_JSON)
  foreach(test test_json_storage test_json_utils)
    target_include_directories(${test} PRIVATE
      ${dlite_SOURCE_DIR}/plugins/json
      )
  endforeach()
endif()
