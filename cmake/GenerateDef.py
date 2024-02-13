#!/usr/bin/env python
"""gendef.py [options] index.xml header

Generates MSVS .def file for shared library from doxygen-generated XML.
A .def file contains the names of exported symbols.

Make sure that `GENERATE_XML = YES` in the doxygen configurations.
"""
from __future__ import print_function
import os
import argparse
from xml.dom import minidom


def gettext(element):
    """Returns the atcual content of a leaf element as a text string."""
    return '.'.join([node.data for node in element.childNodes
                    if node.nodeType == node.TEXT_NODE])


class DoxyXML:
    """Converts doxygen generated XML file to a .def file containing
    names of exported symbols for MSVS.

    Parameters
    ----------
    xmlfile : string
        Name of index.xml file generated by DoxyGen.
    """
    def __init__(self, xmlfile):
        self.xmlfile = xmlfile
        self.dom = minidom.parse(xmlfile)

    def get_from_file(self, filename, kind=None):
        """Returns a list with the names of members defined in `filename`.
        if `kind` is given, only the names of members of this kind are
        returned."""
        names = []
        for compound in self.dom.getElementsByTagName('compound'):
            if (compound.getAttribute('kind') == 'file' and
                gettext(compound.firstChild) == filename):
                for member in compound.getElementsByTagName('member'):
                    if kind is None or member.getAttribute('kind') == kind:
                        names.append(gettext(member.firstChild))
        return names

    def generate_deffile(self, library, sources, deffile=None):
        """Generates `deffile` for `library` containing the exported function
        and variable names in `sources`.  If `deffile` is None, it defaults
        to `library` with extension replaced with ".def"."""
        if deffile is None:
            deffile = os.path.splitext(library)[0] + '.def'
        libname = os.path

        exports = []
        for source in sources:
            header = os.path.splitext(os.path.basename(source))[0] + '.h'
            exports.extend(self.get_from_file(header, kind='function'))
            exports.extend(self.get_from_file(header, kind='variable'))
        defs = '\n'.join(['LIBRARY ' + os.path.basename(library), 'EXPORTS'] +
                         exports)
        with open(deffile, 'w') as f:
            f.write(defs + os.linesep)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('library',
                        help='Name of library to generate .def file for.')
    parser.add_argument('xmlfile',
                        help='Doxygen-generated index.xml file.')
    parser.add_argument('sources', nargs='+',
                        help='Source files, typically .c files.')
    parser.add_argument('--output', '-o',
                        help='Full path to .def file to write. Defaults to '
                        '`library` with extension replaced with ".def".')
    args = parser.parse_args()

    doxyxml = DoxyXML(args.xmlfile)
    doxyxml.generate_deffile(args.library, args.sources, args.output)


if __name__ == '__main__':
    main()
