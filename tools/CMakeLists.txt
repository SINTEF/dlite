# -*- Mode: cmake -*-
project(dlite-tools C)

# Workaround for CMake not setting the correct binary path for MSVC
if(MSVC)
  set(dlite-tools_BINARY_DIR ${dlite-tools_BINARY_DIR}/$<CONFIG>)
endif()


link_directories(${uuid_BINARY_DIR})

# dlite-getuuid
add_executable(dlite-getuuid dlite-getuuid.c)
target_link_libraries(dlite-getuuid dlite uuid)
target_include_directories(dlite-getuuid PRIVATE
  ${dlite_SOURCE_DIR}/src
  ${uuid_SOURCE_DIR}
  ${uuid_BINARY_DIR}
  )


# dlite-codegen
add_executable(dlite-codegen dlite-codegen.c)
target_link_libraries(dlite-codegen dlite)
target_include_directories(dlite-codegen PRIVATE
  ${dlite_SOURCE_DIR}/src
  ${dlite_BINARY_DIR}/src
  ${uuid_BINARY_DIR}
  )


# Subdirectories
add_subdirectory(tests)


# Install
install(
  TARGETS dlite-getuuid dlite-codegen
  DESTINATION bin)

install(
  DIRECTORY templates
  DESTINATION share/dlite
  PATTERN "*~" EXCLUDE
  )
