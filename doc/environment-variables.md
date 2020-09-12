Environment variables
=====================
This page describes environment variables used by DLite.

Generic environment variables
-----------------------------
  - **PATH**: Standard search path for runtimes (.exe and .dll on Windows).
    The paths are separated by ";" on Windows and ":" on Linux.

  - **LD_LIBRARY_PATH**: Standard search path for shared libraries on
    Linux.  The paths are separated by ":".

  - **PYTHONPATH**: Standard search path for python modules and packages.
    The paths are separated by ";" on Windows and ":" on Linux.


DLite-specific environment variables
------------------------------------
  - **DLITE_STORAGE_PLUGIN_DIRS**: Search path for DLite storage plugins.
    The paths are separated by ";" on Windows and ":" on Linux.

  - **DLITE_PYTHON_STORAGE_PLUGIN_DIRS**: Search path for DLite storage plugins
    written in Python.
    The paths are separated by ";" on Windows and ":" on Linux.

  - **DLITE_MAPPING_PLUGIN_DIRS**: Search path for DLite mapping plugins.
    The paths are separated by ";" on Windows and ":" on Linux.

  - **DLITE_PYTHON_MAPPING_PLUGIN_DIRS**: Search path for DLite mapping plugins
    written in Python.
    The paths are separated by ";" on Windows and ":" on Linux.

  - **DLITE_TEMPLATE_DIRS**: Search path for DLite templates.
    The paths are separated by ";" on Windows and ":" on Linux.

  - **DLITE_STORAGES**: DLite storages for looking up metadata and instances.
    The storages is separated by "|". Each storage should be specified by
    either
      - an URL (driver://location?options)
      - a file path (/path/to/storage.json)
      - a glob pattern (/path/to/*.json)
    In the two last cases, the file extension must match the driver name.


Environment variables for controlling error handling
----------------------------------------------------
  - **ERR_STREAM**: Error stream to write messages to.  May be:
      - empty             : do not write anything
      - "stderr" | unset  : write to stderr
      - "stdout"          : write to stdout
      - otherwise         : open the given file and append to it

  - **ERR_ABORT**: Whether errors should should return normally,
    exit or about.  May be:
      - "0" | "normal" | unset : return normally
      - "1" | "exit"           : exit
      - "2" | "abort" | empty  : abort
      - otherwise              : return normally

  - **ERR_WARN**: Whether warnings should be ignored or turned into errors.
    May be:
      - "0" | "normal" | unset : report normally
      - "1" | "ignore"         : ignore
      - "2" | "error"          : turn into error
      - otherwise              : report normally

  - **ERR_DEBUG**: Wheter debugging information (source file, line number
    and function name) should be included in the error message.  May be:
      - "0" | unset | empty  : no debugging info
      - "1" | "debug"        : print file and line number
      - "2" | "full"         : print file, line number and function
      - otherwise            : no debugging info

  - **ERR_OVERRIDE**: How to handle error messages when there already is a
     message in the error message buffer.  Note that these options will
     only affect the error message, not the error value.  May be:
      - unset | empty       : append new error message to the old one
      - "0" | "append"      : append new error message
      - "1" | "warn-old"    : overwrite old error message and write warning
      - "2" | "warn-new"    : ignore new error message and write warning
      - "3" | "old"         : overwrite old error message
      - "4" | "ignore-new"  : ignore new error message
      - otherwise           : append new error message to the old one
