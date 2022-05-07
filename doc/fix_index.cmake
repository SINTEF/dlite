# CMake script that fixes links in html file

find_package(Python3)

if(Python3_FOUND)
  message(STATUS "Fix html links")
  execute_process(
    COMMAND ${Python3_EXECUTABLE} ${script} ${htmlfile}
    )
endif()
