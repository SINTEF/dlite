Environment variables
=====================
A summary of all environment variables are used by DLite.


General environment variables
-----------------------------
Standard environment variables also known outside of DLite.  You may
have to adjust these.

Generic environment variables
-----------------------------
  - **PATH**: Standard search path for runtimes (.exe and .dll on Windows).
    The paths are separated by ";" on Windows and ":" on Linux.

  - **LD_LIBRARY_PATH**: Standard search path for shared libraries on
    Linux.  The paths are separated by ":". This has no special meaning for Windows.

  - **PYTHONPATH**: Standard search path for python modules and packages.
    The paths are separated by ";" on Windows and ":" on Linux.


DLite-specific environment variables
------------------------------------
  - **DLITE_ROOT**: The root of the installation path.  It defaults
    to the installation prefix provided to cmake.  You have to set this
    if dlite is moved.

    In case no install prefix is provided to CMake, the default for
    DLITE_ROOT is "/.local" on Linux and "%ALLUSERSPROFILE%/.local" on Windows.

  - **DLITE_PLATFORM**: Specifies how paths are formatted.  If defined,
    it should be "Native" (default), "Unix" or "Windows".

  - **DLITE_USE_BUILD_ROOT**: If this is defined, use the build root instead
    of the installation root.  This environment variable is only intended
    for testing.

    **note**:
    On Windows `DLITE_USE_BUILD_ROOT` affects both the library and the
    plugin search paths, while it only affects the plugin search paths on
    Linux.

  - **DLITE_PYDEBUG**: If defined, it will turn on printing error
    messages from C to standard error.

    It will also enable traceback from exceptions occuring in Python
    plugins.  Traceback from exceptions occurring in Python plugins
    are normally lost.  With `DLITE_PYDEBUG` defined the full error
    message, including traceback, will be written to standard error.

  - **DLITE_ATEXIT_FREE**: Free memory at exit.  This might be useful to avoid
    getting false positive when tracking down memory leaks with tools like valgrind.

  - **DLITE_BEHAVIOR**: Enables/disables all behavior changes by default.

    The empty string or any of the following values will enable the behaviors:
    "true", ".true.", "on", "yes", 1

    Any of the following values will disable the behaviors:
    "false", ".false.", "off", "no", 0

  - **DLITE_BEHAVIOR_<name>**: Enables/disables the behavior `<name>`.
    This overrides `DLITE_BEHAVIOR` for the named behavior.  The value
    has the same meaning as for `DLITE_BEHAVIOR`.


### Specific paths
These environment variables can be used to provide additional search
paths apart from the defaults, which is either in the installation
root (if DLITE_USE_BUILD_ROOT is not set) or build directory (if
DLITE_USE_BUILD_ROOT is set).

  - **DLITE_PKG_ROOT**: With Python, this is the root directory of the installed
    DLite python package.  Without Python, this is the same as DLITE_ROOT.

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

  - **DLITE_IMPORTSKIP_EXITCODE**: Exit code from tests that fail to
    load a python module with `importskip()`.  Define this to 1 at
    configure time if you want the tests not to be skipped if they
    cannot import a needed module.


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

  - **ERR_COLOR**: Whether to print error messages in colour. May be:
      - "0" | "never"       : print errors colour-coded
      - "1" | "always"      : print errors not colour-coded
      - otherwise           : only print errors colour-coded if the error
                              stream is a terminal


Path handling when using the pre-packaged wheel (Linux, Windows)
----------------------------------------------------------------

dlite-python can be installed from the wheel distribution. In that case, paths
compiled into the executables during build have no meaning (They may originate from a different machine).
The installer (pip) determines the location of the local install which also depends on
the Python used (Anaconda base, virtual environment, CPython, etc.)

In the following, we assume that the package is installed in .env/Lib/site-packages/dlite

The dlite-specific search paths can be influenced by the following environment variables:

DLITE_ROOT
DLITE_STORAGE_PLUGIN_DIRS
DLITE_PYTHON_STORAGE_PLUGIN_DIRS
DLITE_PYTHON_MAPPING_PLUGIN_DIRS
DLITE_MAPPING_PLUGIN_DIRS
DLITE_STORAGES

The following rules apply:

1. DLITE_ROOT is defined

dlite.storage_plugin_path        %DLITE_STORAGE_PLUGIN_DIRS%;        "%DLITE_ROOT%\share\dlite\storage-plugins"
dlite.python_storage_plugin_path %DLITE_PYTHON_STORAGE_PLUGIN_DIRS%; "%DLITE_ROOT%\share\dlite\python-storage-plugins"
dlite.python_mapping_plugin_path %DLITE_PYTHON_MAPPING_PLUGIN_DIRS%; "%DLITE_ROOT%\share\dlite\python-mapping-plugins"

dlite.mapping_plugin_path        %DLITE_MAPPING_PLUGIN_DIRS%;       ".env\Lib\site-packagesshare\dlite\mapping-plugins"
dlite.storage_path               %DLITE_STORAGES%;                  ".env\Lib\site-packagesshare\dlite\storages\*.json"


2. DLITE_ROOT is undefined

dlite.storage_plugin_path        %DLITE_STORAGE_PLUGIN_DIRS%;        ".env\Lib\site-packages\dlite\share\dlite\storage-plugins"
dlite.python_storage_plugin_path %DLITE_PYTHON_STORAGE_PLUGIN_DIRS%; ".env\Lib\site-packages\dlite\share\dlite\python-storage-plugins"
dlite.python_mapping_plugin_path %DLITE_PYTHON_MAPPING_PLUGIN_DIRS%; ".env\Lib\site-packages\dlite\share\dlite\python-mapping-plugins"

dlite.mapping_plugin_path        %DLITE_MAPPING_PLUGIN_DIRS%;        ".env\Lib\site-packagesshare\dlite\mapping-plugins"
dlite.storage_path               %DLITE_STORAGES%;                   ".env\Lib\site-packagesshare\dlite\storage\*.jsons"
