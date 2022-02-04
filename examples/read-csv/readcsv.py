from pathlib import Path

import dlite


# Set up some paths
thisdir = Path(__file__).parent
csvfile = thisdir / 'faithful.csv'

# Add the python-storage-plugins subdir to python storage plugin search path
dlite.python_storage_plugin_path.append(thisdir / 'python-storage-plugins')

csv = dlite.Instance.create_from_url(f'csv://{csvfile}')

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



csv.save('yaml://faithful.yaml?mode=w')
