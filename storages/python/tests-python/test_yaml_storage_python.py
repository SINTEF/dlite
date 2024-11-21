"""Script to test the DLite plugin 'yaml.py' in Python."""
import os
import sys
from importlib import util
from pathlib import Path

try:
    import yaml as pyyaml
except ImportError:
    print("yaml not installed, skipping test")
    sys.exit(44)  # skip test


sys.dont_write_bytecode = True
from run_python_storage_tests import print_test_exception


thisfile = Path(__file__)
print(f'Running Python test <{thisfile.name}>...')
thisdir = thisfile.absolute().parent
input_path = thisdir / 'input'
plugin_path = thisdir.parent / 'python-storage-plugins/yaml.py'
spec = util.spec_from_file_location('yaml.py', plugin_path)
yaml_mod = util.module_from_spec(spec)
spec.loader.exec_module(yaml_mod)

try:
    # Test loading YAML metadata
    yaml_inst1 = yaml_mod.yaml()
    yaml_inst1.open(input_path / 'test_meta.yaml')
    inst = yaml_inst1.load('d9910bde-6028-524c-9e0f-e8f0db734bc8')
    print('...Loading metadata ok!')


    # Test saving YAML metadata
    yaml_inst2 = yaml_mod.yaml()
    yaml_inst2.open('yaml_test_save.yaml', 'mode=w;soft7=false;with_meta=true')
    yaml_inst2.save(inst)
    yaml_inst2.flush()
    with open(input_path / 'test_meta.yaml', "r") as f:
        d1 = pyyaml.safe_load(f)
    with open('yaml_test_save.yaml', "r") as f:
        d2 = pyyaml.safe_load(f)
    assert d1 == d2
    print('...Saving metadata ok!')

    # Test loading YAML data
    yaml_inst3 = yaml_mod.yaml()
    yaml_inst3.open(input_path / 'test_data.yaml')
    inst1 = yaml_inst3.load('52522ba5-6bfe-4a64-992d-e9ec4080fbac')
    inst2 = yaml_inst3.load('2f8ba28c-add6-5718-a03c-ea46961d6ca7')
    print('...Loading data ok!')

    # Test saving YAML data
    yaml_inst4 = yaml_mod.yaml()
    yaml_inst4.open('yaml_test_save2.yaml', 'mode=w;with_uuid=false')
    yaml_inst4.save(inst1)
    yaml_inst4.save(inst2)
    yaml_inst4.flush()
    with open(input_path / 'test_data.yaml', "r") as f:
        d1 = pyyaml.safe_load(f)
    with open('yaml_test_save2.yaml', "r") as f:
        d2 = pyyaml.safe_load(f)
    assert d1 == d2
    print('...Saving data ok!')

    print(f'Test <{thisfile.name}> ran successfully')
except Exception as err:
    if __name__ == '<run_path>':
        print_test_exception(err)
    else:
        raise
finally:
    # Cleanup
    if os.path.exists(thisdir / 'yaml_test_save.yaml'):
        os.remove(thisdir / 'yaml_test_save.yaml')
