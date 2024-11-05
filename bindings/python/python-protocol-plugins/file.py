"""DLite protocol plugin for files."""
import re
from pathlib import Path
from urllib.parse import urlparse

import dlite
from dlite.options import Options
from dlite.protocol import archive_names, is_archive, load_path, save_path


class file(dlite.DLiteProtocolBase):
    """DLite protocol plugin for files."""

    def open(self, location, options=None):
        """Opens `location`.

        Arguments:
            location: A URL or path to a file or directory.
            options: Supported options:

                - `mode`: Combination of "r" (read), "w" (write) or "a" (append)
                  Defaults to "r" if `location` exists and "w" otherwise.
                  This default avoids accidentially overwriting an existing
                  file.  Use "rw" for reading and writing.
                - `url`: Whether `location` is an URL.  The default is
                  read it as an URL if it starts with a scheme, otherwise
                  as a file path.
                - `include`: Regular expression matching file names to be
                  included if `location` is a directory.
                - `exclude`: Regular expression matching file names to be
                  excluded if `location` is a directory.
                - `compression`: Compression to use if path is a directory.
                  Should be "none", "deflated", "bzip2" or "lzma".
                - `compresslevel`: Integer compression level.
                  For compresison "none" or "lzma"; no effect.
                  For compresison "deflated"; 0 to 9 are valid.
                  For compresison "bzip2"; 1 to 9 are valid.
        """
        opts = Options(options, "compression=lzma")
        isurl = dlite.asbool(opts.url) if "url" in opts else bool(
            # The last slash in the regex is to avoid matching a Windows
            # path C:\...
            re.match(r"^[a-zA-Z][a-zA-Z0-9.+-]*:[^\\]", str(location))
        )
        self.path = Path(urlparse(location).path if isurl else location)
        self.mode = (
            opts.mode if "mode" in opts
            else "r" if self.path.exists()
            else "w"
        )
        self.options = opts

    def load(self, uuid=None):
        """Return data loaded from file.

        If `location` is a directory, it is returned as a zip archive.
        """
        self._required_mode("r", "load")
        path = self.path/uuid if uuid else self.path
        return load_path(
            path=path,
            include=self.options.get("include"),
            exclude=self.options.get("exclude"),
            compression=self.options.get("compression"),
            compresslevel=self.options.get("compresslevel"),
        )

    def save(self, data, uuid=None):
        """Save `data` to file."""
        self._required_mode("wa", "save")
        path = self.path/uuid if uuid else self.path
        save_path(
            data=data,
            path=path,
            overwrite=True if "w" in self.mode else False,
            include=self.options.get("include"),
            exclude=self.options.get("exclude"),
        )

    def delete(self, uuid):
        """Delete instance with given `uuid`."""
        self._required_mode("w", "delete")
        path = self.path/uuid
        if path.exists():
            path.unlink()
        elif self.path.exists() and self.path.is_dir():
            save_path(
                data=load_path(self.path, exclude=uuid),
                path=path,
                overwrite=True,
            )
        else:
            raise dlite.DLiteIOError(f"cannot delete {uuid} in: {self.path}")

    def query(self):
        """Iterator over all filenames in the directory referred to by
        `location`.  If `location` is a file, return its name."""
        data = self.load()
        return archive_names(data) if is_archive(data) else self.path.name

    def _required_mode(self, required, operation):
        """Raises DLiteIOError if mode does not contain any of the mode
        letters in `required`."""
        if not any(c in self.mode for c in required):
            raise dlite.DLiteIOError(
                f"mode='{self.mode}', cannot {operation}: {self.path}"
            )
