/* -*- c -*-  (not really, but good for syntax highlighting) */

/* Python-spesific extensions to dlite-entity.i */
%pythoncode %{
import sys
import json
import base64
from uuid import UUID
if sys.version_info >= (3, 7):
    from collections import OrderedDict
else:
    OrderedDict = dict

import numpy as np

class BytearrayEncoder(json.JSONEncoder):
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
        else:
            return json.JSONEncoder.default(self, obj)

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
        descr = ', description=%r' %self.description if self.description else ''
        return 'Property(%r, type=%d, size=%d%s%s%s)' % (
            self.name, self.type, self.size, dims, unit, descr)

    def asdict(self):
        """Returns a dict representation of self."""
        d = OrderedDict([('name', self.name),
                         ('type', self.type),
                         ('size', self.size)])
        if self.ndims:
            d['dims'] = self.dims.tolist()
        if self.unit:
            d['unit'] = self.unit
        if self.description:
            d['description'] = self.description
        return d

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
    char buff[64];
    n += snprintf(buff+n, sizeof(buff)-n, "<Instance:");
    if ($self->uri && $self->uri[0])
      n += snprintf(buff+n, sizeof(buff)-n, " uri='%s'", $self->uri);
    else
      n += snprintf(buff+n, sizeof(buff)-n, " uuid='%s'", $self->uuid);
    n += snprintf(buff+n, sizeof(buff)-n, ">");
    return strdup(buff);
  }

  %pythoncode %{
    meta = property(get_meta, doc="Reference to the metadata of this instance.")
    dimensions = property(get_dimensions, doc='Array of dimension sizes.')
    properties = property(lambda self:
        {p.name: self[p.name] for p in self.meta['properties']},
        doc='Dictionary with all properties.')
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

    # TODO - fix __getattr__() and __setattr__() to allow us to work with
    #        properties as attributes...

    #def __getattr__(self, name):
    #    if _has_property(self, name):
    #        return _get_property(self, name)
    #    else:
    #        return _swig_getattr(self, Instance, name)
    #
    #def __setattr__(self, name, value):
    #    if _has_property(self, name):
    #        _set_property(self, name, value)
    #    else:
    #        _swig_setattr(self, Instance, name, value)

    def __hash__(self):
        return UUID(self.uuid).int

    def __eq__(self, other):
        return self.uuid == other.uuid

    def __str__(self):
        return self.asjson(indent=2)

    def asdict(self):
        d = OrderedDict()
        if self.is_meta:
            d['name'] = self['name']
            d['version'] = self['version']
            d['namespace'] = self['namespace']
            d['meta'] = self.meta.uri
            d['description'] = self['description']
            d['dimensions'] = [dim.asdict() for dim in self['dimensions']]
            d['properties'] = [p.asdict() for p in self['properties']]
        else:
            d['meta'] = self.meta.uri
            d['dimensions'] = {dim.name: int(val) for dim, val in
                zip(self.meta['dimensions'], self.dimensions)}
            d['properties'] = self.properties
        if 'relations' in self:
            d['relations'] = self['relations']
        return d

    def asjson(self, **kwargs):
        """Returns a JSON representation of self.  Arguments are passed to
        json.dumps()."""
        return json.dumps(self.asdict(), cls=BytearrayEncoder, **kwargs)

  %}

}
