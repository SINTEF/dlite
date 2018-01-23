# -*- Mode: cmake -*-
#


link_directories(${uuid_BINARY_DIR})


# dlite-getuuid
add_executable(dlite-getuuid dlite-getuuid.c)
target_link_libraries(dlite-getuuid dlite uuid)
target_include_directories(dlite-getuuid PRIVATE
  ${dlite_SOURCE_DIR}/src
  ${uuid_SOURCE_DIR}
  ${uuid_BINARY_DIR}
  )
install(
  TARGETS dlite-getuuid
  DESTINATION bin)


# Subdirectories
add_subdirectory(tests)
