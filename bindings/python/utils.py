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


def instance_from_dict(d):
    """Returns a new DLite instance created from dict `d`, which should
    be of the same form as returned by the Instance.asdict() method.
    """
    meta = dlite.get_instance(d.get('meta', dlite.ENTITY_SCHEMA))
    if meta.is_metameta:

        if 'uri' in d:
            uri = d['uri']
        else:
            uri = dlite.join_meta_uri(d['name'], d['version'], d['namespace'])

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
                    dims=p.get('dims'),
                    unit=p.get('unit'),
                    description=p.get('description'),
                ))
        elif isinstance(d['properties'], Mapping):
            for k, v in d['properties'].items():
                props.append(dlite.Property(
                    name = k,
                    type = v['type'],
                    dims=v.get('dims'),
                    unit=v.get('unit'),
                    description=v.get('description'),
                ))
        else:
            raise TypeError(
                "`properties` must be either a sequence or a mapping")

        inst = dlite.Instance.create_metadata(uri, dimensions, props,
                              d.get('description'))
    else:
        dims = list(d['dimensions'].values())
        if 'uri' in d.keys():
            arg = d.get('uri', d.get('uuid', None))
        else:
            arg = d.get('uuid', None)
        inst = dlite.Instance.create_from_metaid(meta.uri, dims, arg)
        for p in meta['properties']:
            value = d['properties'][p.name]
            if p.type.startswith('blob'):
                if isinstance(value, str):
                    # Assume that the binary data string is hexadecimal
                    value = bytearray(binascii.unhexlify(value))
                elif isinstance(value, list) and len(value) > 1 \
                        and isinstance(value[1], str):
                    # Assume value = [binary string, encoding]
                    value = bytearray(value[0], value[1])
            if isinstance(inst[p.name], (float, int)):
                # Ensure correct scalar conversion by explicit cast
                inst[p.name] = type(inst[p.name])(value)
            else:
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
