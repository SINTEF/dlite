import os

import dlite

thisdir = os.path.dirname(__file__)


def instance_from_dict(d):
    """Returns a new DLite instance created from dict `d`, which should
    be of the same form as returned by the Instance.asdict() method.
    """
    meta = dlite.get_instance(d['meta'])
    if meta.is_metameta:

        try:
            with dlite.silent:
                inst = dlite.get_instance(d['uri'])
                if inst:
                    return inst
        except dlite.DLiteError:
            pass

        dimensions = [dlite.Dimension(d['name'], d.get('description'))
                      for d in d['dimensions']]
        props = []
        dimmap = {dim['name']: i for i, dim in enumerate(d['dimensions'])}
        for p in d['properties']:
            if 'dims' in p:
                dims = [dimmap[d] for d in p['dims']]
            else:
                dims = None
            props.append(dlite.Property(
                name=p['name'],
                type=p['type'],
                dims=dims,
                unit=p.get('unit'),
                iri=p.get('iri'),
                description=p.get('description')))
        inst = dlite.Instance(d['uri'], dimensions, props, d.get('iri'),
                              d.get('description'))
    else:
        dims = list(d['dimensions'].values())
        inst = dlite.Instance(meta.uri, dims, d.get('uuid', None))
        for p in meta['properties']:
            inst[p.name] = d['properties'][p.name]
    return inst


def get_package_paths():
    return {k:v for k,v in dlite.__dict__.items() if k.endswith('path')}

if __name__ == '__main__':

    url = 'json://' + os.path.join(thisdir, 'tests', 'Person.json')
    Person = dlite.Instance(url)

    person = Person([2])
    person.name = 'Ada'
    person.age = 12.5
    person.skills = ['skiing', 'jumping']

    d1 = person.asdict()
    inst1 = instance_from_dict(d1)

    d2 = Person.asdict()
    inst2 = instance_from_dict(d2)
