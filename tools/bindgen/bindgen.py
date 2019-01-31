# -*- coding: utf-8 -*-
"""
    Fortran Binding Generator
"""

import os
from pprint import pprint
import CppHeaderParser


FORTRAN_BIND_C = """
  {returns} &
  function {name}_c({arg_list}) &
     bind(C,name="{name}")
     import c_ptr, c_char, c_int
{args}
  end function {name}_c
"""

FORTRAN_CALL_C = """
{doc}
  {returns} &
  function {name}({arg_list})
    {args}
    {returns} :: ans

    ans = {name}_c({argc_list})
    {name} = ans

  end function {name}
"""

TYPES = {'fortran': {'int': 'integer',
                     'size_t': 'integer',
                     'char': 'character'},

         'fortran_bind_c': {'int': 'c_int',
                            'size_t': 'c_int',
                            'char': 'c_char'},
                            
         'fortran_call_c': {}
         }


class Argument(object):
    
    """ Representing a function argument """

    def __init__(self, obj):
        self.name = ''
        self.ctype = ''
        self.is_ptr = 0
        self.is_const = 0
        self.is_array = 0
        self.ctypes_type = ''
        self.parse(obj)
        
    def parse(self, obj):
        if isinstance(obj, dict):
            self.name = obj.get('name', '')
            self.ctype = obj.get('raw_type', '')
            self.is_ptr = obj.get('pointer', 0)
            self.is_const = obj.get('const', 0)
            self.is_array = obj.get('array', 0)
            self.ctypes_type = obj.get('ctypes_type', 0)
            
    def get_type(self, lang):
        if lang == 'fortran_return':
            if (self.is_ptr > 0):
                return 'type(c_ptr)'
            else:
                t1 = self.get_type('fortran')
                t2 = self.get_type('fortran_bind_c')
                return '{}({})'.format(t1, t2)
            
        return TYPES[lang].get(self.ctype, self.ctype)
    
    def declare(self, lang):
        """ Return tuple (declare lines, copy lines) """
        lines = []
        copy = []
        if lang == 'fortran_bind_c':
            if self.is_ptr > 0:
                if self.ctype == 'char':
                    res = 'character(len=1,kind=c_char), dimension(*), '
                    res += 'intent(in) :: ' + self.name
                    lines.append(res)
                elif self.ctype not in TYPES[lang]:
                    lines.append('type(c_ptr), value :: ' + self.name)
            if len(lines) == 0:
                ans = self.declare('c')
                lines.append('! ' + ans[0][0])
        
        elif lang == 'fortran_call_c': 
            if self.is_ptr > 0:
                if self.ctype == 'char':
                    l1 = 'character(len=*), intent(in) :: {0}'
                    l2 = 'character(len=1,kind=c_char) :: {0}_c(len_trim({0})+1)'
                    lines.append(l1.format(self.name))
                    lines.append(l2.format(self.name))
                    c1 = '{0}_c = f_c_string_func({0})'
                    copy.append(c1.format(self.name))
                elif self.ctype not in TYPES[lang]:
                    lines.append('type(c_ptr) :: ' + self.name)
            if len(lines) == 0: 
                ans = self.declare('c')
                lines.append('! ' + ans[0][0])

        elif lang == 'c':
            lines.append('{} {}{};'.format(self.ctype,
                                           '*' if self.is_ptr else '',
                                           self.name))

        return (lines, copy)


class Function(object):
    """
    Representing a function, with a name, a return type and some arguments
    """

    def __init__(self, obj):
        self.name = ''        
        self.doxygen = ''
        self.is_const = 0
        self.returns = Argument({'name': 'return'})
        self.args = []
        self.parse(obj)
        
    def parse(self, obj):
        if isinstance(obj, dict):
            self.name = obj.get('name', '')
            self.is_const = obj.get('const', 0)
            self.doxygen = obj.get('doxygen', '')
            self.returns.ctype = obj.get('returns', '')
            self.returns.is_ptr = obj.get('returns_pointer', 0)
            self.args = []
            for par in obj.get('parameters', []):
                arg = Argument(par)
                if arg.name:
                    self.args.append(arg)
            
    def fortran(self, key):
        if key == 'doc':
            sep = '!{}\n'.format('-' * 75)
            doc = sep
            for line in self.doxygen.split('\n'):
                doc += '!>{}\n'.format(line)
            doc += sep
            return doc
        
        elif key == 'public':
            return '  public :: ' + self.name
        
        elif key == 'bind_c':
            lang = 'fortran_bind_c'
            args = [arg.name for arg in self.args]
            ctypes = [arg.get_type(lang) for arg in self.args]
            dc = []
            cp = []
            for item in [arg.declare(lang) for arg in self.args]:
                dc += ['     {}'.format(x) for x in item[0]]
                cp += ['     {}'.format(x) for x in item[1]]
            return FORTRAN_BIND_C.format(
                    returns=self.returns.get_type('fortran_return'),
                    name=self.name,
                    arg_list=', '.join(args),
                    ctype_list=', '.join(ctypes),
                    args='{}\n{}'.format('\n'.join(dc), '\n'.join(cp))
                   )
            
        elif key == 'call_c':
            lang = 'fortran_call_c'
            arg_list = [arg.name for arg in self.args]
            dc = []
            cp = []
            for item in [arg.declare(lang) for arg in self.args]:
                dc += ['     {}'.format(x) for x in item[0]]
                cp += ['     {}'.format(x) for x in item[1]]            
            return FORTRAN_CALL_C.format(
                    doc=self.fortran('doc'),
                    returns=self.returns.get_type('fortran_return'),
                    name=self.name,
                    arg_list=', '.join(arg_list),
                    argc_list=', '.join([arg + '_c' for arg in arg_list]),
                    args='{}\n{}'.format('\n'.join(dc), '\n'.join(cp))
                   )            
            
            
class Headers(object):
    
    def __init__(self):
        self.types = []
        self.functions = []
        
    def parse(self, filename):
        header = CppHeaderParser.CppHeader(filename)
        for fcn in header.functions:
            ff = Function(fcn)
            if ff.name:
                self.functions.append(ff)        
        
    def generate_fortran(self):
        """ Generate dlite Fortran module """
        with open('template-dlite.f90') as fil:
            txt = fil.read()
            
        arg = {}
        for key in ['public', 'bind_c', 'call_c']:
            arg[key] = '\n'.join([fcn.fortran(key) for fcn in self.functions])
            
        with open('dlite.f90', 'w') as fil:
            fil.write(txt.format(**arg))
        
        
if __name__ == '__main__':
        
    dlite = Headers()
    dlite.parse('../../src/dlite-entity.h')
    dlite.parse('../../src/dlite-storage.h')
    dlite.generate_fortran()
    