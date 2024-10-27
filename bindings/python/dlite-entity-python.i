/* -*- Python -*-  (not really, but good for syntax highlighting) */

/* Python-specific extensions to dlite-entity.i */
%pythoncode %{
import tempfile
import warnings
from typing import Sequence
from pathlib import Path
from urllib.parse import urlparse
from uuid import UUID

import numpy as np


#class InvalidMetadataError:
#    """Malformed or invalid metadata."""


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

    def __init__(self, *args, **kwargs):
        # Do nothing, just avoid calling Instance.__init__()
        #
        # The reason for this is that Instance.__init__() requires that the
        # first argument is a dlite.Instance object ().  All needed
        # instantiation is already done in __new__().
        pass

    def __repr__(self):
        return f"<Metadata: uri='{self.uri}'>"

    def __call__(self, dimensions=(), properties=None, id=None, dims=None):
        """Returns a new instance of this metadata.

        By default the instance is uninitialised, but with the `properties`
        argument it can be either partly or fully initialised.

        Arguments:
            dimensions: Either a dict mapping dimension names to values or
                a sequence of dimension values.
            properties: Dict of property name-property value pairs.  Used
                to initialise the instance (fully or partly).  A KeyError
                is raised if a key is not a valid property name.
            id: Id of the new instance.  The default is to create a
                random UUID.
            dims: Deprecated alias for `dimensions`.

        Returns:
            New instance.
        """
        if not self.is_meta:
            raise TypeError('data instances are not callable')
        if dims is not None:
            warnings.warn(
                "`dims` argument of metadata constructor is deprecated.\n"
                "Use `dimensions` instead.",
                DeprecationWarning,
                stacklevel=2,
            )
            dimensions = dims
        if isinstance(dimensions, dict):
            dimnames = [d.name for d in self.properties['dimensions']]
            dimensions = [dimensions[name] for name in dimnames]

        inst = Instance.from_metaid(self.uri, dimensions, id)
        if isinstance(properties, dict):
            for k, v in properties.items():
                inst[k] = v
        return inst

    # For convenience. Easier to use than self.properties["properties"]
    props = property(
        fget=lambda self: {p.name: p for p in self.properties["properties"]},
        doc="A dict mapping property name to the `Property` object for the "
        "described metadata.",
    )

    def getprop(self, name):
        """Returns the metadata property object with the given name."""
        if "properties" not in self.properties:
            raise _dlite.DLiteInvalidMetadataError(
                'self.properties on metadata must contain a "properties" item'
            )
        lst = [p for p in self.properties["properties"] if p.name == name]
        if lst:
            return lst[0]
        raise _dlite.DLiteKeyError(
            f"Metadata {self.uri} has no such property: {name}")

    def dimnames(self):
        """Returns a list of all dimension names in this metadata."""
        if "dimensions" not in self.properties:
            return []
        return [d.name for d in self.properties['dimensions']]

    def propnames(self):
        """Returns a list of all property names in this metadata."""
        if "properties" not in self.properties:
            raise _dlite.DLiteInvalidMetadataError(
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

    if prop.dtype == BlobType:
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


def get_instance(
    id: str,
    metaid: str = None,
    check_storages: bool = True
) -> "Instance":
    """Return instance with given id.

    Arguments:
        id: ID of instance to return.
        metaid: If given, dlite will try to convert the instance to a new
            instance of ``metaid``.
        check_storages: Whether to check for the instance in storages listed
            in dlite.storage_path if the instance is not already in memory.

    Returns:
        DLite Instance.
    """
    if isinstance(id, Instance):
        inst = id
    else:
        id = str(id).rstrip("#/")
        if metaid and not isinstance(metaid, Instance):
            metaid = str(metaid).rstrip("#/")
        inst = _dlite.get_instance(id, metaid, check_storages)

    if inst is None:
        raise _dlite.DLiteMissingInstanceError(f"no such instance: {id}")
    return instance_cast(inst)

%}


/* ---------
 * Dimension
 * --------- */
%extend _DLiteDimension {
  %pythoncode %{
    def __repr__(self):
        return 'Dimension(name=%r, description=%r)' % (
            self.name, self.description)

    def __eq__(self, other):
        """Returns true if `other` equals self."""
        return self.asdict() == other.asdict()

    def asdict(self):
        """Returns a dict representation of self."""
        d = {}
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
        ref = ', ref=%r' % self.ref if self.ref else ''
        shape = ', shape=%r' % self.shape.tolist() if self.ndims else ''
        unit = ', unit=%r' % self.unit if self.unit else ''
        descr = ', description=%r' %self.description if self.description else ''
        return 'Property(%r, type=%r%s%s%s%s)' % (
            self.name, self.type, ref, shape, unit, descr)

    def __eq__(self, other):
        """Returns true if `other` equals self."""
        return self.asdict() == other.asdict()

    def asdict(self, soft7=True, exclude_name=False):
        """Returns a dict representation of self.

        Args:
            soft7: Whether to use new SOFT7 format.
            exclude_name: Whether to exclude "name" from the returned dict.
        """
        d = {}
        if not exclude_name:
            d['name'] = self.name
        d['type'] = self.get_type()
        if self.ref:
            d['ref'] = self.ref
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
        args = [f"s='{self.s}', p='{self.p}', o='{self.o}'"]
        if self.d:
            args.append(f"d='{self.d}'")
        if self.id:
            args.append(f"id='{self.id}'")
        return f"Relation({', '.join(args)})"

    def __eq__(self, other):
        if isinstance(other, Relation):
            return (self.s == other.s and self.p == other.p and
                    self.o == other.o and self.d == other.d)
        return NotImplemented

    def copy(self):
        """Returns a copy of self."""
        return Relation(s=self.s, p=self.p, o=self.o, d=self.d, id=self.id)

    def aspreferred(self):
        """Returns preferred Python representation."""
        return self.asstrings()

    def asdict(self):
        """Returns a dict representation of self."""
        d = dict(s=self.s, p=self.p, o=self.o)
        if self.id:
            d[id] = self.id
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

        # Some versions of SWIG may generate a __new__() method that
        # is not a standard wrapper function and will therefore bypass
        # the standard error checking.  Check manually that we are not
        # in an error state.
        if hasattr(self, "__new__"):
            _dlite.errcheck()

        if self is None:
            raise _dlite.DLitePythonError(f"cannot create dlite.Instance")

        obj = _dlite.new_Instance(*args, **kwargs)

        # The swig-internal Instance_swiginit() function is not a standard
        # wrapper function and therefore bypass the standard error checking.
        # Therefore, check manually that it doesn't produce an error.
        _dlite.Instance_swiginit(self, obj)
        _dlite.errcheck()

        if not hasattr(self, 'this') or not getattr(self, 'this'):
            raise _dlite.DLitePythonError(f"cannot initiate dlite.Instance")
        instance_cast(self)

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
        raise ValueError(
            f'No property "{name}" in '
            f'"{self.uri if self.uri else self.meta.uri}"'
        )

    meta = property(get_meta, doc="Reference to the metadata of this instance.")
    dimensions = property(
        lambda self: dict((d.name, int(v))
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
    def from_metaid(cls, metaid, dimensions, id=None):
        """Create a new instance of metadata `metaid`.  `dimensions` must be a
        sequence with the size of each dimension.  All values initialized
        to zero.  If `id` is None, a random UUID is generated.  Otherwise
        the UUID is derived from `id`.
        """
        if isinstance(dimensions, dict):
            meta = get_instance(metaid)
            dimensions = [dimensions[dim.name]
                          for dim in meta.properties['dimensions']]
        # Allow metaid to be an Instance
        if isinstance(metaid, Instance):
            metaid = metaid.uri
        inst = Instance(
            metaid=metaid, dims=dimensions, id=id,
            dimensions=(), properties=()  # arrays must not be None
        )
        return instance_cast(inst)

    @classmethod
    def load(
        cls, protocol, driver, location, options=None, id=None, metaid=None
    ):
        """Load the instance from storage:

        Arguments:
            protocol: Name of protocol plugin used for data transfer.
            driver: Name of storage plugin for data parsing.
            location: Location of resource.  Typically a URL or file path.
            options: Options passed to the protocol and driver plugins.
            id: ID of instance to load.
            metaid: If given, the instance is tried mapped to this metadata
                before it is returned.

        Return:
            Instance loaded from storage.
        """
        from dlite.protocol import Protocol

        with Protocol(protocol, location=location, options=options) as pr:
            buffer = pr.load(uuid=id)
        try:
            return cls.from_bytes(
                driver, buffer, id=id, options=options, metaid=metaid
            )
        except _dlite.DLiteUnsupportedError:
            pass
        tmpfile = None
        try:
            with tempfile.NamedTemporaryFile(delete=False) as f:
                tmpfile = f.name
                f.write(buffer)
            inst = cls.from_location(
                driver, tmpfile, options=options, id=id, metaid=metaid
            )
        finally:
            Path(tmpfile).unlink()
        return instance_cast(inst)

    @classmethod
    def from_url(cls, url, metaid=None):
        """Load the instance from `url`.  The URL should be of one of the
        the following forms:

            driver://location?options#id
            protocol+driver://location?options#id
            protocol://location?driver=<driver>;options#id

        where `protocol`, `driver`, `location`, `options` and `id` are
        documented in the load() method.

        If `metaid` is provided, the instance is tried mapped to this
        metadata before it is returned.
        """
        from dlite.protocol import Protocol
        from dlite.options import parse_query

        p = urlparse(url)
        if "driver=" in p.query or "+" in p.scheme:
            if "driver=" in p.query:
                protocol = p.scheme
                driver = parse_query(p.query)["driver"]
            elif "+" in p.scheme:
                protocol, driver = p.scheme.split("+", 1)
            elif Path(p.path).suffix:
                protocol = p.scheme
                driver = Path(p.path).suffix[1:]
            else:
                raise _dlite.DLiteParseError(
                    f"cannot infer driver from URL: {url}"
                )
            location = f"{protocol}://{p.netloc}{p.path}"
            inst = cls.load(
                protocol, driver, location, options=p.query, id=p.fragment,
                metaid=metaid
            )
        else:
            inst = Instance(
                url=url, metaid=metaid,
                dims=(), dimensions=(), properties=()  # arrays
            )
        return instance_cast(inst)

    @classmethod
    def from_storage(cls, storage, id=None, metaid=None):
        """Load the instance from `storage`.  `id` is the id of the instance
        in the storage (not required if the storage only contains more one
        instance).
        If `metaid` is provided, the instance is tried mapped to this
        metadata before it is returned.
        """
        inst = Instance(
            storage=storage, id=id, metaid=metaid,
            dims=(), dimensions=(), properties=()  # arrays
        )
        return instance_cast(inst)

    @classmethod
    def from_location(
        cls, driver, location, options=None, id=None, metaid=None
    ):
        """Load the instance from storage specified by `driver`, `location`
        and `options`.  `id` is the id of the instance in the storage (not
        required if the storage only contains more one instance).
        """
        from dlite.options import make_query
        if options and not isinstance(options, str):
            options = make_query(options)
        inst = Instance(
            driver=driver, location=str(location), options=options, id=id,
            metaid=metaid,
            dims=(), dimensions=(), properties=()  # arrays
        )
        return instance_cast(inst)

    @classmethod
    def from_json(cls, jsoninput, id=None, metaid=None):
        """Load the instance from json input."""
        inst = Instance(
            jsoninput=jsoninput, id=id, metaid=metaid,
            dims=(), dimensions=(), properties=()  # arrays
        )
        return instance_cast(inst)

    @classmethod
    def from_bson(cls, bsoninput):
        """Load the instance from bson input."""
        inst = Instance(
            bsoninput=bsoninput,
            dims=(), dimensions=(), properties=()  # arrays
        )
        return instance_cast(inst)

    @classmethod
    def from_dict(cls, d, id=None, single=None, check_storages=True):
        """Load the instance from dictionary.

        Arguments:
            d: Dict to parse.  It should be of the same form as returned
                by the Instance.asdict() method.
            id: Identity of the returned instance.

                If `d` is in single-entity form with no explicit 'uuid' or
                'uri', its identity will be assigned by `id`.  Otherwise
                `id` must be consistent with the 'uuid' and/or 'uri'
                fields of `d`.

                If `d` is in multi-entity form, `id` is used to select the
                instance to return.
            single: A boolean, None or "auto".  Determines whether to
                assume that the dict is in single-entity form.
                If `single` is None or "auto", the form is inferred.
            check_storages: Whether to check if the instance already exists
                in storages specified in `dlite.storage_path`.

        Returns:
            New instance.
        """
        from dlite.utils import instance_from_dict
        inst = instance_from_dict(
            d, id=id, single=single, check_storages=check_storages,
        )
        return instance_cast(inst)

    @classmethod
    def from_bytes(cls, driver, buffer, options=None, id=None, metaid=None):
        """Load the instance with ID `id` from bytes `buffer` using the
        given storage driver.
        """
        from dlite.options import make_query
        if options and not isinstance(options, str):
            options = make_query(options)
        inst = _from_bytes(
            driver, buffer, id=id, options=options, metaid=metaid
        )
        return instance_cast(inst)

    @classmethod
    def create_metadata(cls, uri, dimensions, properties, description):
        """Create a new metadata entity (instance of entity schema) casted
        to an instance.
        """
        inst = Instance(
            uri=uri, dimensions=dimensions, properties=properties,
            description=description,
            dims=()  # arrays
        )
        return instance_cast(inst)

    @classmethod
    def create_from_metaid(cls, metaid, dimensions, id=None):
        """Create a new instance of metadata `metaid`.  `dimensions` must be a
        sequence with the size of each dimension.  All values initialized
        to zero.  If `id` is None, a random UUID is generated.  Otherwise
        the UUID is derived from `id`.
        """
        warnings.warn(
            "create_from_metaid() is deprecated, use from_metaid() instead.",
            DeprecationWarning, stacklevel=2)
        if isinstance(dimensions, dict):
            meta = get_instance(metaid)
            dimensions = [dimensions[dim.name]
                          for dim in meta.properties['dimensions']]
        inst = Instance(
            metaid=metaid, dims=dimensions, id=id,
            dimensions=(), properties=()  # arrays must not be None
        )
        return instance_cast(inst)

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
        inst = Instance(
            storage=storage, id=id, metaid=metaid,
            dims=(), dimensions=(), properties=()  # arrays
        )
        return instance_cast(inst)

    @classmethod
    def create_from_location(cls, driver, location, options=None, id=None):
        """Load the instance from storage specified by `driver`, `location`
        and `options`.  `id` is the id of the instance in the storage (not
        required if the storage only contains more one instance).
        """
        warnings.warn(
            "create_from_location() is deprecated, use from_location() "
            "instead.", DeprecationWarning, stacklevel=2)
        from dlite.options import make_query
        if options and not isinstance(options, str):
            options = make_query(options)
        inst = Instance(
            driver=driver, location=str(location), options=options, id=id,
            dims=(), dimensions=(), properties=()  # arrays
        )
        return instance_cast(inst)

    def save(self, *dest, location=None, options=None):
        """Saves this instance to url or storage.

        Call signatures:
          - save(storage)
          - save(url)
          - save(driver, location, options=None)
          - save(protocol, driver, location, options=None)

        Arguments:
            storage: A dlite.Storage instance to store the instance to.
            url: A URL for the storate to store to.
            protocol: Name of protocol plugin to use for transferring the
                serialised data to `location`.
            driver: Name of storage plugin for serialisation.
            location: A string describing where to save the instance.
            options: Options to the protocol and driver plugins. Should be
                a semicolon- or ampersand-separated string of key=value pairs.

        Notes:
            The URL may be given in any of the following forms:

                driver://location?options#id
                protocol+driver://location?options#id
                protocol://location?driver=<driver>;options#id

        """
        from dlite.protocol import Protocol
        from dlite.options import make_query, parse_query

        if options and not isinstance(options, str):
            options = make_query(options)

        # Assign arguments from call signature.
        # Far too complicated, but ensures backward compatibility.
        storage = url = protocol = driver = None
        if isinstance(dest, Storage):
            storage = dest
        elif isinstance(dest, str):
            if location:
                driver = dest
            else:
                url = dest
        elif isinstance(dest, Sequence):
            if len(dest) == 1:
                if isinstance(dest[0], Storage):
                    storage, = dest
                elif location:
                    driver, = dest
                else:
                    url, = dest
            if len(dest) == 2:
                if location:
                    protocol, driver = dest
                else:
                    driver, location = dest
            elif len(dest) == 3:
                if not location and options is None:
                     arg1, arg2, arg3 = dest
                     if arg2 is None and arg3 is None:
                         url = arg1
                     else:
                         driver, location, options = dest
                elif not location:
                     protocol, driver, location = dest
                else:
                    raise _dlite.DLiteValueError(
                        "dlite.Instance.save() got `location` both as "
                        f"positional ({dest[2]}) and keyword ({location}) "
                        "argument"
                    )
            elif len(dest) == 4:
                if location or options:
                    raise _dlite.DLiteValueError(
                        "dlite.Instance.save() got `location` and/or "
                        "`options` both as positional and keyword arguments"
                    )
                protocol, driver, location, options = dest

        # Call lower-level save methods
        if protocol:
            try:
                buf = self.to_bytes(driver, options=options)
            except (_dlite.DLiteAttributeError, _dlite.DLiteUnsupportedError):
                buf = None
            if not buf:
                try:
                    with tempfile.NamedTemporaryFile(delete=False) as f:
                        tmpfile = f.name
                    self.save(driver, location=tmpfile, options=options)
                    with open(tmpfile, "rb") as f:
                        buf = f.read()
                finally:
                    Path(tmpfile).unlink()
            with Protocol(protocol, location=location, options=options) as pr:
                pr.save(buf)
        elif driver:
            with Storage(driver, str(location), options) as storage:
                storage.save(self)
        elif url:
            protocol = driver = None
            scheme, loc, options, _ = split_url(url)
            if "+" in scheme:
                protocol, driver = scheme.split("+", 1)
            elif scheme in Protocol.loaded_plugins():
                protocol = scheme
            else:
                driver = scheme
            if "driver=" in options:
                driver = parse_query(options)["driver"]
            elif not driver:
                suffix = Path(loc).suffix
                if suffix:
                    driver = suffix[1:]
                else:
                    raise _dlite.DLiteParseError(
                        f"cannot infer driver from URL: {url}"
                    )
            if protocol:
                self.save(protocol, driver, location=loc, options=options)
            else:
                opts = f"?{options}" if options else ""
                self.save_to_url(f"{driver}://{loc}{opts}")
        elif storage:
            self.save_to_storage(storage=storage)
        else:
            raise _dlite.DLiteTypeError(
                'Arguments to save() do not match any of the call signatures'
            )

    def __getitem__(self, ind):
        if isinstance(ind, int):
            value = self.get_property_by_index(ind)
        elif self.has_property(ind):
            value = _get_property(self, ind)
        elif isinstance(ind, int):
            raise _dlite.DLiteIndexError(
                'instance property index out of range: %d' % ind
            )
        else:
            raise _dlite.DLiteKeyError('no such property: %s' % ind)
        if isinstance(value, np.ndarray) and self.is_frozen():
            value.flags.writeable = False  # ensure immutability
        return value

    def __setitem__(self, ind, value):
        if self.is_frozen():
            raise _dlite.DLiteUnsupportedError(
                f'frozen instance does not support assignment of property '
                f'{ind}')
        if isinstance(ind, int):
            self.set_property_by_index(ind, value)
        elif self.has_property(ind):
            _set_property(self, ind, value)
        elif isinstance(ind, int):
            raise _dlite.DLiteIndexError(
                'instance property index out of range: %d' % ind
            )
        else:
            raise _dlite.DLiteKeyError('no such property: %s' % ind)

    def __contains__(self, item):
        return item in self.properties.keys()

    def __getattr__(self, name):
        if name == 'this':
            return object.__getattribute__(self, name)
        d = object.__getattribute__(self, '__dict__')
        if name in d:
            value = d[name]
        elif self.has_property(name):
            value = _get_property(self, name)
        elif self.has_dimension(name):
            value = self.get_dimension_size(name)
        else:
            raise _dlite.DLiteAttributeError(
                'Instance object has no attribute %r' % name
            )
        if isinstance(value, np.ndarray) and self.is_frozen():
            value.flags.writeable = False  # ensure immutability
        return value

    def __setattr__(self, name, value):
        if name == 'this':
            object.__setattr__(self, name, value)
        elif self.is_frozen():
            raise _dlite.DLiteUnsupportedError(
                f"frozen instance does not support assignment of property "
                f"'{name}'"
            )
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
        return self.asjson()

    def copy(self, newid=None):
        """Returns a copy of this instance.  If `newid` is given, it
        will be the id of the new instance, otherwise it will be given
        a random UUID."""
        newinst = self._copy(newid=newid)
        return instance_cast(newinst)

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

    def asdict(self, soft7=True, uuid=None, single=None):
        """Returns a dict representation of self.

        Arguments:
            soft7: Whether to structure metadata as SOFT7.
            uuid: Whether to include UUID in the dict.  The default is true
                if `single=True` and URI is None, otherwise it is false.
            single: Whether to return in single-entity format.
                If None, single-entity format is used for metadata and
                multi-entity format for data instances.
        """
        dct = d = {}
        if single is None:
            single = self.is_meta

        if uuid is None:
            uuid = single and self.uri

        if not single:
            d = {}
            dct[self.uuid] = d

        if uuid:
            d['uuid'] = self.uuid
        if self.uri:
            d['uri'] = self.uri
        d['meta'] = self.meta.uri
        if self.is_meta:
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
        return dct

    def asjson(self, indent=0, single=None,
               urikey=False, with_uuid=False, with_meta=False,
               with_arrays=False, no_parent=False,
               soft7=None, uuid=None):
        """Returns a JSON representation of self.

        Arguments:
            indent: Number of spaces to indent each line with.
            single: Whether to return single-entity format.  Default is
                single-entity format for metadata and multi-entity format
                for data instances.
            urikey: Use uri (if it exists) as JSON key in multi-entity format.
            with_uuid: Whether to include UUID in output.
            with_meta: Always include "meta" field, even for entities.
            with_arrays: Write metadata dimensions and properties as jSON
                arrays (old format).
            no_parent: Do not include transaction parent info.
            soft7: Whether to structure metadata as SOFT7. Deprecated.
                Use `with_arrays=False` instead.
            uuid: Alias for `with_uuid`. Deprecated.
        """
        if single is None:
            single = self.is_meta

        if soft7 is not None:
            warnings.warn(
                "`soft7` argument of asjson() is deprecated, use "
                "`with_arrays` (negated) instead.",
                 DeprecationWarning, stacklevel=2)
            with_arrays = not bool(soft7)

        if uuid is not None:
            warnings.warn(
                "`uuid` argument of asjson() is deprecated, use "
                "`with_uuid` instead.",
                 DeprecationWarning, stacklevel=2)
            with_uuid = bool(uuid)

        return self._asjson(indent=indent, single=single, urikey=urikey,
                            with_uuid=with_uuid, with_meta=with_meta,
                            with_arrays=with_arrays, no_parent=no_parent)


    # Deprecated methods
    def tojson(self, indent=0, single=False,
               urikey=False, with_uuid=False, with_meta=False,
               with_arrays=False, no_parent=False,
               soft7=None, uuid=None):
        """Deprecated alias for asjson()."""
        warnings.warn(
            'tojson() is deprecated, use asjson() instead.',
             DeprecationWarning, stacklevel=2)
        return self._asjson(indent=indent, single=single, urikey=urikey,
                            with_uuid=with_uuid, with_meta=with_meta,
                            with_arrays=with_arrays, no_parent=no_parent)

    def get_copy(self):
        """Returns a copy of self.

        This method is deprecated.  Use copy() instead.
        """
        warnings.warn(
            "Instance.get_copy() is deprecated.  Use Instance.copy() instead.",
            DeprecationWarning, stacklevel=2)
        return self.copy()

    @property
    def q(self):
        """ to work with quantities """
        from dlite.quantity import get_quantity_helper
        return get_quantity_helper(self)

    def get_quantity(self, name):
        return self.q[name]

    def set_quantity(self, name, value, unit):
        self.q[name] = (value, unit)

%}
}
