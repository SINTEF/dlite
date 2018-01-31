"""Common stuff for DLite code generators
"""
import re
import json

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
        self.Name = d['name']                     # original
        self.name = toLowerCase(d['name'])        # lower_case
        self.NAME = toUpperCase(d['name'])        # UPPER_CASE
        self.nameCamel = toLowerCamel(d['name'])  # lowerCamelCase
        self.NameCamel = toUpperCamel(d['name'])  # UpperCamelCase
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
