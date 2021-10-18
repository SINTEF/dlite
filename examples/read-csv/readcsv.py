from config_paths import csvfile
import dlite


csv = dlite.Instance(f'csv:{csvfile}')

print(csv.meta)
print(csv.uuid)


csv.save('csv://newfile.csv?mode=w')


# Try to use the pandas hdf writer...
csv.save('csv:newfile.h5?pandas_opts="key": "group", "index": False')


# ... or excel writer (requires openpyxl)
try:
    import openpyxl  # noqa: F401
    csv.save('csv:newfile.xlsx')
except ImportError:
    pass
