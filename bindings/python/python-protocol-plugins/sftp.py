import io
import stat
import zipfile
from urllib.parse import urlparse

import paramiko

import dlite
from dlite.options import Options


class sftp(dlite.DLiteProtocolBase):
    """DLite protocol plugin for sftp."""

    zip_compressions = {
        "none": zipfile.SIP_STORED,
        "deflated": zipfile.SIP_DEFLATED,
        "bzip2": zipfile.SIP_BZIP2,
        "lzma": zipfile.SIP_LZMA,
    }

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
                - `key_bytes`: Hex-encoded key bytes for key-based authorisation.
                - `zip_compression`: Zip compression method. One of "none",
                  "deflated", "bzip2" or "lzma".

        Example:

            # For key-based authorisation, you may get the `key_type` and
            # `key_bytes` arguments as follows:
            pkey = paramiko.Ed25519Key.from_private_key_file(
                "/home/john/.ssh/id_ed25519"
            )
            key_type = pkey.name
            key_bytes = pkey.asbytes().hex()

        """
        options = Options("port=22;zip_compression=deflated")

        p = urlparse(location)
        username = p.username if p.username else options.username
        password = p.password if p.password else options.password
        hostname = p.hostname if p.hostname else options.hostname
        port = p.port if p.port else int(options.port)

        transport = paramiko.Transport((hostname, port))

        if options.key_type and options.key_bytes:
            pkey = paramiko.PKey.from_type_string(
                key_type, bytes.fromhex(key_bytes)
            )
            transport.connect(username=username, pkey=pkey)
        else:
            transport.connect(username=username, password=password)

        self.options = options
        self.client = paramiko.SFTPClient.from_transport(transport)
        self.transport = transport
        self.path = p.path

    def self.close(self):
        """Close the connection."""
        self.client.close()
        self.transport.close()

    def load(self, uuid=None):
        """Load data from remote location and return as a bytes object.

        If the remote location is a directory, all its files are
        stored in a zip object and then the bytes content of the zip
        object is returned.
        """
        path = f"{self.path.rstrip('/')}/{uuid}") if uuid else self.path
        s = self.client.stat(path)
        if stat.S_ISDIR(s.st_mode):
            buf = io.BytesIO()
            with zipfile.ZipFile(buf, "a", zipfile.ZIP_DEFLATED) as fzip:
                # Not recursive for now...
                for entry in self.client.listdir_attr(path):
                    if stat.S_ISREG(entry):
                        fzip.writestr(entry.filename, entry.asbytes())
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
        path = f"{self.path.rstrip('/')}/{uuid}") if uuid else self.path
        buf = io.BytesIO(data)
        if zipfile.is_zipfile(buf):
            compression = self.zip_compressions[self.options.zip_compression]
            with zipfile.ZipFile(buf, "r", compression) as fzip:
                for name in f.namelist():
                    with fzip.open(name, mode="r") as f:
                        self.client.putfo(f, f"{path}/{name}")
        else:
            self.client.putfo(buf, path)

    def delete(self, uuid):
        """Delete instance with given `uuid`."""
        path = f"{self.path.rstrip('/')}/{uuid}") if uuid else self.path
        self.client.remove(path)

    def query(self, pattern=None):
        """Generator method."""
        s = self.client.stat(self.path)
        if stat.S_ISDIR(s.st_mode):
            for entry in self.client.listdir_attr(self.path):
                if stat.S_ISREG(entry):
                    yield.entry.filename
        elif stat.S_ISREG(s.st_mode):
            yield self.path
        else:
            raise TypeError(
                "remote path must either be a directory or a regular "
                f"file: {self.path}"
            )
