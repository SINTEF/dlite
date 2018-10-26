"""DLite code generator
"""
import os
import re
import json
import argparse


# DLite type table -- should be consistent with the one in src/dlite-types.c
# In addition fix-sized blobs and strings are supported. Examples: blob256,
# string3, string1024, etc...
dlite_types = [
    # typename    dtype             size
    ("bool",     'dliteBool',      'sizeof(bool)'),
    ("int",      'dliteInt',       'sizeof(int)'),
    ("int8",     'dliteInt',       '1'),
    ("int16",    'dliteInt',       '2'),
    ("int32",    'dliteInt',       '4'),
    ("int64",    'dliteInt',       '8'),
    ("uint",     'dliteUInt',      'sizeof(unsigned int)'),
    ("uint8",    'dliteUInt',      '1'),
    ("uint16",   'dliteUInt',      '2'),
    ("uint32",   'dliteUInt',      '4'),
    ("uint64",   'dliteUInt',      '8'),
    ("float",    'dliteFloat',     'sizeof(float)'),
    ("double",   'dliteFloat',     'sizeof(double)'),
    ("float32",  'dliteFloat',     '4'),
    ("float64",  'dliteFloat',     '8'),
    ("string",   'dliteStringPtr', 'sizeof(char *)'),
    ("triplet",  'dliteTriplet',   'sizeof(DLiteTriplet)'),
]


#-------------------------
# Case conversions
#-------------------------
def isUpperCase(s):
    """Returns true if `s` is upper case."""
    return s.isupper()


def isLowerCase(s):
    """Returns true if `s` is lower case."""
    return s.islower()


def isUpperCamel(s):
    """Returns true if `s` is "UpperCamelCase"."""
    return not s.islower() and not s.isupper() and s[0].isupper()


def isLowerCamel(s):
    """Returns true if `s` is "lowerCamelCase"."""
    return not s.islower() and not s.isupper() and s[0].islower()


def toUpperCase(s):
    """Returns `s` converted to upper case with underscores."""
    s1 = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', s)
    return re.sub('([a-z0-9])([A-Z])', r'\1_\2', s1).upper()


def toLowerCase(s):
    """Returns `s` converted to lower case with underscores."""
    s1 = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', s)
    return re.sub('([a-z0-9])([A-Z])', r'\1_\2', s1).lower()


def toUpperCamel(s):
    """Returns `s` converted to "UpperCamelCase"."""
    return ''.join(l[0].upper() + l[1:] for l in s.split('_'))


def toLowerCamel(s):
    """Returns `s` converted to "lowerCamelCase"."""
    x = toUpperCamel(s)
    return x[0].lower() + x[1:]


def toIdentifier(s):
    """Returns `s` converted to a valid identifier."""
    s = str(s).replace('-', '_')
    #s = s.replace('+', '_plus')
    if s[0].isdigit():
        s = '_' + s
    return re.sub('[^a-zA-Z0-9_]', '', s)


def gettype(typename):
    """Returns (dtype, size) pair correspoinging to `typename`."""
    if typename.startswith('blob') and len(typename) > 4:
        return 'dliteBlob', int(typename[4:])
    elif timename.startswith('string') and len(typename) > 6:
        return 'dliteFixString', int(typename[6:])
    else:
        pass



def typeconv_dlite(metatype):
    """Returns the DLite type corresponding to given metadata type."""
    if metatype == 'blob':
        return 'dliteBlob'
    if metatype == 'bool':
        return 'dliteBool'
    elif metatype.startswith('int'):
        return 'dliteInt'
    elif metatype.startswith('uint'):
        return 'dliteUInt'
    elif metatype in ('float', 'double'):
        return 'dliteFloat'
    elif metatype == 'string':
        return 'dliteStringPtr'
    else:
        raise ValueError('unknown metadata type: %s' % metatype)


#-------------------------
# Info about metadata
#-------------------------
class AttrDict(dict):
    def __init__(self, *args, **kwargs):
        super(AttrDict, self).__init__(*args, **kwargs)
        self.__dict__ = self


class Metadata(object):
    """Holds information about metadata.
    """
    def __init__(self, metadict):
        d = metadict
        self._dict = d
        self.props = AttrDict(
            {p['name']: AttrDict(p) for p in d['properties']})
        self.dims = AttrDict(
            {p['name']: AttrDict(p) for p in d['dimensions']})
        name = d['name'].replace('-', '_')
        self.Name = d['name']                # original
        self.name = toLowerCase(name)        # lower_case
        self.NAME = toUpperCase(name)        # UPPER_CASE
        self.nameCamel = toLowerCamel(name)  # lowerCamelCase
        self.NameCamel = toUpperCamel(name)  # UpperCamelCase
        self.version = d['version']
        self.namespace = d['namespace']
        self.description = d.get('description', '')
        self.type = '%s:%s:%s' % (self.Name, self.version, self.namespace)
        self.uri = '%s/%s/%s' % (self.namespace, self.version, self.Name)
        self.dimnames = [p['name'] for p in d['dimensions']]
        self.propnames = [p['name'] for p in d['properties']]
        self.ndims = len(d['dimensions'])
        self.nprops = len(d['properties'])
        self.prop_dtypes = AttrDict(
            {prop: typeconv_dlite(self.props[prop].type)
             for prop in self.propnames})
        self.prop_ndims = AttrDict({prop: len(self.props[prop].dims)
                                    if 'dims' in self.props[prop] else 1
                                    for prop in self.propnames})
        #self.dimdescr = [p.get('description', '') for p in d['dimensions']]
        #self.types = [p['type'] for p in d['properties']]
        #self.descr = [p.get('description', '') for p in d['properties']]

    @classmethod
    def fromstring(cls, s):
        """Returns Metadata object loaded from string `s`."""
        return cls(json.loads(s))

    @classmethod
    def fromfile(cls, fname):
        """Returns Metadata object loaded from file `fname`."""
        if isinstance(fname, str):
            with open(fname, 'r') as f:
                d = json.load(f)
        else:
            d = json.load(f)
        return cls(d)
