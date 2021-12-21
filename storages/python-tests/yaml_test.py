"""Script to test the DLite plugin 'yaml.py' in Python."""
import os
import shutil
import sys
import uuid

from run_python_tests import print_test_exception

print('Running Python test <yaml_test>...')
cwd = str(os.getcwd()).replace('\\', '/')
input_path = cwd + '/input/'
dlite_path = cwd[:cwd.rfind('storages')]
plugin_path = dlite_path + 'storages/python/python-storage-plugins/'
plugin = plugin_path + 'yaml.py'

# To avoid potential module name conflicts with other YAML packages,
# copy the plugin to a temporary file with a unique name
plugin_copy = 'yaml_' + str(uuid.uuid4()).replace('-', '_')
with open(plugin, 'r') as orig:
    s = orig.read()
    s = s.replace('DLiteStorageBase', 'object')
    with open(cwd + '/' + plugin_copy + '.py', 'w') as cpy:
        cpy.write(s)

try:
    exec('from ' + plugin_copy + ' import yaml as dlite_yaml')
    
    # Test loading YAML metadata
    yaml_inst1 = dlite_yaml()
    yaml_inst1.open(input_path + 'test_meta.yaml')
    inst = yaml_inst1.load('2b10c236-eb00-541a-901c-046c202e52fa')
    print('...Loading metadata ok!')
    
    # Test saving YAML metadata
    yaml_inst2 = dlite_yaml()
    yaml_inst2.open('yaml_test_save.yaml', 'mode=w')
    yaml_inst2.save(inst)
    yaml_inst2.close()
    with open(input_path + 'test_meta.yaml', 'r') as orig:
        orig_yaml = orig.read()
    with open(cwd + '/yaml_test_save.yaml', 'r') as cpy:
        cpy_yaml = cpy.read()
    if cpy_yaml == orig_yaml:
        print('...Saving metadata ok!')
    else:
        raise ValueError('...Saving metadata failed!')
    
    # Test loading YAML data
    yaml_inst3 = dlite_yaml()
    yaml_inst3.open(input_path + 'test_data.yaml')
    inst1 = yaml_inst3.load('204b05b2-4c89-43f4-93db-fd1cb70f54ef')
    inst2 = yaml_inst3.load('e076a856-e36e-5335-967e-2f2fd153c17d')
    print('...Loading data ok!')
    
    # Test saving YAML data
    yaml_inst4 = dlite_yaml()
    yaml_inst4.open('yaml_test_save.yaml', 'mode=w')
    yaml_inst4.save(inst1)
    yaml_inst4.save(inst2)
    yaml_inst4.close()
    with open(input_path + 'test_data.yaml', 'r') as orig:
        orig_yaml = orig.read()
    with open(cwd + '/yaml_test_save.yaml', 'r') as cpy:
        cpy_yaml = cpy.read()
    if cpy_yaml == orig_yaml:
        print('...Saving data ok!')
    else:
        raise ValueError('...Saving data failed!')
    
    print('Test <yaml_test> ran successfully')
except Exception as err:
    print_test_exception(err)
finally:
    # Cleanup
    if os.path.exists(cwd + '/' + plugin_copy + '.py'):
        os.remove(cwd + '/' + plugin_copy + '.py')
    if os.path.exists(cwd + '/yaml_test_save.yaml'):
        os.remove(cwd + '/yaml_test_save.yaml')
    if os.path.isdir(cwd + '/__pycache__'):
        shutil.rmtree(cwd + '/__pycache__')
