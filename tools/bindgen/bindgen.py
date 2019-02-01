# -*- coding: utf-8 -*-
"""
    Fortran Binding Generator
"""

import os
from pprint import pprint
import CppHeaderParser


FORTRAN_FCN_BIND_C = """
!{cdecl}
  {returns} &
  function {name}_c({func_arg}) &
    bind(C,name="{name}")
    import {cimport}
{decl_arg}
  end function {name}_c
"""

FORTRAN_FCN_CALL_C = """
{doc}
!{cdecl}
  {returns} &
  function {name}({func_arg})
{decl_arg}
    {returns} :: returns
{f2c}
    returns = {name}_c({func_argc})
    {name} = returns
  end function {name}
"""

FORTRAN_SUB_BIND_C = """
!{cdecl}
  subroutine {name}_c({func_arg}) &
    bind(C,name="{name}")
    import {cimport}
{decl_arg}
  end subroutine {name}_c
"""

FORTRAN_SUB_CALL_C = """
{doc}
!{cdecl}
  subroutine {name}({func_arg})
{decl_arg}
{f2c}
    call {name}_c({func_argc})
  end subroutine {name}
"""


def format_template(tpl):
    return tpl[1:len(tpl)]
         
         
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
            self.ctype = obj.get('raw_type', '').strip('::')
            self.is_ptr = obj.get('pointer', 0)
            self.is_const = obj.get('const', 0)
            self.is_array = obj.get('array', 0)
            self.ctypes_type = obj.get('ctypes_type', 0)


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
            

class Formatter(object):
    
    def func(self, fcn, fmt):
        """ Format a function with the specific format """
        return ''
    
    def arg(self, arg, fmt):
        """ Format an argument with the specific format """
        return ''

    def module(self, mod):
        """ Format a module/header file """
        return ''
    
    
class CFormat(Formatter):
    
    """ Formatter for the C language """
    
    decl_arg = '{ctype} {pointer}{name};'
    func_arg = '{ctype} {pointer}{name}'
    decl_func = '{returns} {pointer}{name}({func_arg});'
    impl_func = '{returns} {pointer}{name}({func_arg}) {\n};'
    
    def prop(self, obj):
        """ Return the c properties of an argument """
        if isinstance(obj, Argument):
            return {'pointer': '*' * obj.is_ptr,
                    'name': obj.name,
                    'ctype': obj.ctype}
            
        elif isinstance(obj, Function):
            return {'pointer': '*' * obj.returns.is_ptr,
                    'returns': obj.returns.ctype,
                    'name': obj.name,
                    'func_arg': self.func(obj, 'func_arg')}
                        
    def func(self, fcn, fmt):
        """ Format a function with the specific format """
        if fmt == 'func_arg':
            args = [self.func_arg.format(**self.prop(arg)) for arg in fcn.args]
            return ', '.join(args)
        elif fmt == 'decl_arg':
            args = [self.decl_arg.format(**self.prop(arg)) for arg in fcn.args]
            return '\n'.join(args)
        elif fmt == 'decl_func':
            return self.decl_func.format(**self.prop(fcn))
            
    def arg(self, arg, fmt):
        """ Format an argument with the specific format """
        prop = self.prop(arg)
        if hasattr(self, fmt):
            return getattr(self, fmt).format(**prop)
        else:
            return ''
    
    
class FortranFormat(Formatter):
    
    """ Formatter for the Fortran language """
    
    decl_arg = '{ftype}{comma}{attr} :: {name}'
    func_arg = '{name}'
    func_argc = '{name}_c'
    public = '  public :: {name}'
    fcn_bind_c = format_template(FORTRAN_FCN_BIND_C)
    fcn_call_c = format_template(FORTRAN_FCN_CALL_C)
    sub_bind_c = format_template(FORTRAN_SUB_BIND_C)
    sub_call_c = format_template(FORTRAN_SUB_CALL_C)
    f2c_str = '{name}_c = f_c_string_func({name})'
    
    def __init__(self):
        self.cfmt = CFormat()
    
    def c_type(self, arg):
        """ Return the iso_c_binding type for Fortran """        
        if arg.ctype == 'char':
            return 'c_char'        
        
        if arg.is_ptr > 0:
            return 'c_ptr'
        
        if arg.ctype == 'size_t':
            return 'c_size_t'
        
        elif arg.ctype.find('int') >= 0:
            items = arg.ctype.split(' ')
            if len(items) > 1:
                return 'c_{}'.format('_'.join(items[0:-1]))
            else:
                return 'c_' + arg.ctype
            
        elif arg.ctype.find('float128') >= 0:
            return 'c_float128'
        
        elif arg.ctype in ['float', 'double', 'long double']:
            return 'c_' + arg.ctype.replace(' ', '_')
        
        else:
            return 'c_ptr'
            
    def ftype(self, arg, length='', bind_c=False):
        """ Return the Fortran type of the given C type """   
        ctype = arg.ctype
        ftype = 'type'
        if ctype.find('int') >= 0:
            ftype = 'integer'
        elif ctype == 'size_t':
            ftype = 'integer'
        elif ctype.find('double') >= 0:
            ftype = 'real'
        elif ctype.find('float') >= 0:
            ftype = 'real'
        elif ctype.find('bool') >= 0:
            ftype = 'logical'
        elif ctype == 'char':
            ftype = 'character'

        attr = []
        if ctype == 'char':
            if length:
                attr.append('len={}'.format(length))
            if bind_c:
                attr.append('kind=c_char')
        elif bind_c or (ftype == 'type'):
            attr.append(self.c_type(arg))
        if attr:
            return '{ftype}({attr})'.format(ftype=ftype, attr=','.join(attr))
        else:
            return ftype
    
    def func(self, fcn, fmt):
        """ Format a function with the specific format """
        if fmt.find('func_arg') == 0:
            return ', '.join([self.arg(arg, fmt) for arg in fcn.args])

        elif fmt == 'doc':
            sep = '!{}'.format('-' * 75)
            doc = [sep]
            for line in fcn.doxygen.split('\n'):
                lin = line.replace('/**', '').replace('*/', '')
                doc.append('!>{}'.format(lin))
            doc.append(sep)
            return '\n'.join(doc)
        
        elif fmt == 'public':
            return self.public.format(name=fcn.name)
        
        elif fmt == 'bind_c':
            sub = fcn.returns.ctype == 'void'
            prop = {}
            prop['cdecl'] = self.cfmt.func(fcn, 'decl_func')
            prop['name'] = fcn.name
            if not sub:
                prop['returns'] = self.ftype(fcn.returns, bind_c=True)
            prop['func_arg'] = self.func(fcn, 'func_arg')
            ctyp = [self.c_type(arg) for arg in fcn.args]
            if not sub:
                ctyp.append(self.c_type(fcn.returns))
            prop['cimport'] = ', '.join(set(ctyp))
            decl_arg = []
            for arg in fcn.args:
                item = self.arg(arg, 'decl_arg',
                                length=1,
                                dimension='*',
                                intent='in',
                                bind_c=True)
                decl_arg.append('    ' + item)
            prop['decl_arg'] = '\n'.join(decl_arg)
            if sub:
                return self.sub_bind_c.format(**prop)
            else:
                return self.fcn_bind_c.format(**prop)
            
        elif fmt == 'call_c':
            prop = {}
            prop['name'] = fcn.name
            prop['doc'] = self.func(fcn, 'doc')
            prop['cdecl'] = self.cfmt.func(fcn, 'decl_func')
            prop['func_arg'] = self.func(fcn, 'func_arg')
            prop['func_argc'] = self.func(fcn, 'func_argc')
            decl_arg = []
            ind = '    '
            for arg in fcn.args:
                decl_arg.append(ind + self.arg(arg, 'decl_arg'))
            for arg in fcn.args:
                item = self.arg(arg, 'decl_argc', bind_c=True, length=1)
                if arg.ctype == 'char':
                    item += '(len_trim({})+1)'.format(arg.name)
                decl_arg.append(ind + item)
            prop['decl_arg'] = '\n'.join(decl_arg)
            sub = fcn.returns.ctype == 'void'
            if not sub:
                prop['returns'] = self.ftype(fcn.returns)
            f2c = []
            for arg in fcn.args:
                if (arg.ctype == 'char') & (arg.is_ptr == 1):
                    f2c.append(ind + self.f2c_str.format(name=arg.name))
            prop['f2c'] = '\n'.join(f2c)
            if sub:
                return self.sub_call_c.format(**prop)
            else:
                return self.fcn_call_c.format(**prop)
                
    def arg(self, arg, fmt='', **kwargs):
        """ Format an argument with the specific format """
        if fmt == 'func_arg':
            return self.func_arg.format(name=arg.name)
        
        elif fmt == 'func_argc':
            return self.func_argc.format(name=arg.name)        
        
        elif fmt.find('decl_arg') == 0:
            attr = []
            bind_c = kwargs.get('bind_c', False)
            if arg.ctype == 'char':
                for key in ['intent', 'dimension']:
                    val = kwargs.get(key, '')
                    if val:
                        attr.append('{}({})'.format(key, val))
            elif ((arg.is_ptr > 0) | (arg.ctype != 'char')) and bind_c:
                attr.append('value')
                
            if kwargs.get('parameter', False):
                attr.append('parameter')
                
            ftype = self.ftype(arg, kwargs.get('length', ''), bind_c)
            name = arg.name + '_c' if fmt == 'decl_argc' else arg.name
            return self.decl_arg.format(ftype=ftype,
                                        comma=',' if attr else '',
                                        attr=','.join(attr),
                                        name=name)
            
    def module(self, mod, template, filename):
        """ Generate Fortran module """
        with open(template) as fil:
            txt = fil.read()
            
        arg = {}
        for key in ['public', 'bind_c', 'call_c']:
            items = [self.func(fcn, key) for fcn in mod.functions]
            arg[key] = '\n'.join(items)
            
        with open(filename, 'w') as fil:
            fil.write(txt.format(**arg))
    
            
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
        
        
if __name__ == '__main__':
        
    dlite = Headers()
    dlite.parse('../../src/dlite-entity.h')
    dlite.parse('../../src/dlite-storage.h')
    
    fmt = FortranFormat()
    fmt.module(dlite, 'template-dlite.f90', 'dlite.f90')
    