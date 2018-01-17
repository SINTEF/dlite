#/usr/bin/env python
"""DLite code generator for C

Usage: codegen_c entity.json
"""
import textwrap

import codegen

ident = codegen.toIdentifier


# Mappings from property types to C
type_to_c = {
    'string': 'char *',
}

# Mappings from C to Fortran
c_to_fortran = {
    'char *': 'character(c_char)'
    'char **': 'type(c_ptr), dimension(:), pointer'
    'double *': 'real(c_double), dimension(:), pointer'
    'int *': 'real(c_int), dimension(:), pointer'
    }


class HeaderMetadata(codegen.Metadata):
    """Metadata subclass for C."""

    def __init__(self, jsondoc):
        super(HeaderMetadata, self).__init__(jsondoc)

        self.type_dims = '\n'.join('    integer(c_size_t) :: %s  !%s' % (
            dimname, self.dims[dimname].get('description', ''))
                                    for dimname in self.dimnames)

        self.type_cprops = '\n'.join('    type(c_ptr) :: %s  !%s' % (
            propname, self.props[propname].get('description', ''))
                                     for propname in self.propnames)

        self.type_fprops = ''

    def get_source(self):
        """Returns content of F90 source file."""
        return template_f90.format(**self.__dict__)



template_f90 = """\
!! This is a generated with DLite codegen_f90 -- do not edit!
!
MODULE {name}

  USE iso_c_binding, ONLY : c_ptr, c_size_t, c_f_pointer, c_double, c_int

  IMPLICIT NONE

  PRIVATE

  TYPE, BIND(c) :: {name}_dimensions_s
{type_dims}
  END TYPE {name}_dimensions_s

  TYPE, BIND(c) :: {name}_properties_s
{type_cprops}
  END TYPE {name}_properties_s

  TYPE, BIND(c) :: {name}_s
    type(c_ptr) :: id
    type({name}_dimensions_s :: dims
    type({name}_properties_s :: props
  END TYPE {name}_s

  TYPE T{NameCamel}Dims
{type_dims}
  END TYPE T{NameCamel}Dims

  TYPE T{NameCamel}
    type(T{NameCamel}Dims :: dimensions
{type_fprops}
  END TYPE T{NameCamel}



"""




if __name__ == '__main__':
    meta = HeaderMetadata.fromfile(
        '../../../calm/entities/Chemistry-0.1.json')
    print(meta.get_source())
