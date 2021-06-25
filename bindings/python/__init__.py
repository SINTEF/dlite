import sys
import os
from pathlib import Path

if 'DLITE_ROOT' not in os.environ:
    os.environ['DLITE_ROOT']=Path(__file__).parent.resolve().as_posix()

from .dlite import *  # noqa: F401, F403
from .factory import classfactory, objectfactory, loadfactory  # noqa: F401

