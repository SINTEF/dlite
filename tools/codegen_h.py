#/usr/bin/env python
"""DLite code generator for C header file

Usage: codegen_h entity.json
"""

import codegen


template = """\
/* ${entity_name}.h --
 * This is a generated file -- do not edit!
 */

typedef struct _${EntityName} {
${types}
} ${EntityName};




"""
