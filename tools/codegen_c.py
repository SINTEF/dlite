#/usr/bin/env python
"""DLite code generator for C

Usage: codegen_c entity.json
"""
from __future__ import print_function
from __future__ import division

import textwrap

import codegen

ident = codegen.toIdentifier


# Maps metadata types to C types
typeconv_C = {
    'string': 'char *',
}




class HeaderMetadata(codegen.Metadata):
    """Metadata subclass for C."""

    def __init__(self, jsondoc):
        super(HeaderMetadata, self).__init__(jsondoc)
        self.struct_props = self.get_struct_props()
        self.struct_dims = self.get_struct_dims()
        self.description = '\n * '.join(textwrap.wrap(self.description))
        self.args_dims = ', '.join(['size_t %s' % ident(d)
                                    for d in self.dimnames])
        self.args2_dims = ', '.join(['%s' % ident(d) for d in self.dimnames])
        self.names_dims = ', '.join(['"%s"' % ident(d) for d in self.dimnames])

        self.getter_declarations = self.get_getter_declarations()
        self.setter_declarations = self.get_setter_declarations()
        self.getter2_declarations = self.get_getter2_declarations()

        self.create_assign_proptypes = self.get_create_assign_proptypes()
        self.create_dims = self.get_create_dims()
        self.create_props = self.get_create_props()

        self.free_stringptr = self.get_free_stringptr()
        self.free_props = self.get_free_props()

        self.load_define_dimensions = self.get_load_define_dimensions()
        self.load_define_prop_dims = self.get_load_define_prop_dims()
        self.load_assign_prop_dims = self.get_load_assign_prop_dims()
        self.load_free_prop_dims = self.get_load_free_prop_dims()
        self.load_get_properties = self.get_load_get_properties()

        self.save_set_properties = self.get_save_set_properties()
        self.save_set_dimensions = self.get_save_set_dimensions()
    def get_header(self):
        """Returns content of C header file."""
        return template_h.format(**self.__dict__)

    def get_source(self):
        """Returns content of C source file."""
        return template_c.format(**self.__dict__)

    def _get_type(self, prop, npointers=0, dimref=True):
        """Returns C type of property `prop` (incl. final space).

        If `npointers` is larger than zero, additional `npointers`
        stars (*) will be appended to the type.

        If `dimref` is true, an additional star (*) will be added if
        `prop` has dimensions.
        """
        ptype = typeconv_C.get(self.props[prop].type, self.props[prop].type)
        if not ptype.endswith('*'):
            ptype += ' '
        if npointers > 0:
            ptype += '*' * npointers
        if dimref and 'dims' in self.props[prop]:
            ptype += '*'
        return ptype

    def _get_multidim_props(self):
        """Returns a list with names of properties with more than 1
        dimension."""
        return [prop for prop in self.propnames
                if 'dims' in self.props[prop] and
                len(self.props[prop].dims) > 1]

    def get_define_prop_dims(self, prop):
        """Returns definition of variable holding dimension of `prop`."""
        if not 'dims' in self.props[prop]:
            return ''
        ndims = len(self.props[prop]['dims'])
        return 'size_t *%s_dims = malloc(%d*sizeof(size_t));' % (prop, ndims)

    def get_assign_prop_dims(self, prop):
        """Returns definition of variable holding dimension of `prop`."""
        if not 'dims' in self.props[prop]:
            return ''
        return '\n'.join('  %s_dims[%d] = self->dims.%s;' % (prop, i, dim)
                         for i, dim in enumerate(self.props[prop]['dims']))

    def get_free_prop_dims(self, prop):
        """Returns definition of variable holding dimension of `prop`."""
        if not 'dims' in self.props[prop]:
            return ''
        return 'free(%s_dims);' % prop

    def get_struct_props(self):
        """Returns elements of properties struct."""
        ptypes = ['  %s%s;' % (self._get_type(prop), ident(prop))
                  for prop in self.propnames]
        n = max(len(t) for t in ptypes)
        comments = []
        for p in self._dict['properties']:
            if 'description' in p:
                if 'dims' in p:
                    c = '%s  %s' % (
                        p['description'], [ident(d) for d in p['dims']])
                else:
                    c = p['description']
                comments.append('  /* %s */' % ('\n     ' + ' ' * n).join(
                    textwrap.wrap(c, 70 - n)))
            else:
                comments.append('')
        return '\n'.join('%-*s%s' % (n, t, c) for t, c in zip(ptypes, comments))

    def get_struct_dims(self):
        """Returns elements of dimensions struct."""
        dims = self._dict['dimensions']
        types = ['  size_t %s;' % ident(d['name']) for d in dims]
        comments = ['  /* %s */' % d['description']
                    if 'description' in d else '' for d in dims]
        n = max(len(t) for t in types)
        return '\n'.join('%-*s%s' % (n, t, c) for t, c in zip(types, comments))

    def get_setter_declarations(self):
        """Returns setter function declarations."""
        setters = ['void %s_property_set_%s('
                   '%s_s *self, const %svalue, size_t size);' % (
                       self.name, prop, self.name, self._get_type(prop))
                   for prop in self.propnames]
        return '\n'.join(setter for setter in setters)

    def get_getter_declarations(self):
        """Returns getter function declarations."""
        getters = ['void %s_property_get_%s('
                   '%s_s *self, %svalue, size_t size);' % (
                       self.name, prop, self.name, self._get_type(prop, 1))
                   for prop in self.propnames]
        return '\n'.join(getter for getter in getters)

    def get_getter2_declarations(self):
        """Returns getter function declarations for returning property."""
        getters = ['%s%s_get_%s(%s_s *self);' % (
            self._get_type(prop), self.name, prop, self.name)
                   for prop in self.propnames]
        return '\n'.join(getter for getter in getters)

    def get_create_assign_proptypes(self):
        """Returns string with DLite property types."""
        return '\n'.join('  self->proptypes[%d] = %s;' % (
            i, self.prop_dtypes[prop]) for i, prop in enumerate(self.propnames))

    def get_create_dims(self):
        """Returns assignment of dimensions in {name}_create()."""
        lines = ['  self->dims.%s = %s;' % (dim, dim) for dim in self.dimnames]
        return '\n'.join(line for line in lines)

    def get_create_props(self):
        """Returns assignment of properties in {name}_create()."""
        lines = []
        for prop in self.propnames:
            if 'dims' in self.props[prop]:
                nmemb = '*'.join(self.props[prop]['dims'])
                lines.append('  self->props.%s = calloc(%s, sizeof(%s));' % (
                    prop, nmemb, self._get_type(prop)))
        return '\n'.join(line for line in lines)

    def get_free_stringptr(self):
        """Returns free strings in {name}_free()."""
        lines = []
        for prop in self.propnames:
            if self.prop_dtypes[prop] == 'dliteStringPtr':
                p = self.props[prop]
                if 'dims' in p:
                    lines.append('  n = %s;' % '*'.join(
                        'self->dims.%s' % dim for dim in p.dims))
                    lines.append('  for (i=0; i<n; i++)')
                    lines.append('    if (self->props.%s[i])' % prop)
                    lines.append('      free(self->props.%s[i]);' % prop)
                else:
                    lines.append('  if (self->props.%s) free(self->props.%s);'
                                 % (prop, prop))
        return '\n'.join(lines)

    def get_free_props(self):
        """Returns free statements in {name}_free()."""
        return '\n'.join('  free(self->props.%s);' % prop
                         for prop in self.propnames
                         if 'dims' in self.props[prop])

    def get_load_define_dimensions(self):
        """Returns definitions of dimensions in {name}_load()."""
        return '\n'.join(
            '  int %s = dlite_datamodel_get_dimension_size(d, "%s");' % (
                dim, dim) for dim in self.dimnames)

    def get_load_define_prop_dims(self):
        """Returns definitions of property dimensions in {name}_load()."""
        return '\n'.join('  %s' % self.get_define_prop_dims(prop)
                         for prop in self._get_multidim_props())

    def get_load_assign_prop_dims(self):
        """Returns assignment of property dimensions in {name}_load()."""
        return '\n'.join('%s' % self.get_assign_prop_dims(prop)
                         for prop in self._get_multidim_props())

    def get_load_free_prop_dims(self):
        """Free allocated memory for property dimensions in {name}_load()."""
        return '\n'.join('  %s' % self.get_free_prop_dims(prop)
                         for prop in self._get_multidim_props())

    def get_load_get_properties(self, action='get'):
        """Returns statements for setting properties in {name}_load()."""
        lines = []
        for prop in self.propnames:
            p = self.props[prop]
            if 'dims' in p and len(p.dims) > 1:
                dims = '%d, %s_dims' % (len(p.dims), prop)
            elif 'dims' in p:
                dims = '1, &self->dims.%s' % p.dims[0]
            else:
                dims = '1, NULL'
            ref = '&' if 'dims' not in p else ''
            ctype = self._get_type(prop, dimref=False)
            dtype = self.prop_dtypes[prop]
            if dtype == 'dliteString':
                size = 'strlen(self->props.%s)' % prop
            else:
                size = 'sizeof(%s)' % ctype
            lines.append('  dlite_datamodel_%s_property(d, "%s", '
                         '%sself->props.%s, %s, sizeof(%s), %s);' % (
                             action, prop, ref, prop, dtype, ctype, dims))

        return '\n'.join(lines)

    def get_save_set_properties(self):
        return self.get_load_get_properties(action='set')

    def get_save_set_dimensions(self):
        return '\n'.join(
            '  dlite_datamodel_set_dimension_size(d, "%s", self->dims.%s);' % (
                dim, dim) for dim in self.dimnames)




template_h = """\
/* This is a generated with DLite codegen_c -- do not edit! */

/* {description} */
#ifndef _{NAME}_H
#define _{NAME}_H

#include "integers.h"
#include "boolean.h"

typedef struct _{name}_s {name}_s;

/* {Name} dimensions */
typedef struct {{
{struct_dims}
}} {name}_dimensions_s;

/* {Name} properties

   NOTE about strings:
   All strings are initialised to NULL and should be allocated with
   malloc() before use.  All strings that are not NULL will be free'ed
   by {name}_free().  Hence, if you free a string yourself, you must
   set the corresponding pointer to NULL to avoid calling free() twice. */
typedef struct {{
{struct_props}
}} {name}_properties_s;


/* ??? */
/* {name}_s *{name}_create0(const char *id); */

/* Returns a pointer to a new allocated {name}_s instance or NULL on error.
   The id is assigned to a random UUID.
   All fields are initialised with zeros. */
{name}_s *{name}_create({args_dims});

/* Returns a pointer to a new allocated {name}_s instance or NULL on error.
   All fields are initialised with zeros. */
{name}_s *{name}_create_with_id({args_dims}, const char *id);

/* Returns a {name}_s instance loaded from storage or NULL on error.
   `id` is the UUID of the instance to load. */
{name}_s *{name}_load(DLiteStorage *s, const char *id);

/* Saves a {name}_s instance to storage. */
int {name}_save(const {name}_s *self, DLiteStorage *s);

/* Frees a {name}_s instance. */
void {name}_free({name}_s *self);

/* Get pointers for direct access to properties and dimensions. Do not free. */
{name}_properties_s *{name}_props({name}_s *self);
const {name}_properties_s *{name}_const_props({name}_s *self);
const {name}_dimensions_s *{name}_dims({name}_s *self);

/* Getter functions that returns the property value */
/*
{getter2_declarations}
*/

/* Setter and getter functions */
/*
{setter_declarations}

{getter_declarations}
*/
#endif /* _{NAME}_H */
"""


template_c = """\
/* This is a generated with DLite codegen_c -- do not edit! */

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "dlite.h"
#include "dlite-datamodel.h"
#include "{name}.h"

/*
static const char {NAME}_META_TYPE[] = "{type}";
static const char {NAME}_META_NAME[] = "{name}";
static const char {NAME}_META_VERSION[] = "{version}";
static const char {NAME}_META_NAMESPACE[] = "{namespace}";
static const char *static_dims[] = {{{names_dims}}};
*/

struct _{name}_s {{
  const char *id;                  /* Instance UUID */
  /* DLiteType proptypes[{nprops}]; */         /* Property types */
  {name}_dimensions_s dims;     /* Dimensions */
  {name}_properties_s props;    /* Properties */
}};


/* Returns a pointer to a new allocated {name}_s instance or NULL on error.
   The id is assigned to a random UUID.
   All fields are initialised with zeros. */
{name}_s *{name}_create({args_dims})
{{
  return {name}_create_with_id({args2_dims}, NULL);
}}

/* Returns a pointer to a new allocated {name}_s instance or NULL on error.
   All fields are initialised with zeros. */
{name}_s *{name}_create_with_id({args_dims}, const char *id)
{{
  char uuid[DLITE_UUID_LENGTH+1];

  {name}_s *self = calloc(1, sizeof({name}_s));
  dlite_get_uuid(uuid, id);
  self->id = strdup(uuid);

  /* {{create_assign_proptypes}} -- not used, removed for now... */

{create_dims}
{create_props}

  return self;
}}

/* Free's a {name}_s instance. */
void {name}_free({name}_s *self)
{{
  int n, i;
  (void)n;  /* get rid of compiler warnings about unused variable */
  (void)i;  /* get rid of compiler warnings about unused variable */
  free((char *)self->id);

  /* free strings allocated on heap */
{free_stringptr}

  /* free arrays */
{free_props}

  free(self);
}}

/* Returns a new {name}_s instance loaded from storage or NULL on error.
   `id` is the UUID of the instance to load. */
{name}_s *{name}_load(DLiteStorage *s, const char *id)
{{
  DLiteDataModel *d = dlite_datamodel(s, id);
{load_define_dimensions}
  {name}_s *self = {name}_create_with_id({args2_dims}, id);

{load_define_prop_dims}
{load_assign_prop_dims}

{load_get_properties}

  dlite_datamodel_free(d);
{load_free_prop_dims}

  return self;
}}

/* Saves a {name}_s instance to storage. */
int {name}_save(const {name}_s *self, DLiteStorage *s)
{{
  DLiteDataModel *d = dlite_datamodel(s, self->id);

{load_define_prop_dims}
{load_assign_prop_dims}

  dlite_datamodel_set_meta_uri(d, "{uri}");
{save_set_dimensions}

{save_set_properties}

  dlite_datamodel_free(d);
{load_free_prop_dims}

  return 0;
}}


/* Returns a pointer for direct access to properties.  Do not free. */
{name}_properties_s *{name}_props({name}_s *self)
{{
  return &self->props;
}}

/* Returns a const pointer for direct access to properties.  Do not free. */
const {name}_properties_s *{name}_const_props({name}_s *self)
{{
  return (const {name}_properties_s *)&self->props;
}}

/* Returns a pointer for direct access to dimensions.  Do not free. */
const {name}_dimensions_s *{name}_dims({name}_s *self)
{{
  return (const {name}_dimensions_s *)&self->dims;
}}

"""


def main():
    import os
    import argparse
    import re

    parser = argparse.ArgumentParser(
        description='Generates C code (SOFT-like) from json entity.')
    parser.add_argument(
        'infile', metavar='JSONFILE',
        help='Name of input json metadata file.')
    parser.add_argument(
        '--destination', '-d', metavar='DIR',
        help='Default destination directory.')
    parser.add_argument(
        '--header', '-H',
        help='Name of output header file. Defaults to JSONFILE with (version '
        'and) extension replaced with ".h".')
    parser.add_argument(
        '--source', '-c',
        help='Name of output source file. Defaults to JSONFILE with (version '
        'and) extension replaced with ".c".')
    args = parser.parse_args()

    basename = os.path.splitext(args.infile)[0].lower()
    m = re.match(r'(.*)-[0-9._]+$', basename)
    if m:
        basename = m.groups(0)[0]
    if args.destination:
        basename = os.path.join(args.destination, os.path.basename(basename))

    meta = HeaderMetadata.fromfile(args.infile)

    hfile = args.header if args.header else basename + '.h'
    with open(hfile, 'w') as f:
        f.write(meta.get_header())

    cfile = args.source if args.source else basename + '.c'
    with open(cfile, 'w') as f:
        f.write(meta.get_source())



if __name__ == '__main__':
    main()
    #meta = HeaderMetadata.fromfile(
    #    '../../../calm/entities/Chemistry-0.1.json')
    #print(meta.get_header())
    #print(meta.get_source())
