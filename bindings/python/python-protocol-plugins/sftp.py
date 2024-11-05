"""DLite protocol plugin for sftp.

Note: Continous testing is not run for this plugin.
"""

import io
import stat
import zipfile
from pathlib import Path
from urllib.parse import urlparse

import paramiko

import dlite
from dlite.options import Options
from dlite.protocol import zip_compressions


class sftp(dlite.DLiteProtocolBase):
    """DLite protocol plugin for sftp."""

    def open(self, location, options=None):
        """Opens `location`.

        Arguments:
            location: SFTP host name.  May be just the host name or fully
                qualified as `username:password@host:port`.  In the latter
                case the port/username/password takes precedence over `options`.
            options: Supported options:

                - `username`: User name.
                - `password`: Password.
                - `hostname`: Host name.
                - `port`: Port number. Default is 22.
                - `key_type`: Key type for key-based authorisation, ex:
                  "ssh-ed25519"
                - `key_bytes`: Hex-encoded key bytes for key-based
                  authorisation.
                - `include`: Regular expression matching file names to be
                  included if `location` is a directory.
                - `exclude`: Regular expression matching file names to be
                  excluded if `location` is a directory.
                - `compression`: Zip compression method. One of "none",
                  "deflated" (default), "bzip2" or "lzma".
                - `compresslevel`: Integer compression level.
                  For compresison "none" or "lzma"; no effect.
                  For compresison "deflated"; 0 to 9 are valid.
                  For compresison "bzip2"; 1 to 9 are valid.

        Example:
            Here is an example of how you can use a private ssh-key

                >>> # For key-based authorisation, you may get the `key_type`
                >>> # and `key_bytes` arguments as follows:
                >>> pkey = paramiko.Ed25519Key.from_private_key_file(
                ...     "/home/john/.ssh/id_ed25519"
                ... )
                >>> key_type = pkey.name
                >>> key_bytes = pkey.asbytes().hex()

        """
        options = Options(options, "port=22;compression=lzma")

        p = urlparse(location)
        if not p.scheme:
            p = urlparse("sftp://" + location)
        username = p.username if p.username else options.pop("username", None)
        password = p.password if p.password else options.pop("password", None)
        hostname = p.hostname if p.hostname else options.get("hostname")
        port = p.port if p.port else int(options.pop("port"))

        transport = paramiko.Transport((hostname, port))

        if "key_type" in options and "key_bytes" in options:
            pkey = paramiko.PKey.from_type_string(
                options.key_type, bytes.fromhex(options.key_bytes)
            )
            transport.connect(username=username, pkey=pkey)
        elif username and password:
            transport.connect(username=username, password=password)
        else:
            transport.connect()

        self.options = options
        self.client = paramiko.SFTPClient.from_transport(transport)
        self.transport = transport
        self.path = p.path

    def close(self):
        """Close the connection."""
        self.client.close()
        self.transport.close()

    def load(self, uuid=None):
        """Load data from remote location and return as a bytes object.

        If the remote location is a directory, all its files are
        stored in a zip object and then the bytes content of the zip
        object is returned.
        """
        path = f"{self.path.rstrip('/')}/{uuid}" if uuid else self.path
        opts = self.options
        s = self.client.stat(path)
        if stat.S_ISDIR(s.st_mode):
            incl = re.compile(opts.include) if "include" in opts else None
            excl = re.compile(opts.exclude) if "exclude" in opts else None
            buf = io.BytesIO()
            with zipfile.ZipFile(
                    file=buf,
                    mode="w",
                    compression=zip_compressions[opts.compression],
                    compresslevel=opts.get("compresslevel"),
            ) as fzip:

                def iterdir(dirpath):
                    """Add all files matching regular expressions."""
                    for entry in self.client.listdir_attr(dirpath):
                        fullpath = Path(dirpath) / entry.filename
                        name = str(fullpath.relative_to(path))
                        if (stat.S_ISREG(entry.st_mode)
                            and (not incl or incl.match(name))
                            and (not excl or not excl.match(name))
                        ):
                            fzip.writestr(name, entry.asbytes())
                        elif stat.S_ISDIR(entry.st_mode):
                            iterdir(str(fullpath))

                iterdir(path)
            data = buf.getvalue()
        elif stat.S_ISREG(s.st_mode):
            with self.client.open(path, mode="r") as f:
                data = f.read()
        else:
            raise TypeError(
                "remote path must either be a directory or a regular "
                f"file: {path}"
            )
        return data

    def save(self, data, uuid=None):
        """Save bytes object `data` to remote location."""
        path = f"{self.path.rstrip('/')}/{uuid}" if uuid else self.path
        opts = self.options

        buf = io.BytesIO(data)
        iszip = zipfile.is_zipfile(buf)
        buf.seek(0)

        if iszip:
            incl = re.compile(opts.include) if "include" in opts else None
            excl = re.compile(opts.exclude) if "exclude" in opts else None
            with zipfile.ZipFile(file=buf, mode="r") as fzip:
                for name in fzip.namelist():
                    if ((incl and not incl.match(name))
                        or (excl and excl.match(name))):
                        continue
                    with fzip.open(name, mode="r") as f:
                        pathname = f"{path}/{name}"
                        self._create_missing_dirs(pathname)
                        self.client.putfo(f, pathname)
        else:
            self._create_missing_dirs(path)
            self.client.putfo(buf, path)

    def _create_missing_dirs(self, path):
        """Create missing directories in `path` on remote host."""
        p = Path(path).parent
        missing_dirs = []
        while True:
            try:
                self.client.stat(str(p))
            except FileNotFoundError:
                missing_dirs.append(p.parts[-1])
                p = p.parent
            else:
                break

        for dir in reversed(missing_dirs):
            p = p / dir
            self.client.mkdir(str(p))

    def delete(self, uuid=None):
        """Delete instance with given `uuid`."""
        path = f"{self.path.rstrip('/')}/{uuid}" if uuid else self.path
        self.client.remove(path)

    def query(self, pattern=None):
        """Generator over all filenames in the directory referred to by
        `location`.  If `location` is a file, return its name.

        Arguments:
            pattern: Glob pattern for matching metadata URIs.  Unused.
        """
        s = self.client.stat(self.path)
        if stat.S_ISDIR(s.st_mode):
            for entry in self.client.listdir_attr(self.path):
                if stat.S_ISREG(entry):
                    yield entry.filename
        elif stat.S_ISREG(s.st_mode):
            yield self.path
        else:
            raise TypeError(
                "remote path must either be a directory or a regular "
                f"file: {self.path}"
            )
