import os

import dlite

try:
    import bson
except ImportError:
    HAVE_BSON = False
else:
    HAVE_BSON = True

try:
    import yaml
except ImportError:
    HAVE_YAML = False
else:
    HAVE_YAML = True


thisdir = os.path.abspath(os.path.dirname(__file__))

# Test JSON

url = 'json://' + os.path.join(thisdir, 'Person.json')
Person = dlite.Instance.from_url(url)

person = Person(dims=[2])
person.name = 'Ada'
person.age = 12.5
person.skills = ['skiing', 'jumping']


print('=== saving...')
with dlite.Storage('json', 'test.json', 'mode=w') as s:
    s.save(person)


print('=== loading...', person.uuid)
with dlite.Storage('json', 'test.json', 'mode=r') as s:
    inst = s.load(id=person.uuid)


person2 = Person(dims=[3])
person2.name = 'Berry'
person2.age = 24.3
person2.skills = ['eating', 'sleeping', 'reading']
with dlite.Storage('json://test.json') as s:
    s.save(person2)


s = dlite.Storage('json://test.json')
uuids = s.get_uuids()
del s
del uuids

# =====================================================================
# Test the BSON and YAML Python plugins

input_dir = thisdir.replace('bindings', 'storages')
input_dir = input_dir.replace('tests', 'tests-python/input/')

if HAVE_BSON:
    # Test BSON
    print('\n\n=== Test BSON plugin ===')
    meta_file = input_dir + 'test_meta.bson'
    meta_test_file = meta_file.replace('.bson', '_save.bson')
    data_file = input_dir + 'test_data.bson'
    data_test_file = data_file.replace('.bson', '_save.bson')

    print('Test loading metadata...')
    with dlite.Storage('bson', meta_file, 'mode=r') as s:
        inst = s.load('2b10c236-eb00-541a-901c-046c202e52fa')
    print('...Loading metadata ok!')

    print('Test saving metadata...')
    with dlite.Storage('bson', meta_test_file, 'mode=w') as s:
        s.save(inst)
    with dlite.Storage('bson', meta_test_file, 'mode=r') as s:
        inst2 = s.load('2b10c236-eb00-541a-901c-046c202e52fa')
    if inst == inst2:
        print('...Saving metadata ok!')
    else:
        raise ValueError('...Saving metadata failed!')
    os.remove(meta_test_file)
    del inst, inst2

    print('Test loading data...')
    with dlite.Storage('bson', data_file, 'mode=r') as s:
        inst1 = s.load('204b05b2-4c89-43f4-93db-fd1cb70f54ef')
        inst2 = s.load('e076a856-e36e-5335-967e-2f2fd153c17d')
    print('...Loading data ok!')

    print('Test saving data...')
    with dlite.Storage('bson', data_test_file, 'mode=w') as s:
        s.save(inst1)
        s.save(inst2)
    with dlite.Storage('bson', data_test_file, 'mode=r') as s:
        inst3 = s.load('204b05b2-4c89-43f4-93db-fd1cb70f54ef')
        inst4 = s.load('e076a856-e36e-5335-967e-2f2fd153c17d')
    if inst1 == inst3 and inst2 == inst4:
        print('...Saving data ok!')
    else:
        raise ValueError('...Saving data failed!')
    os.remove(data_test_file)
    del inst1, inst2, inst3, inst4
else:
    print('Skip testing BSON plugin - bson not installed')

if HAVE_YAML:
    # Test YAML
    print('\n\n=== Test YAML plugin ===')
    meta_file = input_dir + 'test_meta.yaml'
    meta_test_file = meta_file.replace('.yaml', '_save.yaml')
    data_file = input_dir + 'test_data.yaml'
    data_test_file = data_file.replace('.yaml', '_save.yaml')

    print('Test loading metadata...')
    with dlite.Storage('yaml', meta_file, 'mode=r') as s:
        inst = s.load('2b10c236-eb00-541a-901c-046c202e52fa')
    print('...Loading metadata ok!')

    print('Test saving metadata...')
    with dlite.Storage('yaml', meta_test_file, 'mode=w') as s:
        s.save(inst)
    with dlite.Storage('yaml', meta_test_file, 'mode=r') as s:
        inst2 = s.load('2b10c236-eb00-541a-901c-046c202e52fa')
    if inst == inst2:
        print('...Saving metadata ok!')
    else:
        raise ValueError('...Saving metadata failed!')
    os.remove(meta_test_file)


    print('Test loading data...')
    with dlite.Storage('yaml', data_file, 'mode=r') as s:
        inst1 = s.load('204b05b2-4c89-43f4-93db-fd1cb70f54ef')
        inst2 = s.load('e076a856-e36e-5335-967e-2f2fd153c17d')
    print('...Loading data ok!')

    print('Test saving data...')
    with dlite.Storage('yaml', data_test_file, 'mode=w') as s:
        s.save(inst1)
        s.save(inst2)
    with dlite.Storage('yaml', data_test_file, 'mode=r') as s:
        inst3 = s.load('204b05b2-4c89-43f4-93db-fd1cb70f54ef')
        inst4 = s.load('e076a856-e36e-5335-967e-2f2fd153c17d')
    if inst1 == inst3 and inst2 == inst4:
        print('...Saving data ok!')
    else:
        raise ValueError('...Saving data failed!')
    os.remove(data_test_file)
    del inst1, inst2, inst3, inst4
else:
    print('Skip testing YAML plugin - PyYAML not installed')
