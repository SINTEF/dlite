#/usr/bin/env python
"""DLite code generator for C

Usage: codegen_c entity.json
"""
import textwrap

import codegen

ident = codegen.toIdentifier


typeconv = {
    'string': 'char *',
}



class HeaderMetadata(codegen.Metadata):
    """Metadata subclass for C."""

    def __init__(self, jsondoc):
        super(HeaderMetadata, self).__init__(jsondoc)
        self.struct_props = self.get_struct_props()
        self.struct_dims = self.get_struct_dims()
        self.description = '\n * '.join(textwrap.wrap(self.description))
        self.args_dims = ', '.join(['int %s' % ident(d['name'])
                                    for d in self._dict['dimensions']])
        self.getter_declarations = self.get_getter_declarations()
        self.setter_declarations = self.get_setter_declarations()

    def get_header(self):
        """Returns content of C header file."""
        return template_h.format(**self.__dict__)

    def get_source(self):
        """Returns content of C source file."""
        return template_c.format(**self.__dict__)

    def _get_type(self, prop):
        """Returns type of property `prop` (incl. final space)."""
        ptype = typeconv.get(self.props[prop].type, self.props[prop].type)
        if not ptype.endswith('*'):
            ptype += ' '
        if 'dims' in self.props[prop]:
            ptype += '*'
        return ptype

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
                    textwrap.wrap(c, 72 - n)))
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

    def get_getter_declarations(self):
        """Returns getter function declarations."""
        getters = ['%s%s_get_%s(%s_s *self);' % (
            self._get_type(prop), self.name, prop, self.name)
                   for prop in self.propnames]
        return '\n'.join(getter for getter in getters)

    def get_setter_declarations(self):
        """Returns setter function declarations."""
        return ''



template_h = """\
/* This is a generated with DLite codegen_c -- do not edit! */

/* {description} */
#ifndef _{NAME}_H
#define _{NAME}_H

#include "integer.h"
#include "boolean.h"

typedef struct _{name}_s {name}_s;

/* Dimensions */
typedef struct {{
{struct_dims}
}} {name}_dimensions_s;

/* Properties */
typedef struct {{
{struct_props}
}} {name}_properties_s;


/* ??? */
/* {name}_s *{name}_create0(const char *id); */

/* Returns a pointer to a new allocated {name}_s instance or NULL on error.
   All fields are uninitialised. */
{name}_s *{name}_create({args_dims});

/* Frees a {name}_s instance. */
void {name}_free({name}_s *self);

/* Get pointers for direct access to properties and dimensions.  Do not free. */
{name}_properties_s *{name}_props({name}_s *self);
{name}_properties_s * const {name}_const_props({name}_s *self);
{name}_dimensions_s * const {name}_dims({name}_s *self);

/* Getter functions */
{getter_declarations}


/* Setter functions */
{setter_declarations}

#endif /* _{NAME}_H */
"""


template_c = """\

#include "{name}.h"


typedef struct {{
  const char *id;         /* Instance UUID */
  const char *name;       /* Metadata name */
  const char *version;    /* Metadata version */
  const char *namespace;  /* Metadata namespace */
  {name}_dimensions_s dims;   /* Dimensions */
  {name}_properties_s props;  /* Properties */
}} {name}_s;


/* Returns a {d.nameUpperCamel} instance loaded from storage or NULL on error.

   The `driver`, `uri` and `option` arguments are passed to dopen().

   `id` is the UUID of the instance to load. */
{d.nameUpperCamel} *{d.nameLowerCase}_load(
    const char *driver, const char *uri, const char *options, const char *id);

/* Saves a {d.nameUpperCamel} instance to storage. */
int {d.nameLowerCase}_save({d.nameUpperCamel} *s,
    const char *driver, const char *uri, const char *options);

"""



if __name__ == '__main__':
    meta = HeaderMetadata.fromfile(
        '../../../calm/entities/Chemistry-0.1.json')
    print(meta.get_header())
