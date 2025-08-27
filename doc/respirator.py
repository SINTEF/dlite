"""respirator is a support tool for 'breathe'. respirator will
generate .rst files with toctrees and doxygen generated doxumentation.

"""

import os
import re
import sys
from typing import List, Union
from pathlib import Path
from bs4 import BeautifulSoup as bs
from jinja2 import Environment, FileSystemLoader

class DoxygenXML:
    """Generate sphinx-compatible .rst files based on
    Doxygen generated XML files
    """

    TOC_TEMPLATE = "toc.rst.j2"
    DOXYFILE_TEMPLATE = "doxygenfile.rst.j2"

    def __init__(self,
                 doxygen_xml_path: Union[str,Path],
                 output_path: Union[str,Path],
                 template_path: Union[str, Path],
                 index_file = 'index.xml',
                 filename_filter=r".*\.h"):
        """
        initialize class
        """
        self.input_path = Path(doxygen_xml_path)
        self.output_path = Path(output_path)
        self.template_path = Path(template_path)
        self.index_file = index_file
        self.filename_filter = filename_filter
        self.filesmap = {}
        self.dirsmap = {}
        self._init_dirmap()

    def _read_content(self, filename):
        """Return parsed xml contents
        """
        q = self.input_path / filename
        with q.open("r") as file:
            buffer = file.readlines()
            content = bs("".join(buffer), features='xml')
            return content

    def _innerfiles(self, dir_id : str) -> List[str]:
        """
        return list of files belonging to a component
        """
        content = self._read_content(f"{dir_id}.xml")
        innerfiles = content.find_all("innerfile")
        for innerfile in innerfiles:
            if re.search(self.filename_filter, innerfile.text):
                yield innerfile.text

    def _innerdirs(self, dir_id : str) -> List[str]:
        """
        Yield innerdirs belonging to a component
        """
        content = self._read_content(f"{dir_id}.xml")
        innerfiles = content.find_all("innerdir")
        for innerfile in innerfiles:
            yield innerfile.text

    def _dirname(self, dir_id : str) -> Path:
        """return the name of the component given a dir_id
        """

        q = self.input_path / f"{dir_id}.xml"
        with q.open("r") as file:
            content = bs("".join(file.readlines()), features='xml')
            compoundname = content.find("compoundname")
            return compoundname.text

    def _init_dirmap(self):
        """update internal self.filesmap
        """
        content = self._read_content(self.index_file)
        dirmap = {}
        compound = content.find_all("compound", {"kind":"dir"})
        for element in compound:
            dir_id = element["refid"]
            dirname = self._dirname(dir_id)
            files = list(self._innerfiles(dir_id))
            dirs = list(self._innerdirs(dir_id))
            if files:
                self.filesmap[dirname] = files
            if dirs:
                self.dirsmap[dirname] = dirs

    def write_output(self):
        """
        Create output folders and write .rst index files and doxyfile references
        The default template can be overloaded by using a custom template that
        matches the output filename (with the .j2 extension) in the template folder
        """
        env = Environment(loader=FileSystemLoader(str(self.template_path)))
        env.globals.update(len=len)

        for module in self.filesmap:
            # Create TOC file
            toc_out = self.output_path / f"{module}.rst"
            foldername = self.output_path / module
            basename = os.path.basename(module)
            innerdirs = []

            # Create list of submodules (innerdirs)
            if module in self.dirsmap:
                innerdirs = [(os.path.basename(module) +
                              '/' + os.path.basename(innerdir))
                             for innerdir in self.dirsmap[module]]

            # Create a list of documented modules
            # (represented by a doxygen file)
            innerfiles = [basename + "/" + os.path.basename(
                innerfile.rsplit(".", 1)[0])
                    for innerfile in self.filesmap[module]]

            # Set jinja2-template, base or overloaded
            template = self.template_path / f"{module}.rst.j2"
            if template.exists():
                toc_template = env.get_template(f'{module}.rst.j2')
            else:
                toc_template = env.get_template(self.TOC_TEMPLATE)

            # Render template into a buffer
            buffer = toc_template.render(title=basename,
                                         refs=list(innerfiles) + innerdirs)

            # Create subdirectory on filesystem if needed
            os.makedirs(foldername, exist_ok=True)

            # Write out buffer to a file
            with toc_out.open("w") as output:
                os.makedirs(os.path.dirname(toc_out), exist_ok=True)
                output.write(buffer)

            # Create doxygen ref-files
            for filename in self.filesmap[module]:
                file = filename.rsplit(".", 1)[0]
                # Set jinja2-template, base or overloaded
                template = self.template_path / module / f"{file}.rst.j2"
                if template.exists():
                    template = f"{module}/{file}.rst.j2"
                    doxygenfile_template = env.get_template(template)
                else:
                    doxygenfile_template = env.get_template(
                        self.DOXYFILE_TEMPLATE)

                # Render template into buffer
                buffer = doxygenfile_template.render(
                    title=file, doxyfile=basename + "/" + filename)

                # Write out buffer to a file
                doxyfile_out = foldername / f"{file}.rst"
                with doxyfile_out.open("w") as output:
                    output.write(buffer)

# Main

if __name__ == '__main__':
    xml_path = sys.argv[1]
    output_path = sys.argv[2]
    jinja2_template_path = sys.argv[3]
    doxy = DoxygenXML(xml_path, output_path, jinja2_template_path)
    doxy.write_output()

