"""Configure environment paths.  Should be called before importing dlite."""
import os
from pathlib import Path


thisdir = Path(__file__).parent
csvfile = thisdir / 'faithful.csv'
plugindir = thisdir / 'python-storage-plugins'

os.environ['DLITE_PYTHON_STORAGE_PLUGIN_DIRS'] = str(plugindir)
