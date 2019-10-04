"""Simplifies writing DLite storage plugins using Python.

This module implements a data model represented by a Python dict.  The
layout of the dict corresponds to the json layout, which is also
returned by the Instance.asdict() method.

An example of how to use this module can be found in
plugins/python/plugins/yaml_plugin.py
"""
import os
import dlite


class DictDataModel:
    """Implements a data model represented by a Python dict with the same
    layout as returned by Instance.asdict() and in the json representations.

    Parameters
    ----------
    d : dict
        A dict representing the underlying data.  It should have the same
        layout as returned by Instance.asdict().
    """
    def __init__(self, d):
        self.d = d

        obj = d['meta']
        if isinstance(obj, str):
            url = obj
        else:
            url = '/'.join([obj['namespace'], obj['version'], obj['name']])
        self.meta = dlite.get_instance(url)

        #self.is_meta = isinstance(d['dimensions'], list)
        self.is_meta = hasattr(d['dimensions'], 'count')

    def getMetaURI(self):
        """Returns metadata uri for this datamodel."""
        return self.meta.uri

    def getDimensionSize(self, name):
        """Returns size of dimension `name`."""
        if self.is_meta:
            for d in self.d['dimensions']:
                if d['name'] == name:
                    return d
            raise ValueError('No such dimension: %s' % name)
        else:
            return self.d['dimensions'][name]

    def getProperty(self, name, type=None, size=None, ndims=None, dims=None):
        """Returns value of property `name`.

        The expected type, size, number of dimensions and size of each
        dimension of the memory is described by `type`, `size`,
        `ndims` and `dims`, respectively.
        """
        if self.is_meta:
            for p in self.d[properties]:
                if p['name'] == name:
                    return p
            raise ValueError('No such property: %s' % name)
        else:
            return self.d[properties]['name']

    # Optional
    def setMetaURI(self, uri):
        """Sets the metadata uri for this datamodel."""
        self.meta = dlite.get_instance(uri)
        self.d['meta'] = uri

    def setDimensionSize(self, name, size):
        """Sets size of dimension `name` to `size`."""
        if self.is_meta:
            # FIXME: should this be silently ignored?
            raise NotImplementedError(
                'Cannot change dimension size of metadata')
        if name not in self.d['dimensions']:
            raise ValueError('no such dimension name: "%s"' % name)
        self.d['dimensions'][name] = size

    def setProperty(self, name, value, type=None, size=None,
                    ndims=None, dims=None):
        """Sets property `name` to `value`.

        The expected type, size, number of dimensions and size of each
        dimension of the memory is described by `type`, `size`,
        `ndims` and `dims`, respectively."""
        if self.is_meta:
            # FIXME: should this be silently ignored?
            raise NotImplementedError('Cannot change metadata property')
        if name not in self.d['properties']:
            raise ValueError('no such property name: "%s"' % name)
        self.d['properties'][name] = value

    def hasDimension(self, name):  # ???
        """Returns a positive value if dimension `name` is defined, zero if it
        isn't and a negative value on error."""
        if self.is_meta:
            if [True for d in self.d['dimensions'] if name == d['name']]:
                return 1
        else:
            if name in self.d['dimensions']:
                return 1
        return 0

    def hasProperty(self, name):  # ???
        """Returns a positive value if property `name` is defined, zero if it
        isn't and a negative value on error."""
        if self.is_meta:
            if [True for d in self.d['properties'] if name == d['name']]:
                return 1
        else:
            if name in self.d['properties']:
                return 1
        return 0

    def getURI(self):
        """Returns instance uri or None if it is not set."""
        return self.d.get('uri', self.d.get('dataname', None))
