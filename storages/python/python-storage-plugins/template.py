"""DLite storage plugin for creating a file based on a template.
"""
import dlite
from dlite.options import Options


class template(dlite.DLiteStorageBase):
    """DLite storage plugin for file-generation using a template."""

    def open(self, location, options=None):
        """Opens `location`.

        Arguments:
            location: Path to file to generate.
            options: Supported options:
            - `template`: Path to template file.
            - `engine`: Template engine to use.  One of:
                 - "format": Use format() function from the standard library.
                   See https://docs.python.org/3/library/string.html#formatspec
                   for specifications.  This is the default.
                 - "jinja": Use jinja.  See https://jinja.palletsprojects.com/

        """
        self.options = Options(options, defaults="engine=format")
        if "template" not in options:
            raise ValueError(f"missing 'template' option")
        self.location = location

    def save(self, inst):
        """Store instance to a file that moltemaplate can read."""
        with open(self.options.template, "r", encoding="utf8") as f:
            template = f.read()

        if self.options.engine == "format":
            data = template.format(**inst.properties)
        elif self.options.engine == "jinja":
            from jinja2 import Template  # delay import until we need it

            j2_template = Template(template)
            data = j2_template.render(inst.properties)
        else:
            raise ValueError(
                "The 'engine' option must be either \"format\" " 'or "jinja"'
            )

        with open(self.location, "w", encoding="utf8") as f:
            f.write(data)
