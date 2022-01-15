"""Script to test the DLite plugin 'bson.py' in Python."""
import os
import shutil
import sys
from pathlib import Path

sys.dont_write_bytecode = True
from run_python_tests import print_test_exception


print('Running Python test <bson_test>...')
thisdir = Path(__file__).absolute().parent
input_path = thisdir / 'input'
plugin = thisdir.parent / 'python-storage-plugins/bson.py'

try:
    with open(plugin, 'r') as orig:
        s = orig.read()
    s = s.replace('bson(DLiteStorageBase)', 'dlite_bson(object)')
    exec(s)
    
    # Test loading BSON metadata
    bson_inst1 = dlite_bson()
    bson_inst1.open(input_path / 'test_meta.bson')
    inst = bson_inst1.load('2b10c236-eb00-541a-901c-046c202e52fa')
    print('...Loading metadata ok!')
    
    # Test saving BSON metadata
    bson_inst2 = dlite_bson()
    bson_inst2.open('bson_test_save.bson', 'mode=w')
    bson_inst2.save(inst)
    bson_inst2.close()
    with open(input_path / 'test_meta.bson', 'rb') as orig:
        orig_bson = orig.read()
    with open('bson_test_save.bson', 'rb') as cpy:
        cpy_bson = cpy.read()
    if cpy_bson == orig_bson:
        print('...Saving metadata ok!')
    else:
        raise ValueError('...Saving metadata failed!')
    
    # Test loading BSON data
    bson_inst3 = dlite_bson()
    bson_inst3.open(input_path / 'test_data.bson')
    inst = bson_inst3.load('204b05b2-4c89-43f4-93db-fd1cb70f54ef')
    inst2 = bson_inst3.load('e076a856-e36e-5335-967e-2f2fd153c17d')
    print('...Loading data ok!')
    
    # Test saving BSON data
    bson_inst4 = dlite_bson()
    bson_inst4.open('bson_test_save.bson', 'mode=w')
    bson_inst4.save(inst)
    bson_inst4.save(inst2)
    bson_inst4.close()
    with open(input_path / 'test_data.bson', 'rb') as orig:
        orig_bson = orig.read()
    with open('bson_test_save.bson', 'rb') as cpy:
        cpy_bson = cpy.read()
    if cpy_bson == orig_bson:
        print('...Saving data ok!')
    else:
        raise ValueError('...Saving data failed!')
    
    print('Test <bson_test> ran successfully')
except Exception as err:
    if __name__ == '<run_path>':
        print_test_exception(err)
    else:
        raise
finally:
    # Cleanup
    if os.path.exists(thisdir / 'bson_test_save.bson'):
        os.remove(thisdir / 'bson_test_save.bson')
