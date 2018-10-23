#/usr/bin/env python
"""DLite code generator for C

Usage: codegen_c entity.json
"""
from __future__ import print_function
from __future__ import division

import textwrap

import basegen
from basegen import toIdentifier as ident


# Maps metadata types to C types
typeconv_C = {
    'string': 'char *',
}



class CMetadata(basegen.Metadata):
    """Metadata subclass for C."""

    def __init__(self, jsondoc):
        super(CMetadata, self).__init__(jsondoc)

        self.struct_props = self.get_struct_props()
        self.struct_dims = self.get_struct_dims()
        self.description = '\n * '.join(textwrap.wrap(self.description))

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

    def get_header(self):
        """Returns content of C header file."""
        return template_h.format(**self.__dict__)

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




template_h = """\
/* This is a generated with DLite codegen -- do not edit! */

/* {description} */
#ifndef _{NAME}_H
#define _{NAME}_H

#include "integers.h"
#include "boolean.h"


typedef struct _{Name} {{
  /* -- header */
  char uuid[36+1];   /*!< UUID for this data instance. */
  const char *uri;   /*!< Unique name or uri of the data instance.
                          Can be NULL. */

  size_t refcount;   /*!< Number of references to this instance. */
  const void *meta;  /*!< Pointer to the metadata describing this instance. */

  /* -- dimension values */
  {struct_dims}

  /* -- property values */
  {struct_props}
}} {Name};

#endif /* _{NAME}_H */
"""
