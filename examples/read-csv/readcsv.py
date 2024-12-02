from pathlib import Path

import dlite
from dlite import importskip

importskip("pandas")
importskip("tables")
importskip("yaml")


# Set up some paths
thisdir = Path(__file__).resolve().parent
csvfile = thisdir / 'faithful.csv'

# Add the python-storage-plugins subdir to python storage plugin search path
#dlite.python_storage_plugin_path.append(thisdir / 'python-storage-plugins')

csv = dlite.Instance.from_url(f'csv://{csvfile}')

print(csv.meta)
print(csv.uuid)

csv.save('csv://newfile.csv?mode=w')


# Try to use the pandas hdf writer...
csv.save('csv://newfile.h5?pandas_opts="key": "group", "index": False')


# ... or excel writer (requires openpyxl)
try:
    import openpyxl  # noqa: F401
    csv.save('csv://newfile.xlsx')
except ImportError:
    pass


with dlite.Storage("json", "newfile.json", "mode=w;single=no") as s:
    s.save(csv.meta)
    s.save(csv)

csv.save('yaml://faithful.yaml?mode=w')
csv.meta.save('yaml://faithful-meta.yaml?mode=w')

uuid = csv.uuid
metaid = csv.meta.uuid

del csv
#inst = dlite.Instance.from_location('json', 'newfile.json', id=uuid)
#meta = dlite.Instance.from_location('yaml', 'faithful-meta.yaml', id=metaid)
inst = dlite.Instance.from_location('yaml', 'faithful.yaml', id=uuid)
