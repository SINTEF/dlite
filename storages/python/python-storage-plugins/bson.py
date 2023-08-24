"""A DLite storage plugin for BSON written in Python."""
import os
from typing import TYPE_CHECKING

import bson as pybson  # Must be pymongo.bson

import dlite
from dlite.options import Options
from dlite.utils import instance_from_dict

if TYPE_CHECKING:  # pragma: no cover
    from typing import Generator, Optional


class bson(dlite.DLiteStorageBase):
    """DLite storage plugin for BSON."""

    def open(self, uri: str, options: "Optional[str]" = None) -> None:
        """Open `uri`.

        Parameters:
            uri: A fully resolved URI to the BSON file.
            options: Supported options:

                - `mode`: Mode for opening.
                  Valid values are:

                  - `a`: Append to existing file or create new file (default).
                  - `r`: Open existing file for read-only.
                  - `w`: Truncate existing file or create new file.

                - `soft7`: Whether to save using the SOFT7 format.

        After the options are passed, this method may set attribute
        ``writable`` to ``True`` if it is writable and to ``False`` otherwise.
        If ``writable`` is not set, it is assumed to be ``True``.

        The BSON data is translated to JSON.
        """
        self.options = Options(options, defaults="mode=a;soft7=true")
        self.mode = dict(r="rb", w="wb", a="rb+", append="rb+")[
            self.options.mode
        ]
        if self.mode == "rb" and not os.path.exists(uri):
            raise FileNotFoundError(f"Did not find URI {uri!r}")

        self.readable = "rb" in self.mode
        self.writable = "rb" != self.mode
        self.generic = True
        self.uri = uri
        self._data = {}
        if self.mode in ("rb", "rb+"):
            with open(uri, self.mode) as handle:
                bson_data = handle.read()

            if pybson.is_valid(bson_data):
                self._data = pybson.decode(bson_data)
                if not self._data:
                    raise EOFError(f"Failed to read BSON data from {uri!r}")
            else:
                raise EOFError(f"Invalid BSON data in source {uri!r}")

    def close(self) -> None:
        """Close this storage and write the data to file.

        Assumes the data to store is in JSON format.
        """
        if self.writable:
            if self.mode == "rb+" and not os.path.exists(self.uri):
                mode = "wb"
            else:
                mode = self.mode

            for uuid in self.queue():
                props = self._data[uuid]["properties"]
                if isinstance(props, dict):  # Metadata props is list
                    for key in props:
                        if isinstance(props[key], (bytearray, bytes)):
                            props[key] = props[key].hex()
                    self._data[uuid]["properties"] = props

            with open(self.uri, mode) as handle:
                handle.write(pybson.encode(self._data))

    def load(self, uuid: str) -> dlite.Instance:
        """Load `uuid` from current storage and return it as a new instance.

        Parameters:
            uuid: A UUID representing a DLite Instance to return from the RDF storage.

        Returns:
            A DLite Instance corresponding to the given UUID.

        """
        if uuid in self._data:
            return instance_from_dict(self._data[uuid])
        raise KeyError(f"Instance with ID {uuid!r} not found")

    def save(self, inst: dlite.Instance) -> None:
        """Store `inst` in the current storage.

        Parameters:
            inst: A DLite Instance to store in the BSON storage.

        """
        self._data[inst.uuid] = inst.asdict(
            soft7=dlite.asbool(self.options.soft7)
        )

    def queue(
        self, pattern: "Optional[str]" = None
    ) -> "Generator[str, None, None]":
        """Generator method that iterates over all UUIDs in the storage whose metadata
        URI matches global pattern.

        Parameters:
            pattern: A regular expression to filter the yielded UUIDs.

        Yields:
            DLite Instance UUIDs based on the `pattern` regular expression.
            If no `pattern` is given, all UUIDs are yielded from within the RDF
            storage.

        """
        for uuid, data in self._data.items():
            if pattern and dlite.globmatch(pattern, data["meta"]):
                continue
            yield uuid
