#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""A module that makes it easy to add dlite functionality to existing classes.

Class customisations

In order to handle special cases, the following methods may be
defined/overridden in the class that is to be wrapped or a proxyclass
subclass:

    _dlite_get_<name>(self, name)
    _dlite_set_<name>(self, name, value)
    _dlite__new__(cls, inst)

"""
import numpy as np

from .dlite import Instance, Storage


class DLiteProxyError(Exception):
    """Base exception for dlite class proxies."""
    pass

class IncompatibleClassError(DLiteProxyError):
    """DLite proxy cannot be created around a class."""
    pass


class MetaProxy(type):
    """Metaclass for BaseProxy."""
    def __init__(cls, name, bases, attr):
        super().__init__(name, bases, attr)


class BaseProxy(object, metaclass=MetaProxy):
    """Base class for class proxies.  Arguments are passed further to the
    __init__() function of the class we are creating a proxy around."""
    def __init__(self, *args, **kw):
        self._proxy_cls.__init__(self, *args, **kw)
        dims = self._dlite_infer_dimensions()
        self.dlite_inst = Instance(self.dlite_meta.uri, dims)
        self._dlite_assign_properties()

    def _dlite_get(self, name):
        """Returns the value of property `name` from the wrapped opject."""
        if hasattr(self, '_dlite_get_' + name):
            getter = getattr(self, '_dlite_get_' + name)
            value = getter()
        elif hasattr(self, 'get_' + name):
            getter = getattr(self, 'get_' + name)
            value = getter()
        elif name in self.__dict__:
            value = self.__dict__[name]
        else:
            raise IncompatibleClassError('cannot get value of property: %s' %
                                         name)
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
        #meta = self.dlite_meta
        dims = [-1] * len(meta.properties['dimensions'])
        for prop in meta.properties['properties']:
            if prop.ndims:
                #value = self._dlite_get(prop.name)
                value = getter(prop.name)
                arr = np.array(value, copy=False)
                if arr.ndim < prop.ndims:
                    raise ValueError('expected %d dimensions for array '
                                     'property "%s"; got %d' % (
                                         prop.ndims, key, arr.ndim))
                for i in prop.dims:
                    if dims[i] == -1:
                        dims[i] = arr.shape[i]
                    elif dims[i] != arr.shape[i]:
                        raise ValueError(
                            'inconsistent length of dimension "%s"' %
                            meta.properties['dimensions'][i].name)
        if min(dims) < 0:
            raise ValueError('cannot infer all dimensions')
        return dims

    def _dlite_assign_properties(self):
        """Assigns all dlite properties from wrapped object."""
        for prop in self.dlite_meta.properties['properties']:
            name = prop.name
            self.dlite_inst[name] = self._dlite_get(name)

    @classmethod
    def _dlite__new__(cls, inst=None):
        """Class method returning a new uninitialised instance of the class
        that is proxied.

        This method simply returns ``cls.__new__(cls)``.  Override
        this method if the class that is proxied already defines a
        __new__() method.
        """
        return cls.__new__(cls)

    def dlite_assign(self, inst):
        """Assigns the object that is proxied from dlite instance `inst`."""
        if self.dlite_meta.uri != inst.meta.uri:
            raise TypeError('Expected instance of metadata "%s", got "%s"' % (
                self.dlite_meta.uri, inst.meta.uri))
        for prop in self.dlite_meta.properties['properties']:
            name = prop.name
            self._dlite_set(name, inst[name])

    def dlite_load(self, *args):
        """Loads dlite instance from storage and assign self from it.
        The arguments `args` are passed to dlite.Instance()."""
        inst = Instance(*args)
        self._dlite_assign(inst)


def loadproxy(theclass, *args):
    """Returns a proxy for an instance of Python class `theclass`
    initiated from dlite instance or storage.

    If `*args` is a dlite instance, the returned object is initiated form
    it.  Otherwise `*args` is passed to dlite.Instance()
    """
    inst = args[0] if isinstance(args[0], Instance) else Instance(*args)
    cls = classproxy(theclass, meta=inst.meta)
    obj = cls._dlite__new__(inst)
    obj.dlite_assign(inst)
    obj.dlite_meta = inst.meta
    obj.dlite_inst = inst
    return obj


def instanceproxy(obj, meta=None, url=None, storage=None, id=None):
    """Returns a proxy for Python object `obj`.  See classproxy() for
    documentation of the remaining arguments.
    """
    cls = classproxy(obj.__class__, meta=meta, url=url, storage=storage, id=id)
    meta = cls.dlite_meta
    getter = lambda name: cls._dlite_get(obj, name)
    dims = cls._dlite_infer_dimensions(obj, meta=meta, getter=getter)
    inst = Instance(meta.uri, dims)
    for prop in meta.properties['properties']:
        inst[prop.name] = cls._dlite_get(obj, prop.name)
    return loadproxy(obj.__class__, inst)


def classproxy(theclass, meta=None, url=None, storage=None, id=None):
    """Factory function that returns a proxy class for metadata.

    Parameters
    ----------
    theclass : class instance
        The class to create a proxy around.
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
            meta = Instance(url)
        elif storage is not None:
            if isinstance(storage, Storage):
                meta = Instance(storage, id)
            else:
                meta = Instance(*storage, id=id)
        else:
            raise TypeError('`meta`, `url` or `storage` must be provided')

    if not meta.is_meta:
        raise TypeError('`meta` must refer to metadata')

    attr = dict(
        dlite_meta=meta,
        _proxy_cls=theclass,
        __init__=BaseProxy.__init__
    )

    return type(meta.name, (theclass, BaseProxy), attr)
