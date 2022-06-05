#!/usr/bin/env python
# -*- coding: utf-8 -*-
from pathlib import Path

import dlite

thisdir = Path(__file__).resolve().parent
outdir = thisdir / 'output'

# Create collection
coll = dlite.Collection('mycoll')

# Add relations
coll.add_relation('cat', 'is-a', 'animal')
coll.add_relation('dog', 'is-a', 'animal')
rel = coll.get_first_relation('dog')
assert rel.s == 'dog'
rel = coll.get_first_relation(p='is-a')
assert rel.s == 'cat'
assert coll.nrelations == 2
rel = coll.get_first_relation(s='no-such-subject')
assert rel is None

# Create instances
url = f'json://{thisdir}/MyEntity.json?mode=r'
e = dlite.Instance.from_url(url)
inst1 = dlite.Instance.from_metaid(e.uri, [3, 2])
inst2 = dlite.Instance.from_metaid(e.uri, (3, 4), 'myinst')

# Add instances
coll.add('inst1', inst1)
coll.add('inst2', inst2)
assert len(coll) == 2
assert coll.has('inst1')
assert not coll.has('inst3')
assert coll.has_id(inst2.uuid)
assert coll.has_id(inst2.uri)
assert not coll.has_id('non-existing-id')

# Save
with dlite.Storage('json', outdir / 'coll0.json', 'mode=w') as s:
    coll.save(s)
coll.save('json', outdir / 'coll1.json', 'mode=w')
coll.save(f'json://{outdir}/coll2.json?mode=w')
coll.save(f'json://{outdir}/coll3.json?mode=w', include_instances=False)
data = []
for i in range(3):
    with open(outdir / f'coll{i}.json') as f:
        data.append(f.read())
assert data[1] == data[0]
assert data[2] == data[0]

# Load
with dlite.Storage('json', outdir / 'coll0.json', 'mode=r') as s:
    coll0 = dlite.Collection.load(s, id='mycoll')
coll1 = dlite.Collection.load('json', outdir / 'coll1.json', 'mode=r',
                              id=coll.uuid)
coll2 = dlite.Collection.load(f'json://{outdir}/coll2.json?mode=r#mycoll')
assert coll0 == coll
assert coll1 == coll
assert coll2 == coll

# Check refcount
assert coll._refcount == 4
del coll0
assert coll._refcount == 3

# Remove relation
assert coll.nrelations == 8
coll.remove_relations('cat')
assert coll.nrelations == 7
rel = coll.get_first_relation('dog')
assert rel.s == 'dog'

# Remove instance
assert len(coll) == 2
coll.remove('inst2')
assert len(coll) == 1
inst1b = coll.get('inst1')
assert inst1b == inst1
assert inst1b != inst2

# Cannot add an instance with an existing label
try:
    coll.add('inst1', inst2)
except dlite.DLiteError:
    pass
else:
    raise RuntimeError('should not be able to replace an existing instance')

coll.add('inst1', inst2, force=True)  # forced replacement

s = str(coll)
print(coll)
