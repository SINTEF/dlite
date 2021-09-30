"""Configure environment paths.  Should be called before importing dlite."""
import os
from pathlib import Path


thisdir = Path(__file__).parent
metadatadir = thisdir / molecules
os.environ['DLITE_STORAGES'] = str(metadatadir)
