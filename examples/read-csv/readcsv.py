import os

from config_paths import csvfile
import dlite


csv = dlite.Instance(f'csv:{csvfile}?'
                     'mode=r;'
                     'meta=http://meta.sintef.no/0.1/Eruptions')

#print(csv)
