/* -*- Python -*-  (not really, but good for syntax highlighting) */

/* Python-spesific extensions to dlite-entity.i */
%pythoncode %{
import sys
import json
import base64
import warnings

from uuid import UUID
if sys.version_info >= (3, 7):
    OrderedDict = dict
else:
    from collections import OrderedDict

import numpy as np


class InvalidMetadataError:
    """Malformed or invalid metadata."""


class InstanceEncoder(json.JSONEncoder):
    def default(self, obj):
        if isinstance(obj, (bytes, bytearray)):
            return obj.hex()
        elif isinstance(obj, np.ndarray):
            if obj.dtype.kind == 'V':
                conv = lambda e: ([conv(ele) for ele in e]
                                  if isinstance(e, list)
                                  else base64.b16encode(e))
                return conv(obj.tolist())
            else:
                return obj.tolist()
        elif hasattr(obj, 'aspreferred'):
            return obj.aspreferred()
        elif hasattr(obj, 'asdict'):
            return obj.asdict()
        elif hasattr(obj, 'asstrings'):
            return obj.asstrings()
        else:
            return json.JSONEncoder.default(self, obj)


class Metadata(Instance):
    """A subclass of Instance for metadata.

    Arguments:
        uri: URI of the new metadata.
        dimensions: Sequence of Dimension instances describing each dimension.
        properties: Sequence of Property instances describing each property.
        description: Description of metadata.
    """
    def __new__(
            cls,
            uri: str,
            dimensions: "Sequence[Dimension]",
            properties: "Sequence[Property]",
            description: str = ''
    ):
        return Instance.create_metadata(
            uri, dimensions, properties, description)

    def __repr__(self):
        return f"<Metadata: uri='{self.uri}'>"

    def getprop(self, name):
        """Returns the metadata property object with the given name."""
        if "properties" not in self.properties:
            raise InvalidMetadataError(
                'self.properties on metadata must contain a "properties" item'
            )
        lst = [p for p in self.properties["properties"] if p.name == name]
        if lst:
            return lst[0]
        raise DLiteError(f"Metadata {self.uri} has no such property: {name}")

    def dimnames(self):
        """Returns a list of all dimension names in this metadata."""
        if "dimensions" not in self.properties:
            return []
        return [d.name for d in self.properties['dimensions']]

    def propnames(self):
        """Returns a list of all property names in this metadata."""
        if "properties" not in self.properties:
            raise InvalidMetadataError(
                'self.properties on metadata must contain a "properties" item'
            )
        return [p.name for p in self.properties['properties']]


def standardise(v, prop, asdict=False):
    """Represent property value `v` as a standard python type.
    If `asdict` is true, dimensions, properties and relations will be
    represented with a dict, otherwise as a list of strings."""
    if asdict:
        conv = lambda x: x.asdict() if hasattr(x, 'asdict') else x
    else:
        conv = lambda x: list(x.asstrings()) if hasattr(x, 'asstrings') else x

    if prop.dtype == dlite.BlobType:
        if prop.ndims:
            V = np.fromiter(
                (''.join(f'{c:02x}' for c in s.item()) for s in v.flat),
                dtype=f'U{2*prop.size}')
            V.shape = v.shape
            return V.tolist()
        else:
            return ''.join(f'{c:02x}' for c in v)
    elif hasattr(v, 'tolist'):
        return [conv(x) for x in v.tolist()]
    else:
        return conv(v)


def get_instance(id: "str", metaid: "str"=None, check_storages: "bool"=True) -> "Instance":
    """Return instance with given id.

    Arguments:
        id: Id of instance to return.
        metaid: If given, dlite will try to convert the instance to a new
            instance of `metaid`.
        check_storages: Whether to check for the instance in storages listed
            in dlite.storage_path if the instance is not already in memory.

    Returns:
        DLite instance.
    """
    if isinstance(id, dlite.Instance):
        inst = id
    else:
        inst = _dlite.get_instance(id, metaid, check_storages)

    if inst is None:
        raise DLiteError(f"no such instance: {id}")
    elif inst.is_meta:
        inst.__class__ = Metadata
    elif inst.meta.uri == _dlite.COLLECTION_ENTITY:
        inst.__class__ = Collection
    return inst

%}


/* ---------
 * Dimension
 * --------- */
%extend _DLiteDimension {
  %pythoncode %{
    def __repr__(self):
        return 'Dimension(name=%r, description=%r)' % (
            self.name, self.description)

    def asdict(self):
        """Returns a dict representation of self."""
        d = OrderedDict()
        d['name'] = self.name
        if self.description:
            d['description'] = self.description
        return d

    def asstrings(self):
        """Returns a representation of self as a tuple of strings."""
        return (self.name,
                '' if self.description is None else self.description)
  %}
}


/* --------
 * Property
 * -------- */
%extend _DLiteProperty {
  %pythoncode %{
    def __repr__(self):
        shape = ', shape=%r' % self.shape.tolist() if self.ndims else ''
        unit = ', unit=%r' % self.unit if self.unit else ''
        descr = ', description=%r' %self.description if self.description else ''
        return 'Property(%r, type=%r%s%s%s)' % (
            self.name, self.type, shape, unit, descr)

    def asdict(self, soft7=True, exclude_name=False):
        """Returns a dict representation of self.

        Args:
            soft7: Whether to use new SOFT7 format.
            exclude_name: Whether to exclude "name" from the returned dict.
        """
        d = OrderedDict()
        if not exclude_name:
            d['name'] = self.name
        d['type'] = self.get_type()
        if self.ndims:
            d['shape' if soft7 else 'dims'] = self.shape.tolist()
        if self.unit:
            d['unit'] = self.unit
        if self.description:
            d['description'] = self.description
        return d

    def asstrings(self):
        """Returns a representation of self as a tuple of strings."""
        return (self.name, self.type, ','.join(str(d) for d in self.shape),
                '' if self.unit is None else self.unit,
                '' if self.description is None else self.description)

    type = property(get_type, doc='Type name.')
    dtype = property(get_dtype, doc='Type number.')
    shape = property(get_shape, set_shape, doc='Array of dimension indices.')

    # Too be removed...
    def _get_dims_depr(self):
        warnings.warn('Property `dims` is deprecated, use `shape` instead.',
                      DeprecationWarning, stacklevel=2)
        return self.get_shape()
    dims = property(_get_dims_depr, doc='Array of dimension indices. '
                    'Property `dims` is deprecated, use `shape` instead.')

  %}
}


/* --------
 * Relation
 * -------- */
%extend _Triple {
  %pythoncode %{
    def __repr__(self):
        return 'Relation(s=%r, p=%r, o=%r, id=%r)' % (
            self.s, self.p, self.o, self.id)

    def aspreferred(self):
        """Returns preferred Python representation."""
        return self.asstrings()

    def asdict(self):
        """Returns a dict representation of self."""
        if self.id:
            d = OrderedDict(s=self.s, p=self.p, o=self.o, id=self.id)
        else:
            d = OrderedDict(s=self.s, p=self.p, o=self.o)
        return d

    def asstrings(self):
        """Returns a representation of self as a tuple of strings."""
        return (self.s, self.p, self.o)
  %}
}


/* --------
 * Instance
 * -------- */

%extend _DLiteInstance {

  int __len__(void) {
    return (int)$self->meta->_nproperties;
  }

  %newobject __repr__;
  char *__repr__(void) {
    int n=0;
    char buff[256];
    n += snprintf(buff+n, sizeof(buff)-n, "<Instance:");
    if ($self->uri && $self->uri[0])
      n += snprintf(buff+n, sizeof(buff)-n, " uri='%s'", $self->uri);
    else
      n += snprintf(buff+n, sizeof(buff)-n, " uuid='%s'", $self->uuid);
    n += snprintf(buff+n, sizeof(buff)-n, ">");
    return strdup(buff);
  }

  %newobject _c_ptr;
  PyObject *_c_ptr(void) {
    return PyCapsule_New($self, NULL, NULL);  // XXX - Add name and descructor
  }

  %pythoncode %{

    # Override default generated __init__() method
    def __init__(self, *args, **kwargs):
        if self is None:
            raise DLiteError(f"invalid dlite.Instance")

        dlite.errclr()
        _dlite.Instance_swiginit(self, _dlite.new_Instance(*args, **kwargs))

        if not hasattr(self, 'this') or not getattr(self, 'this'):
            raise DLiteError(f"cannot initiate dlite.Instance")
        elif self.is_meta:
            self.__class__ = Metadata
        elif self.meta.uri == COLLECTION_ENTITY:
            self.__class__ = Collection

    def get_meta(self):
        """Returns reference to metadata."""
        meta = self._get_meta()
        assert meta.is_meta
        meta.__class__ = Metadata
        return meta

    def get_property_descr(self, name):
        """Return a Property object for property `name`."""
        for p in self.meta['properties']:
            if p.name == name:
                return p
        raise ValueError(f'No property "{name}" in "{self.uri}"')

    meta = property(get_meta, doc="Reference to the metadata of this instance.")
    dimensions = property(
        lambda self: OrderedDict((d.name, int(v))
                                 for d, v in zip(self.meta['dimensions'],
                                                 self.get_dimensions())),
        doc='Dictionary with dimensions name-value pairs.')
    properties = property(lambda self:
        {p.name: self[p.name] for p in self.meta['properties']},
        doc='Dictionary with property name-value pairs.')
    is_data = property(_is_data, doc='Whether this is a data instance.')
    is_meta = property(_is_meta, doc='Whether this is a metadata instance.')
    is_metameta = property(_is_metameta,
                           doc='Whether this is a meta-metadata instance.')

    @classmethod
    def from_metaid(cls, metaid, dims, id=None):
        """Create a new instance of metadata `metaid`.  `dims` must be a
        sequence with the size of each dimension.  All values initialized
        to zero.  If `id` is None, a random UUID is generated.  Otherwise
        the UUID is derived from `id`.
        """
        if isinstance(dims, dict):
            meta = get_instance(metaid)
            dims = [dims[dim.name] for dim in meta.properties['dimensions']]
        # Allow metaid to be an Instance
        if isinstance(metaid, dlite.Instance):
            metaid = metaid.uri
        return Instance(
            metaid=metaid, dims=dims, id=id,
            dimensions=(), properties=()  # arrays must not be None
        )

    @classmethod
    def from_url(cls, url, metaid=None):
        """Load the instance from `url`.  The URL should be of the form
        ``driver://location?options#id``.
        If `metaid` is provided, the instance is tried mapped to this
        metadata before it is returned.
        """
        return Instance(
            url=url, metaid=metaid,
            dims=(), dimensions=(), properties=()  # arrays
        )

    @classmethod
    def from_storage(cls, storage, id=None, metaid=None):
        """Load the instance from `storage`.  `id` is the id of the instance
        in the storage (not required if the storage only contains more one
        instance).
        If `metaid` is provided, the instance is tried mapped to this
        metadata before it is returned.
        """
        return Instance(
            storage=storage, id=id, metaid=metaid,
            dims=(), dimensions=(), properties=()  # arrays
        )

    @classmethod
    def from_location(cls, driver, location, options=None, id=None):
        """Load the instance from storage specified by `driver`, `location`
        and `options`.  `id` is the id of the instance in the storage (not
        required if the storage only contains more one instance).
        """
        return Instance(
            driver=driver, location=str(location), options=options, id=id,
            dims=(), dimensions=(), properties=()  # arrays
        )

    @classmethod
    def from_json(cls, jsoninput, id=None, metaid=None):
        """Load the instance from storage specified by `driver`, `location`
        and `options`.  `id` is the id of the instance in the storage (not
        required if the storage only contains more one instance).
        """
        return Instance(
            jsoninput=jsoninput, id=id, metaid=metaid,
            dims=(), dimensions=(), properties=()  # arrays
        )

    @classmethod
    def create_metadata(cls, uri, dimensions, properties, description):
        """Create a new metadata entity (instance of entity schema) casted
        to an instance.
        """
        return Instance(
            uri=uri, dimensions=dimensions, properties=properties,
            description=description,
            dims=()  # arrays
        )

    @classmethod
    def create_from_metaid(cls, metaid, dims, id=None):
        """Create a new instance of metadata `metaid`.  `dims` must be a
        sequence with the size of each dimension.  All values initialized
        to zero.  If `id` is None, a random UUID is generated.  Otherwise
        the UUID is derived from `id`.
        """
        warnings.warn(
            "create_from_metaid() is deprecated, use from_metaid() instead.",
            DeprecationWarning, stacklevel=2)
        if isinstance(dims, dict):
            meta = get_instance(metaid)
            dims = [dims[dim.name] for dim in meta.properties['dimensions']]
        return Instance(
            metaid=metaid, dims=dims, id=id,
            dimensions=(), properties=()  # arrays must not be None
        )

    @classmethod
    def create_from_url(cls, url, metaid=None):
        """Load the instance from `url`.  The URL should be of the form
        ``driver://location?options#id``.
        If `metaid` is provided, the instance is tried mapped to this
        metadata before it is returned.
        """
        warnings.warn(
            "create_from_url() is deprecated, use from_url() instead.",
            DeprecationWarning, stacklevel=2)
        return Instance(
            url=url, metaid=metaid,
            dims=(), dimensions=(), properties=()  # arrays
        )

    @classmethod
    def create_from_storage(cls, storage, id=None, metaid=None):
        """Load the instance from `storage`.  `id` is the id of the instance
        in the storage (not required if the storage only contains more one
        instance).
        If `metaid` is provided, the instance is tried mapped to this
        metadata before it is returned.
        """
        warnings.warn(
            "create_from_storage() is deprecated, use from_storage() instead.",
            DeprecationWarning, stacklevel=2)
        return Instance(
            storage=storage, id=id, metaid=metaid,
            dims=(), dimensions=(), properties=()  # arrays
        )

    @classmethod
    def create_from_location(cls, driver, location, options=None, id=None):
        """Load the instance from storage specified by `driver`, `location`
        and `options`.  `id` is the id of the instance in the storage (not
        required if the storage only contains more one instance).
        """
        warnings.warn(
            "create_from_location() is deprecated, use from_location() "
            "instead.", DeprecationWarning, stacklevel=2)
        return Instance(
            driver=driver, location=str(location), options=options, id=id,
            dims=(), dimensions=(), properties=()  # arrays
        )

    def save(self, dest, location=None, options=None):
        """Saves this instance to url or storage.

        Call signatures:
          - save(url)
          - save(driver, location, options=None)
          - save(storage)
        """
        if isinstance(dest, Storage):
            self.save_to_storage(storage=dest)
        elif location:
            with Storage(dest, str(location), options) as storage:
                storage.save(self)
        elif isinstance(dest, str):
            self.save_to_url(dest)
        else:
            raise DLiteError('Arguments do not match any call signature')

    def __getitem__(self, ind):
        if isinstance(ind, int):
            return self.get_property_by_index(ind)
        elif self.has_property(ind):
            return self.get_property(ind)
        elif isinstance(ind, int):
            raise IndexError('instance property index out of range: %d' % ind)
        else:
            raise KeyError('no such property: %s' % ind)

    def __setitem__(self, ind, value):
        if isinstance(ind, int):
            self.set_property_by_index(ind, value)
        elif self.has_property(ind):
            self.set_property(ind, value)
        elif isinstance(ind, int):
            raise IndexError('instance property index out of range: %d' % ind)
        else:
            raise KeyError('no such property: %s' % ind)

    def __contains__(self, item):
        return item in self.properties.keys()

    def __getattr__(self, name):
        if name == 'this':
            return object.__getattribute__(self, name)
        d = object.__getattribute__(self, '__dict__')
        if name in d:
            return d[name]
        elif self.has_property(name):
            return _get_property(self, name)
        elif self.has_dimension(name):
            return self.get_dimension_size(name)
        else:
            raise AttributeError('Instance object has no attribute %r' % name)

    def __setattr__(self, name, value):
        if name == 'this':
            object.__setattr__(self, name, value)
        elif _has_property(self, name):
            _set_property(self, name, value)
        else:
            object.__setattr__(self, name, value)

    def __dir__(self):
        return (object.__dir__(self) +
                [name for name in self.properties] +
                [d.name for d in self.meta.properties['dimensions']])

    def __hash__(self):
        return UUID(self.uuid).int

    def __eq__(self, other):
        return self.uuid == other.uuid

    def __str__(self):
        return self.asjson(indent=2)

    def __reduce__(self):
        # ensures that instances can be pickled
        def iterfun(inst):
           for i, prop in enumerate(inst.properties.values()):
               if isinstance(prop, np.ndarray):
                   p = np.zeros_like(prop)
                   p.flat = [p.asdict() if hasattr(p, 'asdict') else p
                             for p in prop]
               else:
                   p = prop.asdict() if hasattr(prop, 'asdict') else prop
               yield i, p
        return (
            Instance.from_metaid,
            (self.meta.uri, list(self.dimensions.values()), self.uuid),
            None,
            None,
            iterfun(self),
        )

    def __call__(self, dims=(), id=None):
        """Returns an uninitiated instance of this metadata."""
        if not self.is_meta:
            raise TypeError('data instances are not callable')
        if isinstance(dims, dict):
            dims = [dims[d.name] for d in self.properties['dimensions']]
        return Instance.from_metaid(self.uri, dims, id)

    def asdict(self, soft7=True, uuid=True):
        """Returns a dict representation of self.

        Args:
            soft7: Whether to structure metadata as SOFT7.
            uuid: Whether to include UUID in the dict.
        """
        d = OrderedDict()
        if uuid:
            d['uuid'] = self.uuid
        if self.uri:
            d['uri'] = self.uri
        d['meta'] = self.meta.uri
        if self.is_meta:
            #d['name'] = self['name']
            #d['version'] = self['version']
            #d['namespace'] = self['namespace']
            d['description'] = self['description']
            if soft7:
                d['dimensions'] = {dim.name: dim.description
                                   for dim in self['dimensions']}
                d['properties'] = {p.name: p.asdict(exclude_name=True)
                                   for p in self['properties']}
            else:
                d['dimensions'] = [dim.asdict() for dim in self['dimensions']]
                d['properties'] = [p.asdict(soft7=False)
                                   for p in self['properties']]
        else:
            d['dimensions'] = self.dimensions
            d['properties'] = {k: standardise(v, self.get_property_descr(k))
                               for k, v in self.properties.items()}
        if self.has_property('relations') and (
                self.is_meta or self.meta.has_property('relations')):
            d['relations'] = self['relations'].tolist()
        return d

    def asjson(self, soft7=True, uuid=True, **kwargs):
        """Returns a JSON representation of self.  Keyword arguments are
        passed to json.dumps()."""
        return json.dumps(self.asdict(soft7=soft7, uuid=uuid),
                          cls=InstanceEncoder, **kwargs)

    # Deprecated methods
    def get_copy(self):
        """Returns a copy of self.

        This method is deprecated.  Use copy() instead.
        """
        warnings.warn(
            DeprecationWarning,
            "Instance.get_copy() is deprecated.  Use Instance.copy() instead."
        )
        return self.copy()
%}

}
