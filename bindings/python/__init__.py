import sys
import os
from pathlib import Path

# For Anaconda Python on Windows, it is sufficient to set the PATH before
# calling Python so that _dlite.pyd can find and load the DLLs it depends on.
# The dll loading trick below is not working on Anaconda Python
# https://github.com/pytorch/pytorch/issues/17051

# For CPython, setting the PATH is not sufficient. This may be related to the
# following bug recognised by the numpy cummunity;
# https://github.com/numpy/numpy/issues/12667
#
# For CPython: To work around, call AddDllDirectory() directly via ctypes to set the
# search path on Windows

try:
    import conda
except:
    is_conda_base = False
    is_conda = os.path.exists(os.path.join(sys.prefix, 'conda-meta'))
else:
    assert os.path.exists(os.path.join(sys.prefix, 'conda-meta'))
    is_conda_base = True
    is_conda = True

if (sys.platform == 'win32') and (not is_conda):
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

    del c_wchar_p, DWORD, dlite_INSTALL_ROOT, dlite_PATH
    del AddDllDirectory


# FIXME: Do we need this variable to be set or can we live without this?
if 'DLITE_ROOT' not in os.environ:
    os.environ['DLITE_ROOT']=Path(__file__).parent.resolve().as_posix()

from .dlite import *  # noqa: F401, F403
from .factory import classfactory, objectfactory, loadfactory  # noqa: F401

