# Copy directory
#
# Parameters (passed with -D)
#  - SOURCE_DIR: directory to copy
#  - DEST_DIR: new destination directory
#  - PATTERN: pattern matching files to include
#
file(
  COPY "${SOURCE_DIR}/"
  DESTINATION "${DEST_DIR}"
  FILES_MATCHING PATTERN "${PATTERN}"
)
