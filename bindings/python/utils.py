import os

import numpy as np
import dlite

thisdir = os.path.dirname(__file__)


def instance_from_dict(d):
    """Returns a new DLite instance created from dict `d`, which should
    be of the same form as returned by the Instance.asdict() method.
    """
    meta = dlite.get_instance(d['meta'])
    dims = np.zeros((meta.ndimensions, ), dtype=int)
    for dim in range(len(d['dimensions'])):
        for prop in meta['properties']:
            if dim in prop.dims:
                k = np.nonzero(prop.dims == dim)[0][0]
                i = prop.dims[k]
                data = d['properties'][prop.name]
                for j in range(i):
                    data = data[0]
                n = len(data)
                if dims[i]:
                    assert dims[i] == n
                else:
                    dims[i] = n
    print("*** creating new instance with id:", d.get('uuid', None))
    inst = dlite.Instance(meta.uri, dims.tolist(), d.get('uuid', None))
    for p in meta['properties']:
        if inst.is_meta and p.name == 'dimensions':
            for dim in inst['dimensions']:
                print(dim.name)
                #dim.name = d['dimensions']['name']
                #dim.description = d['dimensions']['description']
        elif inst.is_meta and p.name == 'properties':
            pass
        else:
            inst[p.name] = d['properties'][p.name]
    return inst



if __name__ == '__main__':

    url = 'json://' + os.path.join(thisdir, 'tests', 'Person.json') #+ "?mode=r"
    Person = dlite.Instance(url)

    person = Person([2])
    person.name = 'Ada'
    person.age = 12.5
    person.skills = ['skiing', 'jumping']

    d1 = person.asdict()
    inst1 = instance_from_dict(d1)


    d2 = Person.asdict()
    inst2 = instance_from_dict(d2)
