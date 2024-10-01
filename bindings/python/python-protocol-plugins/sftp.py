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
        if uuid:
            path = f"{self.path.rstrip('/')/{uuid}")
        s = self.client.stat(path)
        if stat.S_ISDIR(s.st_mode):
            buf = io.BytesIO()
            with zipfile.ZipFile(buf, "a", zipfile.ZIP_DEFLATED) as f:
                # Not recursive for now...
                for entry in self.client.listdir_attr(path):
                    if stat.S_ISREG(entry):
                        f.writestr(entry.filename, entry.asbytes())
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

    def save(self, data):
        """Save bytes object `data` to remote location."""
        buf = io.BytesIO(data)
        if zipfile.is_zipfile(buf):
            compression = self.zip_compressions[self.options.zip_compression]
            with zipfile.ZipFile(buf, "r", compression) as f:
                for name in f.namelist():
                    self.client.  WRITE


# SFTP connection parameters

host = "nas.aimen.es"

port = 2122

username = "matchmaker"

password = "1p1B4E45pZcqYz9L@bBzb1&8d"

try:

    transport = paramiko.Transport((host, port))

     transport.connect(username=username, password=password)

    sftp = paramiko.SFTPClient.from_transport(transport)



    # List directories in the root directory

    root_path = '.'

    files_and_dirs = sftp.listdir_attr(root_path)



    # Filter out directories

    directories = [entry.filename for entry in files_and_dirs if stat.S_ISDIR(entry.st_mode)]

    print('Directories in the root directory:', directories)



    # Close the SFTP connection

    sftp.close()

    transport.close()

except Exception as e:

    print(f'An error occurred: {e}')
