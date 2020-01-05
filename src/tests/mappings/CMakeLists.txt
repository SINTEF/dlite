# -*- Mode: cmake -*-
#

set(plugins
  mapA
  )

foreach(plugin ${plugins})
  add_library(${plugin} SHARED ${plugin}.c)
  target_link_libraries(${plugin}
    #dlite-static
    #dlite-utils-static
    dlite
    dlite-utils
    )
  target_include_directories(${plugin} PRIVATE
    ${dlite_SOURCE_DIR}/src
    ${dlite_BINARY_DIR}/src
    )
endforeach()
