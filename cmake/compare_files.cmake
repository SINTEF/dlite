# Compares 2 two files, skipping the first 'offset' bytes
#
# Usage: cmake -Dfile1=FILE1 -Dfile2=FILE1 -Doffset=160 -P compare_odf.cmake
#

if(NOT offset)
  set(offset 0)
endif()

file(READ ${file1} file1_content OFFSET ${offset})
file(READ ${file2} file2_content OFFSET ${offset})

if(NOT ${file1_content} STREQUAL ${file2_content})
  message(FATAL_ERROR "Files differ: ${file1} ${file2}")
endif()
