"""Protocol plugins.

The module also include a few help functions for calling
functions/class methods and using zipfile to transparently
representing a file directory.

"""
import inspect
import io
import os
import re
import traceback
import warnings
import zipfile
from glob import glob
from pathlib import Path

import dlite


class Protocol():
    """Provides an interface to protocol plugins.

    Arguments:
        protocol: Name of protocol.
        location: Location of resource.  Typically a URL or file path.
        options: Options passed to the protocol plugin.
    """

    def __init__(self, protocol, location, options=None):
        self.load_plugins()  # Load plugins before anything else

        d = {cls.__name__: cls for cls in dlite.DLiteProtocolBase.__subclasses__()}
        if protocol not in d:
            if protocol in self._failed_plugins:
                raise dlite.DLiteProtocolError(
                    f"protocol plugin failed to load: {protocol}"
                )
            else:
                msg = [f"no such protocol plugin: {protocol}"]
                if True or os.getenv("DLITE_PYDEBUG"):
                    msg.append("Checked search path:")
                    for path in dlite.python_protocol_plugin_path:
                        from glob import glob
                        msg.append(f"- {path}: {glob(path+'/*')}")
                raise dlite.DLiteProtocolError("\n".join(msg))

        self.conn = d[protocol]()
        self.protocol = protocol
        self.closed = False
        self._call("open", str(location), options=options)

    def close(self):
        """Close connection."""
        self._call("close")
        self.closed = True

    def load(self, uuid=None):
        """Load data from connection and return it as a bytes object."""
        return self._call("load", uuid=uuid)

    def save(self, data, uuid=None):
        """Save bytes object `data` to connection."""
        self._call("save", data, uuid=uuid)

    def delete(self, uuid):
        """Delete instance with given `uuid` from connection."""
        return self._call("delete", uuid=uuid)

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
        return self._call("query", pattern=pattern)

    # The stem of protocol plugins that failed to load
    _failed_plugins = set()

    @classmethod
    def load_plugins(cls):
        """Load all protocol plugins.

        The names of all plugin files that have been attempted to load
        are cached (regardless whether loading succeeded or
        not). Hence, it is safe to call this class method multiple
        times.  It won't reload already loaded plugins or try to
        reload plugins that already failed to load.

        """

        # This should not be needed when PR #953 has been merged
        if not hasattr(dlite, "_plugindict"):
            dlite._plugindict = {}

        for path in dlite.python_protocol_plugin_path:
            if Path(path).is_dir():
                path = f"{Path(path) / '*.py'}"
            for filename in glob(path):
                name = Path(filename).stem
                scopename = f"{name}_protocol"
                if (scopename not in dlite._plugindict
                    and name not in cls._failed_plugins
                ):
                    dlite._plugindict.setdefault(scopename, {})
                    scope = dlite._plugindict[scopename]
                    try:
                        dlite.run_file(filename, scope, scope)
                    except Exception as exc:
                        msg = (
                            f"\n{traceback.format_exc()}"
                            if os.getenv("DLITE_PYDEBUG")
                            else f": {exc}"
                        )
                        warnings.warn(
                            f"cannot load protocol plugin: {name}{msg}"
                        )
                        cls._failed_plugins.add(name)

    @classmethod
    def loaded_plugins(cls):
        """Return a set with the names of already loaded plugins."""
        return set(p.__name__ for p in dlite.DLiteProtocolBase.__subclasses__())

    @classmethod
    def failed_plugins(cls):
        """Return a set with the stem of the plugin files that have failed ot
        load.  This is typically the same as the plugin names.
        """
        return cls._failed_plugins.copy()

    def _call(self, method, *args, **kwargs):
        """Call given method usin `call()` if it exists."""
        if self.closed:
            raise dlite.DLiteIOError(
                f"calling closed connection to '{self.protocol}' protocol "
                "plugin"
            )
        if hasattr(self.conn, method):
            return call(getattr(self.conn, method), *args, **kwargs)

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close()

    def __del__(self):
        try:
            if not self.closed:
                self.close()
        except Exception:  # Ignore exceptions at shutdown
            pass

# Help functions

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


# Functions for zip archive

zip_compressions = {
    "none": zipfile.ZIP_STORED,
    "deflated": zipfile.ZIP_DEFLATED,
    "bzip2": zipfile.ZIP_BZIP2,
    "lzma": zipfile.ZIP_LZMA,
}


def load_path(
    path,
    include=None,
    exclude=None,
    compression="lzma",
    compresslevel=None,
):
    """Returns content of path as bytes.

    If `path` is a directory, it will be zipped first.

    Arguments:
        path: File-like object or name to a regular file, directory or
            socket to load from .
        include: Regular expression matching file names to be included
            when path is a directory.
        exclude: Regular expression matching file names to be excluded
            when path is a directory.  May exclude files included with
            `include`.
        compression: Compression to use if path is a directory.
            Should be "none", "deflated", "bzip2" or "lzma".
        compresslevel: Integer compression level.
            For compresison "none" or "lzma"; no effect.
            For compresison "deflated"; 0 to 9 are valid.
            For compresison "bzip2"; 1 to 9 are valid.

    Returns:
        Bytes object with content of `path`.
    """
    if isinstance(path, io.IOBase):
        return path.read()

    p = Path(path)
    if p.is_file():
        return p.read_bytes()

    if p.is_dir():
        incl = re.compile(include) if include else None
        excl = re.compile(exclude) if exclude else None
        buf = io.BytesIO()
        with zipfile.ZipFile(
                file=buf,
                mode="w",
                compression=zip_compressions[compression],
                compresslevel=compresslevel,
        ) as fzip:

            def iterdir(dirpath):
                """Add all files matching regular expressions to zipfile."""
                for p in dirpath.iterdir():
                    name = str(p.relative_to(path))
                    if (p.is_file()
                        and (not incl or incl.match(name))
                        and (not excl or not excl.match(name))
                    ):
                        fzip.writestr(name, p.read_bytes())
                    elif p.is_dir():
                        iterdir(p)

            iterdir(p)
        return buf.getvalue()

    if p.is_socket():
        buf = []
        while True:
            data = path.recv(1024)
            if data:
                buf.apend(data)
            else:
                return b"".join(buf)


def save_path(data, path, overwrite=False, include=None, exclude=None):
    """Save `data` to file path.

    Arguments:
        data: Bytes object to save.
        path: File-like object or name to a regular file, directory or
            socket to save to.
        overwrite: Whether to overwrite an existing path.
        include: Regular expression matching file names to be included
            when path is a directory.
        exclude: Regular expression matching file names to be excluded
            when path is a directory.  May exclude files included with
            `include`.
    """
    if isinstance(path, io.IOBase):
        path.write(data)
    else:
        p = Path(path)

        buf = io.BytesIO(data)
        iszip = zipfile.is_zipfile(buf)
        buf.seek(0)

        if iszip:
            if p.exists() and not p.is_dir():
                raise ValueError(
                    f"cannot write zipped data to non-directory path: {path}"
                )
            incl = re.compile(include) if include else None
            excl = re.compile(exclude) if exclude else None
            with zipfile.ZipFile(file=buf, mode="r") as fzip:
                for name in fzip.namelist():
                    if ((incl and not incl.match(name))
                        or (excl and  excl.match(name))):
                        continue
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
                    fzip.extract(name, path=path)
        elif p.exists():
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
            p.parent.mkdir(parents=True, exist_ok=True)
            p.write_bytes(data)


def is_archive(data):
    """Returns true if binary data `data` is an archieve, otherwise false."""
    return zipfile.is_zipfile(io.BytesIO(data))


def archive_check(data):
    """Raises an exception if `data` is not an archive.  Otherwise return
    a io.BytesIO object for data."""
    buf = io.BytesIO(data)
    if not zipfile.is_zipfile(buf):
        raise TypeError("data is not an archieve")
    return buf

def archive_names(data):
    """If `data` is an archive, return a list of archive file names."""
    with zipfile.ZipFile(file=archive_check(data), mode="r") as fzip:
        return fzip.namelist()


def archive_extract(data, name):
    """If `data` is an archieve, return object with given name."""
    with zipfile.ZipFile(file=archive_check(data), mode="r") as fzip:
        with fzip.open(name, mode="r") as f:
            return f.read()


def archive_add(data, name, content):
    """If `data` is an archieve, return a new bytes object with
    `content` added to `data` using the given name."""
    with zipfile.ZipFile(file=archive_check(data), mode="a") as fzip:
        with fzip.open(name, mode="w") as f:
            f.write(content.encode() if hasattr(content, "encode") else content)
    return buf.getvalue()
