/* -*- Python -*-  (not really, but good for syntax highlighting) */

/* Python-spesific extensions to dlite-entity.i */
%pythoncode %{
import sys
import json
import base64

from uuid import UUID
if sys.version_info >= (3, 7):
    OrderedDict = dict
else:
    from collections import OrderedDict

import numpy as np

class InstanceEncoder(json.JSONEncoder):
    def default(self, obj):
        if isinstance(obj, bytearray):
            return base64.b16encode(obj).decode()
        elif isinstance(obj, bytes):
            return obj.decode()
        elif isinstance(obj, np.ndarray):
            if obj.dtype.kind == 'V':
                conv = lambda e: ([conv(ele) for ele in e]
                                  if isinstance(e, list)
                                  else base64.b16encode(e))
                return conv(obj.tolist())
            else:
                return obj.tolist()
        elif hasattr(obj, 'asdict'):
            return obj.asdict()
        elif hasattr(obj, 'asstrings'):
            return obj.asstrings()
        else:
            return json.JSONEncoder.default(self, obj)


def standardise(v, asdict=True):
    """Represent property value `v` as a standard python type.
    If `asdict` is true, dimensions, properties and relations will be
    represented with a dict, otherwise as a list of strings."""
    if asdict:
        conv = lambda x: x.asdict() if hasattr(x, 'asdict') else x
    else:
        conv = lambda x: list(x.asstrings()) if hasattr(x, 'asstrings') else x

    if hasattr(v, 'tolist'):
        return [conv(x) for x in v.tolist()]
    else:
        return conv(v)

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
        d = OrderedDict([('name', self.name)])
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
        dims = ', dims=%r' % self.dims.tolist() if self.ndims else ''
        unit = ', unit=%r' % self.unit if self.unit else ''
        iri = ', iri=%r' % self.iri if self.iri else ''
        descr = ', description=%r' %self.description if self.description else ''
        return 'Property(%r, type=%r%s%s%s%s)' % (
            self.name, self.type, dims, unit, iri, descr)

    def asdict(self):
        """Returns a dict representation of self."""
        d = OrderedDict([('name', self.name),
                         ('type', self.get_type()),
                         ])
        if self.ndims:
            d['dims'] = self.dims.tolist()
        if self.unit:
            d['unit'] = self.unit
        if self.iri:
            d['iri'] = self.iri
        if self.description:
            d['description'] = self.description
        return d

    def asstrings(self):
        """Returns a representation of self as a tuple of strings."""
        return (self.name, self.type, ','.join(str(d) for d in self.dims),
                '' if self.unit is None else self.unit,
                '' if self.iri is None else self.iri,
                '' if self.description is None else self.description)

    type = property(get_type, doc='Type name.')
    dtype = property(get_dtype, doc='Type number.')
    dims = property(get_dims, doc='Array of dimension indices.')
  %}
}


/* --------
 * Relation
 * -------- */
%extend _Triplet {
  %pythoncode %{
    def __repr__(self):
        return 'Relation(s=%r, p=%r, o=%r, id=%r)' % (
            self.s, self.p, self.o, self.id)

    def asdict(self):
        """Returns a dict representation of self."""
        d = OrderedDict(s=self.s, p=self.p, o=self.o, id=self.id)
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
    return $self->meta->nproperties;
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
    return PyCapsule_New($self, NULL, NULL);
  }

  %pythoncode %{
    meta = property(get_meta, doc="Reference to the metadata of this instance.")
    iri = property(get_iri, set_iri,
                   doc="Unique IRI to corresponding concept in an ontology.")
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

    def __getitem__(self, ind):
        if self.has_property(ind):
            return self.get_property(ind)
        elif isinstance(ind, int):
            raise IndexError('instance property index out of range: %d' % ind)
        else:
            raise KeyError('no such property: %s', ind)

    def __setitem__(self, ind, value):
        if self.has_property(ind):
            self.set_property(ind, value)
        elif isinstance(ind, int):
            raise IndexError('instance property index out of range: %d' % ind)
        else:
            raise KeyError('no such property: %s', ind)

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
            Instance,
            (self.meta.uri, list(self.dimensions.values()), self.uuid),
            None,
            None,
            iterfun(self),
        )

    def __call__(self, dims=(), id=None):
        """Returns an uninitiated instance of this metadata."""
        if not self.is_meta:
            raise TypeError('data instances are not callable')
        return Instance(self.uri, dims, id)

    def asdict(self):
        """Returns a dict representation of self."""
        d = OrderedDict()
        d['uuid'] = self.uuid
        d['meta'] = self.meta.uri
        if self.uri:
            d['uri'] = self.uri
        if self.iri:
            d['iri'] = self.iri
        if self.is_meta:
            d['name'] = self['name']
            d['version'] = self['version']
            d['namespace'] = self['namespace']
            d['description'] = self['description']
            d['dimensions'] = [dim.asdict() for dim in self['dimensions']]
            #
            # FIXME: property dimensions should be strings, and not indices
            # into the dimension values.
            #d['properties'] = [p.asdict() for p in self['properties']]
            dimnames = list(self.dimensions.keys())
            props = []
            for prop in self['properties']:
                 p = prop.asdict()
                 if 'dims' in p:
                     p['dims'] = [dimnames[i] for i in p['dims']]
                 props.append(p)
            d['properties'] = props
        else:
            d['dimensions'] = self.dimensions
            d['properties'] = {k: standardise(v)
                               for k, v in self.properties.items()}
        if 'relations' in self:
            d['relations'] = self['relations'].tolist()
        return d

    def asjson(self, **kwargs):
        """Returns a JSON representation of self.  Arguments are passed to
        json.dumps()."""
        return json.dumps(self.asdict(), cls=InstanceEncoder, **kwargs)
  %}

}
