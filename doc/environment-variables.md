Environment variables
=====================
A summary of all environment variables are used by DLite.


General environment variables
-----------------------------
Standard environment variables also known outside of DLite.  You may
have to adjust these.

  - **PATH**: Standard search path for runtimes (.exe and .dll on Windows).
    The paths are separated by ";" on Windows and ":" on Linux.

  - **LD_LIBRARY_PATH**: Standard search path for shared libraries on
    Linux.  The paths are separated by ":".

  - **PYTHONPATH**: Standard search path for python modules and packages.
    The paths are separated by ";" on Windows and ":" on Linux.


DLite installation root
-----------------------
DLITE_ROOT is the main installation root that prefixes all the
specific paths below.

  - **DLITE_ROOT**: The root of the installation path.  It default
    to the installation prefix provided to cmake.  You have to set this
    if dlite is moved.

  - **DLITE_USE_BUILD_ROOT**: If this is defined, use the build root instead
    of the installation root.  This environment variable is mainly intended
    for testing.

    @note
    On Windows `DLITE_USE_BUILD_ROOT` affects both the library and the
    plugin search paths, while it only affects the plugin search paths
    on Linux.


Spesific paths
--------------
These environment variables can be used to provide additional search
paths in addition to the defaults in the installation or build root.

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
