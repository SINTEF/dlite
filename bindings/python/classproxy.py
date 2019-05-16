#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
A module that makes it easy to add dlite functionality to existing classes.


"""
import numpy as np

from .dlite import Instance, Storage


#class MetaProxy(type):
#    def __new__(cls, theclass, meta, id=None):
#        print('*** meta:', repr(meta))
#        if isinstance(meta, str):
#            print('    is string')
#            meta = Instance(meta)
#        elif isinstance(meta, Storage):
#            print('    is Storage')
#            meta = Instance(meta, id)
#        elif not isinstance(meta, Instance):
#            raise TypeError('`meta` must be string, Storage or Instance')
#
#        if not meta.is_meta:
#            raise TypeError('`meta` must refer to metadata')
#
#        name = '%s(%s)' % (cls.__name__, theclass.__name__)
#        attr = dict(
#            dlite_meta=meta,
#            _proxy_cls=theclass,
#        )
#        return type(name, (cls, ), attr)
#
#class ClassProxy(metaclass=MetaProxy):
#    pass




class MetaProxy(type):
    #def __init__(cls, *args, **kw):
    def __init__(cls, name, bases, attr):
        print('*** MetaProxy(name=%r, bases=%r, attr=%r)' % (name, bases, attr))
        print('***    cls:', cls)
        super().__init__(name, bases, attr)
        if 'dlite_meta' in attr:
            meta = attr['dlite_meta']
            cls.dlite_name = meta.name
            cls.dlite_version = meta.version
            cls.dlite_namespace = meta.namespace
            cls.dlite_description = meta.properties['description']
            cls.dlite_dimensions = meta.dimensions
            cls.dlite_properties = meta.properties
            cls.dlite_uri = meta.uri
            cls.dlite_uuid = meta.uuid
            cls.dlite_asjson = meta.asjson
            cls.dlite_save_url = meta.save_url


class BaseProxy(object, metaclass=MetaProxy):
    """Base class for class proxies."""
    def __init__(self, *args, **kw):
        self._proxy_cls.__init__(self, *args, **kw)
        print('*** BaseProxy(%s, %s)' % (args, kw))
        dims = self.dlite_infer_dimensions()
        self.dlite_inst = Instance(self.dlite_meta.uri, dims)
        self.dlite_assign_properties()

    def dlite_infer_dimensions(self):
        """Returns inferred property dimensions from __dict__."""
        meta = self.dlite_meta
        dims = [-1] * len(meta.properties['dimensions'])
        props = {p.name: p for p in meta.properties['properties']}
        for key, val in self.__dict__.items():
            if key in props:
                prop = props[key]
                if prop.ndims:
                    arr = np.array(val, copy=False)
                    if arr.ndim < prop.ndims:
                        raise ValueError('expected %d dimensions for array '
                                         'property "%s"; got %d' % (
                                             prop.ndims, key, arr.ndim))
                    for i in props[key].dims:
                        if dims[i] == -1:
                            dims[i] = arr.shape[i]
                        elif dims[i] != arr.shape[i]:
                            raise ValueError(
                                'inconsistent length of dimension "%s"' %
                                meta.properties['dimensions'][i].name)
        if min(dims) < 0:
            raise ValueError('cannot infer dimensions')
        return dims

    def dlite_assign_properties(self):
        """Assigns dlite properties from __dict__."""
        meta = self.dlite_meta
        for prop in meta.properties['properties']:
            name = prop.name
            if name in self.__dict__:
                self.dlite_inst[name] = self.__dict__[name]


        #self.dlite_inst = Instance(dims, )



def classproxy(theclass, meta, id=None):
    """Factory function that returns a proxy class for metadata `meta`.

    Parameters
    ----------
    theclass : class instance
        The class to create a proxy around.
    meta : Instance | Storage | string
        Reference to metadata
    id : string

    """
    if isinstance(meta, str):
        meta = Instance(meta)
    elif isinstance(meta, storage):
        meta = Instance(meta, id)
    elif not isinstance(meta, Instance):
        raise TypeError('`meta` must be string, Storage or Instance')

    if not meta.is_meta:
        raise TypeError('`meta` must refer to metadata')

    attr = dict(
        dlite_meta=meta,
        _proxy_cls=theclass,
        __init__=BaseProxy.__init__
    )
    #attr = dict(dlite_metadata=meta,
    #            dlite_name=meta.name,
    #            dlite_version=meta.version,
    #            dlite_namespace=meta.namespace,
    #            dlite_asdict=meta.asdict,
    #            dlite_asjson=meta.asjson,
    #            dlite_dimensions=meta.dimensions,
    #            dlite_get_dimensions=meta.get_dimensions,
    #            dlite_get_meta=meta.get_meta,
    #            dlite_get_property=meta.get_property,
    #            dlite_has_property=meta.has_property,
    #            dlite_is_data=meta.is_data,
    #            dlite_is_meta=meta.is_meta,
    #            dlite_is_metameta=meta.is_metameta,
    #            dlite_meta=meta.meta,
    #            dlite_propertyes=meta.properties,
    #            dlite_refcount=meta.refcount,
    #            dlite_save_url=meta.save_url,
    #            dlite_set_property=meta.set_property,
    #            dlite_uri=meta.uri,
    #            dlite_uuid=meta.uuid,
    #            )

    return type(meta.name, (theclass, BaseProxy), attr)
