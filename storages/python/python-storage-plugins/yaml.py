"""A simple demonstrage of a DLite storage plugin written in Python."""
import os
from typing import TYPE_CHECKING

import yaml as pyyaml  # To not clash with the current file name.
from dlite.options import Options
from dlite.utils import instance_from_dict

import dlite

if TYPE_CHECKING:  # pragma: no cover
    from typing import Generator, Optional


class yaml(dlite.DLiteStorageBase):
    """DLite storage plugin for YAML."""

    def open(self, uri: str, options: "Optional[str]" = None) -> None:
        """Opens `uri`.

        Parameters:
            uri: A fully resolved URI to the PostgreSQL database.
            options: Supported options:

                - `mode`: Mode for opening.
                  Valid values are:

                  - `a`: Append to existing file or create new file (default).
                  - `r`: Open existing file for read-only.
                  - `w`: Truncate existing file or create new file.

                - `soft7`: Whether to save using SOFT7 format.
                - `single`: Whether the input is assumed to be in single-entity form.
                  The default (`"auto"`) will try to infer it automatically.

        """
        self.options = Options(options, defaults="mode=a;soft7=true;single=auto")
        self.mode = {"r": "r", "w": "w", "a": "r+", "append": "r+"}[self.options.mode]
        self.readable = "r" in self.mode
        self.writable = "r" != self.mode
        self.generic = True
        self.uri = uri
        self._data = {}

        if self.mode in ("r", "r+"):
            with open(uri, self.mode) as handle:
                data = pyyaml.safe_load(handle)
            if data:
                self._data = data

    def close(self) -> None:
        """Closes this storage."""
        if self.writable:
            mode = (
                "w" if self.mode == "r+" and not os.path.exists(self.uri) else self.mode
            )
            with open(self.uri, mode) as handle:
                pyyaml.dump(
                    self._data,
                    handle,
                    default_flow_style=False,
                    sort_keys=False,
                )

    def load(self, id: str) -> dlite.Instance:
        """Loads `uuid` from current storage and return it as a new instance.

        Parameters:
            id: A UUID representing a DLite Instance to return from the RDF storage.

        Returns:
            A DLite Instance corresponding to the given `id` (UUID).

        """
        return instance_from_dict(
            self._data,
            id,
            single=self.options.single,
            check_storages=False,
        )

    def save(self, inst: dlite.Instance) -> None:
        """Stores `inst` in current storage.

        Parameters:
            inst: A DLite Instance to store in the RDF storage.

        """
        self._data[inst.uuid] = inst.asdict(soft7=dlite.asbool(self.options.soft7))

    def queue(self, pattern: "Optional[str]" = None) -> "Generator[str, None, None]":
        """Generator method that iterates over all UUIDs in the storage
        who"s metadata URI matches glob pattern `pattern`.

        Parameters:
            pattern: A regular expression to filter the yielded UUIDs.

        Yields:
            DLite Instance UUIDs based on the `pattern` regular expression.
            If no `pattern` is given, all UUIDs are yielded from within the RDF
            storage.

        """
        for uuid, inst_as_dict in self._data.items():
            if pattern and dlite.globmatch(pattern, inst_as_dict["meta"]):
                continue
            yield uuid
