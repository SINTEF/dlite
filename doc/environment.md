Environment variables
=====================
The following environment variables are used by DLite.

  - **PATH**: Standard search path for runtimes (.exe and .dll on Windows).
    The paths are separated by ";" on Windows and ":" on Linux.

  - **LD_LIBRARY_PATH**: Standard search path for shared libraries on
    Linux.  The paths are separated by ":".

  - **PYTHONPATH**: Standard search path for python modules and packages.
    The paths are separated by ";" on Windows and ":" on Linux.

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
