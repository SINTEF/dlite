"""Protocol plugins.

The module also include a few help functions for calling
functions/class methods and using zipfile to transparently
representing a file directory.

"""
import inspect
import io
import re
import zipfile
from glob import glob
from pathlib import Path

import dlite


class Protocol():
    """Provides an interface to protocol plugins.

    Arguments:
        scheme: Name of protocol.
        location: URL or file path to storage.
        options: Options passed to the protocol plugin.
    """

    def __init__(self, scheme, location, options=None):
        d = {cls.__name__: cls for cls in dlite.DLiteProtocolBase.__subclasses__()}
        if scheme not in d:
            raise DLiteLookupError(f"no such protocol plugin: {scheme}")

        self.conn = d[scheme]()
        call(self.conn.open, location, options=options)

    def close(self):
        """Close connection."""
        self.conn.close()

    def load(self, uuid=None):
        """Load data from connection and return it as a bytes object."""
        return call(self.conn.load, uuid=uuid)

    def save(self, data, uuid=None):
        """Save bytes object `data` to connection."""
        call(self.conn.save, data, uuid=uuid)

    def delete(self, uuid):
        """Delete instance with given `uuid` from connection."""
        return call(self.conn.delete, uuid=uuid)

    def query(self, pattern=None):
        """Generator method that iterates over all UUIDs in the connection
        who"s metadata URI matches glob pattern `pattern`.

        Arguments:
            pattern: Glob pattern for matching metadata URIs.

        Yields:
            UUIDs of instances who's metadata URI matches glob pattern `pattern`.
            If no `pattern` is given, the UUIDs of all instances in the
            connection are yielded.
        """
        return call(self.conn.query, pattern=pattern)

    @staticmethod
    def load_plugins():
        """Load all protocol plugins."""

        # This should not be needed when PR #953 has been merged
        if not hasattr(dlite, "_plugindict"):
            dlite._plugindict = {}

        for path in dlite.python_protocol_plugin_path:
            if Path(path).is_dir():
                path = f"{Path(path) / '*.py'}"
            for filename in glob(path):
                scopename = f"{Path(filename).stem}_protocol"
                dlite._plugindict.setdefault(scopename, {})
                scope = dlite._plugindict[scopename]
                dlite.run_file(filename, scope, scope)


def call(func, *args, **kwargs):
    """Call a function.  Keyword arguments that are None, will not be passed
    to `func` if `func` does not have this argument.
    """
    param = inspect.signature(func).parameters

    kw = {}
    for name, value in kwargs.items():
        if name in param:
            kw[name] = value
        elif value is not None:
            raise KeyError(f"{func} has not keyword argument: {name}")

    return func(*args, **kw)


zip_compressions = {
    "none": zipfile.ZIP_STORED,
    "deflated": zipfile.ZIP_DEFLATED,
    "bzip2": zipfile.ZIP_BZIP2,
    "lzma": zipfile.ZIP_LZMA,
}


def load_path(path, regex=None, zip_compression="deflated", compresslevel=None):
    """Returns content of path as bytes.

    If `path` is a directory, it will be zipped first.

    Arguments:
        path: File-like object or name to a regular file, directory or
            socket to load from .
        regex: If given, a regular expression matching file names to be
            included when path is a directory.
        zip_compression: Zip compression level to use if path is a directory.
            Should be: "none", "deflated", "bzip2" or "lzma".

    Returns:
        Bytes object with content of `path`.
    """
    if isinstance(path, io.IOBase):
        return path.read()

    p = Path(path)
    if p.is_file():
        return p.read_bytes()

    if p.is_dir():
        if regex:
            expr = re.compile(regex)
        buf = io.BytesIO()
        with zipfile.ZipFile(
                file=buf,
                mode="a",
                compression=zip_compressions[zip_compression],
                compresslevel=compresslevel,
        ) as fzip:

            def iterdir(dirpath):
                """Add all files matching `regex` to zipfile."""
                for p in dirpath.iterdir():
                    if p.is_file() and (not regex or expr.match(p.name)):
                        print("  - read:", p.relative_to(path))
                        fzip.writestr(str(p.relative_to(path)), p.read_bytes())
                    elif p.is_dir():
                        iterdir(p)

            iterdir(p)
            #return buf.getvalue()
            data = buf.getvalue()
            print("*** load dir to zip:", len(data))
            return data

    if p.is_socket():
        buf = []
        while True:
            data = path.recv(1024)
            if data:
                buf.apend(data)
            else:
                return b"".join(buf)


def save_path(data, path, overwrite=False):
    """Save `data` to file path.

    Arguments:
        data: Bytes object to save.
        path: File-like object or name to a regular file, directory or
            socket to save to.
        overwrite: Whether to overwrite an existing path.
    """
    if isinstance(path, io.IOBase):
        path.write(data)
    else:
        #magic_numbers = b"PK\x03\x04", b"PK\x05\x06", b"PK\x07\x08"
        p = Path(path)

        #if data.startswith(magic_numbers):
        buf = io.BytesIO(data)
        if zipfile.is_zipfile(buf):
            print("*** save zip to dir")
            if p.exists() and not p.is_dir():
                raise ValueError(
                    f"cannot write zipped data to non-directory path: {path}"
                )
            with zipfile.ZipFile(file=buf, mode="r") as fzip:
                for name in fzip.namelist():
                    filepath = p / name
                    filepath.parent.mkdir(parents=True, exist_ok=True)
                    if filepath.exists():
                        if not overwrite:
                            continue
                        if not filepath.is_file():
                            raise OSError(
                                "cannot write to existing non-file path: "
                                f"{filename}"
                            )
                    fzip.extact(name, path=path)
        elif p.exists():
            print("*** exists:", p)
            if p.is_socket():
                path.sendall(data)
            elif p.is_file():
                if overwrite:
                    p.write_bytes(data)
            else:
                raise ValueError(
                    f"cannot write data to existing non-file path: {path}"
                )
        else:
            print("*** write:", p)
            p.parent.mkdir(parents=True, exist_ok=True)
            p.write_bytes(data)
