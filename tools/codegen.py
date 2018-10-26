#/usr/bin/env python
"""DLite code generator

Usage: codegen [OPTIONS] entity.json
"""
from __future__ import print_function
from __future__ import division

import textwrap

from cgen import CMetadata



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
        '--header', '-H', nargs='?', const=True,
        help='Generate C header file.  Defaults to JSONFILE with (version '
        'and) extension replaced with ".h".')
    #parser.add_argument(
    #    '--source', '-c',
    #    help='Name of output source file. Defaults to JSONFILE with (version '
    #    'and) extension replaced with ".c".')
    args = parser.parse_args()

    basename = os.path.splitext(args.infile)[0].lower()
    m = re.match(r'(.*)-[0-9._]+$', basename)
    if m:
        basename = m.groups(0)[0]
    if args.destination:
        basename = os.path.join(args.destination, os.path.basename(basename))


    if args.header:
        headerfile = args.header if args.header else basename + '.h'
        meta = CMetadata.fromfile(args.infile)
        with open(headerfile, 'w') as f:
            f.write(meta.get_header())



if __name__ == '__main__':
    main()
    #meta = HeaderMetadata.fromfile(
    #    '../../../calm/entities/Chemistry-0.1.json')
    #print(meta.get_header())
    #print(meta.get_source())
