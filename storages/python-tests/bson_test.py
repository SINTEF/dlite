"""Script to test the DLite plugin 'bson.py' in Python."""
import os
import shutil
import sys
import uuid

from run_python_tests import print_test_exception

print('Running Python test <bson_test>...')
cwd = str(os.getcwd()).replace('\\', '/')
input_path = cwd + '/input/'
dlite_path = cwd[:cwd.rfind('storages')]
plugin_path = dlite_path + 'storages/python/python-storage-plugins/'
plugin = plugin_path + 'bson.py'

# To avoid potential module name conflicts with other BSON packages,
# copy the plugin to a temporary file with a unique name
plugin_copy = 'bson_' + str(uuid.uuid4()).replace('-', '_')
with open(plugin, 'r') as orig:
    s = orig.read()
    s = s.replace('DLiteStorageBase', 'object')
    with open(cwd + '/' + plugin_copy + '.py', 'w') as cpy:
        cpy.write(s)

try:
    exec('from ' + plugin_copy + ' import bson as dlite_bson')
    
    # Test loading BSON metadata
    bson_inst1 = dlite_bson()
    bson_inst1.open(input_path + 'test_meta.bson')
    inst = bson_inst1.load('2b10c236-eb00-541a-901c-046c202e52fa')
    print('...Loading metadata ok!')
    
    # Test saving BSON metadata
    bson_inst2 = dlite_bson()
    bson_inst2.open('bson_test_save.bson', 'mode=w')
    bson_inst2.save(inst)
    bson_inst2.close()
    with open(input_path + 'test_meta.bson', 'r') as orig:
        orig_bson = orig.read()
    with open(cwd + '/bson_test_save.bson', 'r') as cpy:
        cpy_bson = cpy.read()
    if cpy_bson == orig_bson:
        print('...Saving metadata ok!')
    else:
        raise ValueError('...Saving metadata failed!')
    
    # Test loading BSON data
    bson_inst3 = dlite_bson()
    bson_inst3.open(input_path + 'test_data.bson')
    inst = bson_inst3.load('204b05b2-4c89-43f4-93db-fd1cb70f54ef')
    inst2 = bson_inst3.load('e076a856-e36e-5335-967e-2f2fd153c17d')
    print('...Loading data ok!')
    
    # Test saving BSON data
    bson_inst4 = dlite_bson()
    bson_inst4.open('bson_test_save.bson', 'mode=w')
    bson_inst4.save(inst)
    bson_inst4.save(inst2)
    bson_inst4.close()
    with open(input_path + 'test_data.bson', 'rb') as orig:
        orig_bson = orig.read()
    with open(cwd + '/bson_test_save.bson', 'rb') as cpy:
        cpy_bson = cpy.read()
    if cpy_bson == orig_bson:
        print('...Saving data ok!')
    else:
        raise ValueError('...Saving data failed!')
    
    print('Test <bson_test> ran successfully')
except Exception as err:
    print_test_exception(err)
finally:
    # Cleanup
    if os.path.exists(cwd + '/' + plugin_copy + '.py'):
        os.remove(cwd + '/' + plugin_copy + '.py')
    if os.path.exists(cwd + '/bson_test_save.bson'):
        os.remove(cwd + '/bson_test_save.bson')
    if os.path.isdir(cwd + '/__pycache__'):
        shutil.rmtree(cwd + '/__pycache__')
