#!/usr/bin/env python
# -*- coding: utf-8 -*-
import os

import dlite

thisdir = os.path.abspath(os.path.dirname(__file__))

url = 'json://' + thisdir + '/MyEntity.json'


# Load metadata (i.e. an instance of meta-metadata) from url
s = dlite.Storage(url)
myentity = dlite.Instance.from_storage(
    s, 'http://onto-ns.com/meta/0.1/MyEntity')
del s

with dlite.Storage(url) as s2:
    myentity2 = dlite.Instance.from_storage(
        s2, 'http://onto-ns.com/meta/0.1/MyEntity')


# Create an instance
inst = myentity(dimensions=[2, 3], id='my-data')
inst['a-bool-array'] = True, False

# Test Storage.save()
with dlite.Storage('json', 'tmp.json', 'mode=w') as s:
    s.save(inst)


# Test json
print('--- testing json')
myentity.save('json://myentity.json?mode=w')
inst.save('json://inst.json?mode=w')
del inst
inst = dlite.Instance.from_url(f'json://{thisdir}/inst.json#my-data')



# Test yaml
try:
    import yaml
except ImportError:
    pass
else:
    print('--- testing yaml')
    inst.save('yaml://inst.yaml?mode=w')
    del inst
    inst = dlite.Instance.from_url('yaml://inst.yaml#my-data')

    # test help()
    expected = """\
DLite storage plugin for YAML.

Opens `uri`.

        Arguments:
            uri: A fully resolved URI to the PostgreSQL database.
            options: Supported options:
            - `mode`: Mode for opening.  Valid values are:
                - `a`: Append to existing file or create new file (default).
                - `r`: Open existing file for read-only.
                - `w`: Truncate existing file or create new file.
            - `soft7`: Whether to save using SOFT7 format.
            - `single`: Whether the input is assumed to be in single-entity form.
                  The default (`"auto"`) will try to infer it automatically.
"""
    s = dlite.Storage('yaml', 'inst.yaml', options='mode=a')
    assert s.help().strip() == expected.strip()

    # Test delete()
    assert len(s.get_uuids()) == 1
    s.delete(inst.uri)
    assert len(s.get_uuids()) == 0

    # Test to_bytes()/from_bytes()
    data = inst.to_bytes('yaml')
    data2 = data.replace(b'uri: my-data', b'uri: my-data2')
    inst2 = dlite.Instance.from_bytes('yaml', data2)
    assert inst2.uuid != inst.uuid
    assert inst2.get_hash() == inst.get_hash()

    s.flush()  # avoid calling flush() when the interpreter is teared down



# Test rdf
try:
    print('--- testing rdf')
    inst.save('rdf:db.xml?mode=w;store=file;filename=inst.ttl;format=turtle')
except dlite.DLiteError:
    print('    skipping rdf test')
else:
    #del inst
    # FIXME: read from inst.ttl not db.xml
    inst3 = dlite.Instance.from_url('rdf://db.xml#my-data')
