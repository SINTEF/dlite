"""DLite YAML storage plugin written in Python."""
import os
from typing import TYPE_CHECKING

import yaml as pyyaml  # To not clash with the current file name.

import dlite
from dlite.options import Options
from dlite.utils import instance_from_dict

if TYPE_CHECKING:  # pragma: no cover
    from typing import Generator, Optional


class yaml(dlite.DLiteStorageBase):
    """DLite storage plugin for YAML."""

    _pyyaml = pyyaml  # Keep a reference to pyyaml to have it during shutdown

    def open(self, location: str, options=None):
        """Opens `location`.

        Arguments:
            location: Path to YAML file.
            options: Supported options:
            - `mode`: Mode for opening.  Valid values are:
                - `a`: Append to existing file or create new file (default).
                - `r`: Open existing file for read-only.
                - `w`: Truncate existing file or create new file.
            - `soft7`: Whether to save using SOFT7 format.
            - `single`: Whether the input is assumed to be in single-entity form.
              If "auto" (default) the form will be inferred automatically.
            - `with_uuid`: Whether to include UUID when saving.
        """
        self.options = Options(
            options, defaults="mode=a;soft7=true;single=auto;with_uuid=false"
        )
        self.readable = "r" in self.options.mode
        self.writable = "r" != self.options.mode
        self.generic = True
        self.location = location
        self.flushed = False  # whether buffered data has been written to file
        self._data = {}  # data buffer
        if self.options.mode in ("r", "a", "append"):
            with open(location, "r") as f:
                data = pyyaml.safe_load(f)
            if data:
                self._data = data

        self.single = (
            "properties" in self._data
            if self.options.single == "auto"
            else dlite.asbool(self.options.single)
        )
        self.with_uuid = dlite.asbool(self.options.with_uuid)

    def flush(self):
        """Flush cached data to storage."""
        if self.writable and not self.flushed:
            with open(self.location, "w") as f:
                self._pyyaml.safe_dump(
                    self._data,
                    f,
                    default_flow_style=False,
                    sort_keys=False,
                )
            self.flushed = True

    def load(self, id: str):
        """Loads `uuid` from current storage and return it as a new instance.

        Arguments:
            id: A UUID representing a DLite Instance to return from the
                storage.

        Returns:
            A DLite Instance corresponding to the given `id` (UUID).
        """
        inst = instance_from_dict(
            self._data,
            id,
            single=self.options.single,
            check_storages=False,
        )
        # Ensure metadata in single-entity form is always read-only
        if inst.is_meta and self.single:
            self.writable = False

        return inst

    def save(self, inst: dlite.Instance):
        """Stores `inst` in current storage.

        Arguments:
            inst: A DLite Instance to store in the storage.

        """
        self._data[inst.uuid] = inst.asdict(
            soft7=dlite.asbool(self.options.soft7),
            single=True,
            uuid=self.with_uuid,
        )
        self.flushed = False

    def delete(self, uuid):
        """Delete instance with given `uuid` from storage.

        Arguments:
            uuid: UUID of instance to delete.
        """
        del self._data[uuid]
        self.flushed = False

    def queue(self, pattern=None):
        """Generator method that iterates over all UUIDs in the storage
        who"s metadata URI matches glob pattern `pattern`.

        Arguments:
            pattern: A glob pattern to filter the yielded UUIDs.

        Yields:
            DLite Instance UUIDs based on `pattern`.
            If no `pattern` is given, all UUIDs are yielded from within the
            storage.

        """
        for uuid, inst_as_dict in self._data.items():
            if pattern and dlite.globmatch(pattern, inst_as_dict["meta"]):
                continue
            yield uuid

    @classmethod
    def from_bytes(cls, buffer, id=None):
        """Load instance with given `id` from `buffer`.

        Arguments:
            buffer: Bytes or bytearray object to load the instance from.
            id: ID of instance to load.  May be omitted if `buffer` only
                holds one instance.

        Returns:
            New instance.
        """
        return instance_from_dict(
            pyyaml.safe_load(buffer),
            id,
            check_storages=False,
        )

    @classmethod
    def to_bytes(cls, inst, soft7=True, with_uuid=False):
        """Save instance `inst` to bytes (or bytearray) object.  Optional.

        Arguments:
            inst: Instance to save.
            soft7: Whether to structure metadata as SOFT7.
            with_uuid: Whether to include UUID in the output.

        Returns:
            The bytes (or bytearray) object that the instance is saved to.
        """
        return pyyaml.safe_dump(
            inst.asdict(soft7=soft7, uuid=with_uuid),
            default_flow_style=False,
            sort_keys=False,
        ).encode()
