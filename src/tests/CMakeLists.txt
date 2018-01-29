# -*- Mode: cmake -*-
#

include_directories(
  ${dlite_BINARY_DIR}/src
  ${uuid_BINARY_DIR}
  )

set(tests
  test_dlite
  test_datamodel
  test_entity
  )

foreach(test ${tests})
  add_executable(${test} ${test}.c)
  target_link_libraries(${test} dlite uuid)
  target_include_directories(${test} PRIVATE ${dlite_SOURCE_DIR}/src)

  add_test(
    NAME ${test}
    COMMAND ${test}
    )
endforeach()

#add_executable(test_dlite test_dlite)
#target_link_libraries(test_dlite dlite uuid)
#target_include_directories(test_dlite PRIVATE ${dlite_SOURCE_DIR}/src)
#
#add_executable(test_datamodel test_datamodel)
#target_link_libraries(test_datamodel dlite uuid)
#target_include_directories(test_datamodel PRIVATE ${dlite_SOURCE_DIR}/src)
#
#add_test(
#  NAME test_dlite
#  COMMAND test_dlite
#  )
#
#add_test(
#  NAME test_datamodel
#  COMMAND test_datamodel
#  )
#
