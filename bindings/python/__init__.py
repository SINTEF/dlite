import sys

# For some reason setting PATH before calling Python, is not enough on
# Windows for _dlite.pyd to find and load the DLLs it depends on.
# This bug has also been recognised by the numpy cummunity;
# https://github.com/numpy/numpy/issues/12667
#
# To work around, call AddDllDirectory() directly via ctypes to set the
# search path on Windows
if sys.platform == 'win32':
    import os
    from pathlib import Path
    from ctypes import windll, c_wchar_p
    from ctypes.wintypes import DWORD
    from .paths import dlite_INSTALL_ROOT, dlite_PATH

    AddDllDirectory = windll.kernel32.AddDllDirectory
    AddDllDirectory.restype = DWORD
    AddDllDirectory.argtypes = [c_wchar_p]

    if ('DLITE_USE_BUILD_ROOT' in os.environ and
        not os.environ['DLITE_USE_BUILD_ROOT'].lower() in
            ('0', 'no', 'false', '.false.', 'off')):
        for path in dlite_PATH.split(';'):
            AddDllDirectory(str(Path(path)))
    else:
        AddDllDirectory(str(Path(dlite_INSTALL_ROOT) / 'bin'))

    del Path, c_wchar_p, DWORD, dlite_INSTALL_ROOT, dlite_PATH
    del AddDllDirectory


from .dlite import *  # noqa: F401, F403
from .factory import classfactory, objectfactory, loadfactory  # noqa: F401
