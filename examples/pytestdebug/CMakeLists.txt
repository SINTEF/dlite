cmake_minimum_required(VERSION 3.16)

project(pytestdebug
  LANGUAGES C
)

add_compile_options("-g")


find_package(Python3 REQUIRED COMPONENTS Interpreter Development)

find_package(SWIG)
include(UseSWIG)
set(swig_flags "-Wall;-keyword")
if(${SWIG_VERSION} VERSION_LESS 4.1)
  list(APPEND swig_flags "-py3")
endif()


set(pkgdir ${CMAKE_CURRENT_BINARY_DIR}/pytestdebug)

swig_add_library(pytestdebug
  TYPE SHARED
  LANGUAGE python
  OUTPUT_DIR ${pkgdir}
  OUTFILE_DIR ${CMAKE_CURRENT_BINARY_DIR}
  SOURCES pytestdebug.i pytestdebug/__init__.py
)

target_include_directories(pytestdebug
  PUBLIC ${Python3_INCLUDE_DIRS}
)

swig_link_libraries(pytestdebug
  ${Python3_LIBRARIES}
)

add_custom_command(
  TARGET pytestdebug
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/pytestdebug/__init__.py"
    ${pkgdir}/__init__.py
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "$<TARGET_FILE:pytestdebug>"
    ${pkgdir}
)


install(
  DIRECTORY ${pkgdir}
  DESTINATION "${Python3_SITEARCH}"
  USE_SOURCE_PERMISSIONS
  PATTERN ".gitignore" EXCLUDE
  PATTERN "*~" EXCLUDE
)
