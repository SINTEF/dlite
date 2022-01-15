"""Script to test the DLite plugin 'yaml.py' in Python."""
import os
import shutil
import sys
from pathlib import Path

sys.dont_write_bytecode = True
from run_python_tests import print_test_exception


print('Running Python test <yaml_test>...')
thisdir = Path(__file__).absolute().parent
input_path = thisdir / 'input'
plugin = thisdir.parent / 'python-storage-plugins/yaml.py'

try:
    with open(plugin, 'r') as orig:
        s = orig.read()
    s = s.replace('yaml(DLiteStorageBase)', 'dlite_yaml(object)')
    exec(s)
    
    # Test loading YAML metadata
    yaml_inst1 = dlite_yaml()
    yaml_inst1.open(input_path / 'test_meta.yaml')
    inst = yaml_inst1.load('2b10c236-eb00-541a-901c-046c202e52fa')
    print('...Loading metadata ok!')
    
    # Test saving YAML metadata
    yaml_inst2 = dlite_yaml()
    yaml_inst2.open('yaml_test_save.yaml', 'mode=w')
    yaml_inst2.save(inst)
    yaml_inst2.close()
    with open(input_path / 'test_meta.yaml', 'r') as orig:
        orig_yaml = orig.read()
    with open('yaml_test_save.yaml', 'r') as cpy:
        cpy_yaml = cpy.read()
    if cpy_yaml == orig_yaml:
        print('...Saving metadata ok!')
    else:
        raise ValueError('...Saving metadata failed!')
    
    # Test loading YAML data
    yaml_inst3 = dlite_yaml()
    yaml_inst3.open(input_path / 'test_data.yaml')
    inst1 = yaml_inst3.load('204b05b2-4c89-43f4-93db-fd1cb70f54ef')
    inst2 = yaml_inst3.load('e076a856-e36e-5335-967e-2f2fd153c17d')
    print('...Loading data ok!')
    
    # Test saving YAML data
    yaml_inst4 = dlite_yaml()
    yaml_inst4.open('yaml_test_save.yaml', 'mode=w')
    yaml_inst4.save(inst1)
    yaml_inst4.save(inst2)
    yaml_inst4.close()
    with open(input_path / 'test_data.yaml', 'r') as orig:
        orig_yaml = orig.read()
    with open('yaml_test_save.yaml', 'r') as cpy:
        cpy_yaml = cpy.read()
    if cpy_yaml == orig_yaml:
        print('...Saving data ok!')
    else:
        raise ValueError('...Saving data failed!')
    
    print('Test <yaml_test> ran successfully')
except Exception as err:
    if __name__ == '<run_path>':
        print_test_exception(err)
    else:
        raise
finally:
    # Cleanup
    if os.path.exists(thisdir / 'yaml_test_save.yaml'):
        os.remove(thisdir / 'yaml_test_save.yaml')
