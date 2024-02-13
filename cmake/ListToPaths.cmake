# -- Macro converting a list of paths to a string with host path separator
#
# list_to_paths(<list>)
#

macro(list_to_paths lst)

  if(${lst})
    list(REMOVE_DUPLICATES ${lst})
  endif()

  if(WIN32)
    #set(${lst} "\"${${lst}}\"")
    string(REPLACE ";" "\;" ${lst} "${${lst}}")
  else()
    string(REPLACE ";" ":" ${lst} "${${lst}}")
  endif()

endmacro()
