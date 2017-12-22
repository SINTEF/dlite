# -*- Mode: cmake -*-
#

include_directories(
  ${dlite_BINARY_DIR}/src
  ${uuid_BINARY_DIR}
  )

add_executable(test_dlite test_dlite)
target_link_libraries(test_dlite dlite uuid)
target_include_directories(test_dlite PRIVATE ${dlite_SOURCE_DIR}/src)

add_test(
  NAME test_dlite
  COMMAND test_dlite "${CMAKE_CURRENT_SOURCE_DIR}/db-AlMgSiFeCu.h5"
  )

#add_executable(h5ex_t_vlstring h5ex_t_vlstring.c)
#target_link_libraries(h5ex_t_vlstring hdf5)
#add_test(NAME h5ex_t_vlstring COMMAND h5ex_t_vlstring)
#
#add_executable(h5ex_t_string h5ex_t_string.c)
#target_link_libraries(h5ex_t_string hdf5)
#add_test(NAME h5ex_t_string COMMAND h5ex_t_string)
