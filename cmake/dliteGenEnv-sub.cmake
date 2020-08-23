# Sub-cmake file for running configure_file() during build time

# TODO - add regex for handling other windows drive letters

macro(tounix0 varname input)
  string(REPLACE "\\"  "/"   ${varname} "${input}")
  string(REPLACE "C:/" "/c/" ${varname} "${${varname}}")
endmacro()

macro(tounix varname input)
  string(REPLACE "\\"  "/"   ${varname} "${input}")
  string(REPLACE "C:/" "/c/" ${varname} "${${varname}}")
  string(REPLACE "|"   ":"   ${varname} "${${varname}}")
endmacro()

macro(towin0 varname input)
  string(REPLACE "/c/" "C:\\" ${varname} "${input}")
  string(REPLACE "/"   "\\"   ${varname} "${${varname}}")
endmacro()

macro(towin varname input)
  string(REPLACE "/c/" "C:\\" ${varname} "${input}")
  string(REPLACE "/"   "\\"   ${varname} "${${varname}}")
  string(REPLACE "|"   ";"    ${varname} "${${varname}}")
endmacro()


tounix(PATH_unix                   "${PATH}")
tounix(LD_LIBRARY_PATH_unix        "${LD_LIBRARY_PATH}")
tounix(PYTHONPATH_unix             "${PYTHONPATH}")
tounix(STORAGE_PLUGINS_unix        "${STORAGE_PLUGINS}")
tounix(MAPPING_PLUGINS_unix        "${MAPPING_PLUGINS}")
tounix(PYTHON_STORAGE_PLUGINS_unix "${PYTHON_STORAGE_PLUGINS}")
tounix(PYTHON_MAPPING_PLUGINS_unix "${PYTHON_MAPPING_PLUGINS}")
tounix(TEMPLATES_unix              "${TEMPLATES}")
tounix0(STORAGES_unix              "${STORAGES}")

# Windows path separators
towin(PATH_win                     "${PATH}")
towin(LD_LIBRARY_PATH_win          "${LD_LIBRARY_PATH}")
towin(PYTHONPATH_win               "${PYTHONPATH}")
towin(STORAGE_PLUGINS_win          "${STORAGE_PLUGINS}")
towin(MAPPING_PLUGINS_win          "${MAPPING_PLUGINS}")
towin(PYTHON_STORAGE_PLUGINS_win   "${PYTHON_STORAGE_PLUGINS}")
towin(PYTHON_MAPPING_PLUGINS_win   "${PYTHON_MAPPING_PLUGINS}")
towin(TEMPLATES_win                "${TEMPLATES}")
towin0(STORAGES_win                "${STORAGES}")


# #string(REPLACE "C:/" "/c/" PATH                   "${PATH}")
# string(REPLACE "C:/" "/c/" LD_LIBRARY_PATH        "${LD_LIBRARY_PATH}")
# string(REPLACE "C:/" "/c/" PYTHONPATH             "${PYTHONPATH}")
# string(REPLACE "C:/" "/c/" STORAGE_PLUGINS        "${STORAGE_PLUGINS}")
# string(REPLACE "C:/" "/c/" MAPPING_PLUGINS        "${MAPPING_PLUGINS}")
# string(REPLACE "C:/" "/c/" PYTHON_STORAGE_PLUGINS "${PYTHON_STORAGE_PLUGINS}")
# string(REPLACE "C:/" "/c/" PYTHON_MAPPING_PLUGINS "${PYTHON_MAPPING_PLUGINS}")
# string(REPLACE "C:/" "/c/" TEMPLATES              "${TEMPLATES}")
#
# #string(REPLACE "|" ":" PATH_unix                   "${PATH}")
# string(REPLACE "|" ":" LD_LIBRARY_PATH_unix        "${LD_LIBRARY_PATH}")
# string(REPLACE "|" ":" PYTHONPATH_unix             "${PYTHONPATH}")
# string(REPLACE "|" ":" STORAGE_PLUGINS_unix        "${STORAGE_PLUGINS}")
# string(REPLACE "|" ":" MAPPING_PLUGINS_unix        "${MAPPING_PLUGINS}")
# string(REPLACE "|" ":" PYTHON_STORAGE_PLUGINS_unix "${PYTHON_STORAGE_PLUGINS}")
# string(REPLACE "|" ":" PYTHON_MAPPING_PLUGINS_unix "${PYTHON_MAPPING_PLUGINS}")
# string(REPLACE "|" ":" TEMPLATES_unix              "${TEMPLATES}")
#
# # Windows path separators
# #string(REPLACE "|" ";" PATH_win                   "${PATH}")
# string(REPLACE "|" ";" LD_LIBRARY_PATH_win        "${LD_LIBRARY_PATH}")
# string(REPLACE "|" ";" PYTHONPATH_win             "${PYTHONPATH}")
# string(REPLACE "|" ";" STORAGE_PLUGINS_win        "${STORAGE_PLUGINS}")
# string(REPLACE "|" ";" MAPPING_PLUGINS_win        "${MAPPING_PLUGINS}")
# string(REPLACE "|" ";" PYTHON_STORAGE_PLUGINS_win "${PYTHON_STORAGE_PLUGINS}")
# string(REPLACE "|" ";" PYTHON_MAPPING_PLUGINS_win "${PYTHON_MAPPING_PLUGINS}")
# string(REPLACE "|" ";" TEMPLATES_win              "${TEMPLATES}")


configure_file(${input} ${output}
  @ONLY
  NEWLINE_STYLE ${newline_style}
  )
