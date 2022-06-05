import binascii
import os
from typing import Sequence, Mapping
import json
from typing import Dict, List, Optional

try:
    # dataclasses is a rather new feature of Python, lets not require it...
    from dataclasses import dataclass, is_dataclass, asdict
except:
    HAVE_DATACLASSES = False
else:
    HAVE_DATACLASSES = True

try:
    # pydantic is a third party library, lets not require it...
    from pydantic import BaseModel, AnyUrl
except:
    HAVE_PYDANTIC = False
else:
    HAVE_PYDANTIC = True

import dlite


def instance_from_dict(d, id=None, single=None, check_storages=True):
    """Returns a new DLite instance created from dict.

    Parameters
    ----------
    d: dict
        Dict to parse.  It should be of the same form as returned
        by the Instance.asdict() method.
    id: str
        Identity of the returned instance.

        If `d` is in single-entity form with no explicit 'uuid' or
        'uri', its identity will be assigned by `id`.  Otherwise
        `id` must be consistent with the 'uuid' and/or 'uri'
        fields of `d`.

        If `d` is in multi-entity form, `id` is used to select the
        instance to return.
    single: bool | None | "auto"
        whether the dict is assumed to be in single-entity form
        If `single` is None or "auto", the form is inferred.
    """
    if single is None or single == 'auto':
        single = True if 'properties' in d else False

    if single:
        if not id and 'uuid' not in d and 'uri' not in d:
            if 'namespace' in d and 'version' in d and 'name' in d:
                id = f"{d['namespace']}/{d['version']}/{d['name']}"
            else:
                raise ValueError('`id` required for dicts in single-entry '
                                 'form with no explicit uuid or uri.')
    else:
        if not id:
            if len(d) == 1:
                id, = d.keys()
            else:
                raise ValueError('`id` required for dicts in multi-entry form.')
        if id in d:
            return instance_from_dict(d[id], id=id, single=True,
                                      check_storages=check_storages)
        else:
            uuid = dlite.get_uuid(id)
            if uuid in d:
                return instance_from_dict(d[uuid], id=id, single=True,
                                          check_storages=check_storages)
            else:
                raise ValueError(f'no such id in dict: {id}')

    if 'uri' in d or 'uuid' in d:
        if 'uri' in d and 'uuid' in d:
            if dlite.get_uuid(d['uri']) != d['uuid']:
                raise dlite.DLiteError('uri and uuid in dict are not consistent')
        uuid = dlite.get_uuid(d.get('uuid', d.get('uri')))
        if id:
            if dlite.get_uuid(id) != uuid:
                raise ValueError(f'`id` is not consistent with uri/uuid in dict')

    meta = dlite.get_instance(d.get('meta', dlite.ENTITY_SCHEMA))

    if meta.is_metameta:
        if 'uri' in d:
            uri = d['uri']
        else:
            uri = dlite.join_meta_uri(d['name'], d['version'], d['namespace'])

        if check_storages:
            try:
                with dlite.silent:
                    inst = dlite.get_instance(uri)
                    if inst:
                        return inst
            except dlite.DLiteError:
                pass

        if isinstance(d['dimensions'], Sequence):
            dimensions = [dlite.Dimension(d['name'], d.get('description'))
                          for d in d['dimensions']]
        elif isinstance(d['dimensions'], Mapping):
            dimensions = [dlite.Dimension(k, v)
                          for k, v in d['dimensions'].items()]
        else:
            raise TypeError(
                "`dimensions` must be either a sequence or a mapping")

        props = []
        if isinstance(d['properties'], Sequence):
            for p in d['properties']:
                props.append(dlite.Property(
                    name=p['name'],
                    type=p['type'],
                    dims=p.get('shape', p.get('dims')),
                    unit=p.get('unit'),
                    description=p.get('description'),
                ))
        elif isinstance(d['properties'], Mapping):
            for k, v in d['properties'].items():
                props.append(dlite.Property(
                    name = k,
                    type = v['type'],
                    dims=v.get('shape', v.get('dims')),
                    unit=v.get('unit'),
                    description=v.get('description'),
                ))
        else:
            raise TypeError(
                "`properties` must be either a sequence or a mapping")

        inst = dlite.Instance.create_metadata(uri, dimensions, props,
                              d.get('description'))
    else:
        dims = [d['dimensions'][dim.name]
                for dim in meta.properties['dimensions']]
        inst_id = d.get('uri', d.get('uuid', id))
        inst = dlite.Instance.from_metaid(meta.uri, dims=dims, id=inst_id)
        for p in meta['properties']:
            value = d['properties'][p.name]
            inst[p.name] = value

    return inst


def to_metadata(obj):
    """Converts `obj` to dlite Metadata."""
    if isinstance(obj, dlite.Instance):
        if obj.is_data:
            raise TypeError("data instances cannot be converted to metadata")
        return obj

    if isinstance(obj, dict):
        d = obj
    elif isinstance(obj, str):
        d = json.loads(obj)
    elif HAVE_DATACLASSES and is_dataclass(obj):
        if isinstance(obj, type):
            raise NotImplementedError(
                "only instances of dataclasses can be converted to metadata"
            )
        d = asdict(obj)
    elif HAVE_PYDANTIC and isinstance(obj, BaseModel):
        if isinstance(obj, type):
            raise NotImplementedError(
                "only pydandic model instances can be converted to metadata"
            )
        d = obj.dict()
    else:
        raise TypeError('obj can be dict, json string, dataclasses instance '
                        f'or pydantic instance.  Got {type(obj)}')
    return instance_from_dict(d)


if HAVE_DATACLASSES:
    def get_dataclass_entity_schema():
        """Returns the datamodel for dataclasses in Python standard library."""

        @dataclass
        class Property:
            type: str
            dims: Optional[List[str]]
            unit: Optional[str]
            description: Optional[str]

        @dataclass
        class EntitySchema:
            uri: str
            description: Optional[str]
            dimensions: Dict[str, str]
            properties: Dict[str, Property]

        return EntitySchema


if HAVE_PYDANTIC:
    def get_pydantic_entity_schema():
        """Returns the datamodel for dataclasses in Python standard library."""

        class Property(BaseModel):
            type: str
            dims: Optional[List[str]]
            unit: Optional[str]
            description: Optional[str]

        class EntitySchema(BaseModel):
            uri: AnyUrl
            description: Optional[str]
            dimensions: Dict[str, str]
            properties: Dict[str, Property]

        return EntitySchema


def get_package_paths():
    return {k: v for k, v in dlite.__dict__.items() if k.endswith('_path')}
