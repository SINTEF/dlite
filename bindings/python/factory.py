#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""A module that makes it easy to add dlite functionality to existing classes.

Class customisations
--------------------
In order to handle special cases, the following methods may be
defined/overridden in the class that is to be extended:
    _dlite_get_<name>(self, name)
    _dlite_set_<name>(self, name, value)
    _dlite__new__(cls, inst)
"""
import copy

import numpy as np

from .dlite import Instance, Storage


class FactoryError(Exception):
    """Base exception for factory errors."""
    pass

class IncompatibleClassError(FactoryError):
    """Raised if an extended class is not compatible with its dlite
    metadata."""
    pass


class MetaExtension(type):
    """Metaclass for BaseExtension."""
    def __init__(cls, name, bases, attr):
        super().__init__(name, bases, attr)


class BaseExtension(object, metaclass=MetaExtension):
    """Base class for extension.  Except for `dlite_id`, all
    arguments are passed further to the __init__() function of the
    class we are inheriting from.
    If `instanceid` is given, the id of the underlying dlite instance
    will be set to it.
    """
    def __init__(self, *args, instanceid=None, **kw):
        self._theclass.__init__(self, *args, **kw)
        self._dlite_init(instanceid=instanceid)

    def _dlite_init(self, instanceid=None):
        """Initialise the underlying dlite instance.  If `id` is given,
        the id of the underlying dlite instance will be set to it."""
        dims = self._dlite_infer_dimensions()
        self.dlite_inst = Instance.from_metaid(
            self.dlite_meta.uri, dims, instanceid)
        self._dlite_assign_properties()

    def _dlite_get(self, name):
        """Returns the value of property `name` from the wrapped opject."""
        if hasattr(self, '_dlite_get_' + name):
            getter = getattr(self, '_dlite_get_' + name)
            value = getter()
        elif hasattr(self, 'get_' + name):
            getter = getattr(self, 'get_' + name)
            value = getter()
        else:
            value = getattr(self, name)
        return value

    def _dlite_set(self, name, value):
        """Sets value of property `name` in the wrapped opject."""
        if hasattr(self, '_dlite_set_' + name):
            setter = getattr(self, '_dlite_set_' + name)
            setter(value)
        elif hasattr(self, 'set_' + name):
            setter = getattr(self, 'set_' + name)
            setter(value)
        else:
            setattr(self, name, value)

    def _dlite_infer_dimensions(self, meta=None, getter=None):
        """Returns inferred property dimensions from __dict__."""
        if not meta:
            meta = self.dlite_meta
        if not getter:
            getter = self._dlite_get
        dims = [-1] * len(meta.properties['dimensions'])
        dimnames = [d.name for d in meta.properties['dimensions']]
        for prop in meta.properties['properties']:
            if prop.ndims:
                value = getter(prop.name)
                if len(value) > 0:
                    arr = np.array(value, copy=False)
                else:
                    arr = np.zeros([0] * prop.ndims)
                if arr.ndim < prop.ndims:
                    raise ValueError('expected %d dimensions for array '
                                     'property "%s"; got %d' % (
                                         prop.ndims, prop.name, arr.ndim))
                for i, pdim in enumerate(prop.dims):
                    if pdim in dimnames:
                        n = dimnames.index(pdim)
                        if dims[n] == -1:
                            dims[n] = arr.shape[i]
                        elif arr.shape[i] and dims[n] != arr.shape[i]:
                            raise ValueError(
                                'inconsistent length of dimension "%s"; was '
                                '%d but got %d for property %r' % (
                                meta.properties['dimensions'][n].name, dims[n],
                                arr.shape[i], prop.name))
        if min(dims) < 0:
            raise ValueError('cannot infer all dimensions')
        return dims

    def _dlite_assign_properties(self):
        """Assigns all dlite properties from extended object."""
        for prop in self.dlite_meta.properties['properties']:
            name = prop.name
            self.dlite_inst[name] = self._dlite_get(name)

    @classmethod
    def _dlite__new__(cls, inst=None):
        """Class method returning a new uninitialised instance of the class
        that is extended.
        This method simply returns ``cls.__new__(cls)``.  Override
        this method if the extended class already defines a __new__()
        method.
        """
        return cls.__new__(cls)

    def dlite_assign(self, inst):
        """Assigns self from dlite instance `inst`."""
        if self.dlite_meta.uri != inst.meta.uri:
            raise TypeError('Expected instance of metadata "%s", got "%s"' % (
                self.dlite_meta.uri, inst.meta.uri))
        for prop in self.dlite_meta.properties['properties']:
            name = prop.name
            self._dlite_set(name, inst[name])

    def dlite_load(self, *args):
        """Loads dlite instance from storage and assign self from it.
        The arguments `args` are passed to dlite.Instance.from_storage()."""
        inst = Instance.from_storage(*args)
        self._dlite_assign(inst)


def instancefactory(theclass, inst):
    """Returns an extended instance of `theclass` initiated from dlite
    instance `inst`.
    """
    cls = classfactory(theclass, meta=inst.meta)
    obj = cls._dlite__new__(inst)
    obj.dlite_assign(inst)
    obj.dlite_meta = inst.meta
    obj.dlite_inst = inst
    return obj

def instancefactory(theclass, inst):
    """Returns an extended instance of `theclass` initiated from dlite
    instance `inst`.
    """
    cls = classfactory(theclass, meta=inst.meta)
    obj = cls._dlite__new__(inst)
    obj.dlite_assign(inst)
    obj.dlite_meta = inst.meta
    obj.dlite_inst = inst
    return obj



def objectfactory(obj, meta=None, deepcopy=False, cls=None,
                  url=None, storage=None, id=None, instanceid=None):
    """Returns an extended copy of `obj`.  If `deepcopy` is true, a deep
    copy is returned, otherwise a shallow copy is returned.
    By default, the returned object will have the same class as `obj`.  If
    `cls` is provided, the class of the returned object will be set to `cls`
    (typically a subclass of ``obj.__class__``).
    The `url`, `storage` and `id` arguments are passed to classfactory().
    """
    if cls is None:
        cls = classfactory(obj.__class__, meta=meta, url=url,
                           storage=storage, id=id)
    new = copy.deepcopy(obj) if deepcopy else copy.copy(obj)
    new.__class__ = cls
    new._dlite_init(instanceid=instanceid)
    return new


def classfactory(theclass, meta=None, url=None, storage=None, id=None):
    """Factory function that returns a new class that inherits from both
    `theclass` and BaseInstance.
    Parameters
    ----------
    theclass : class instance
        The class to extend.
    meta : Instance
        Metadata instance.
    url : string
        URL referring to the metadata.  Should be of the form
        ``driver://location?options#id``
    storage : Storage | (driver, location, options) tuple
        Storage to load meta from.
    id : string
        A unique id referring to the metadata if `storage` is provided.
    """
    if meta is None:
        if url is not None:
            meta = Instance.from_url(url)
        elif storage is not None:
            if isinstance(storage, Storage):
                meta = Instance.from_storage(storage, id)
            else:
                meta = Instance.from_driver(*storage, id=id)
        else:
            raise TypeError('`meta`, `url` or `storage` must be provided')

    if not meta.is_meta:
        raise TypeError('`meta` must refer to metadata')

    attr = dict(
        dlite_meta=meta,
        _theclass=theclass,
        __init__=BaseExtension.__init__
    )


    return type(meta.name, (theclass, BaseExtension), attr)
