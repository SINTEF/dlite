"""DLite YAML storage plugin written in Python."""
import os
from typing import TYPE_CHECKING

import yaml as pyyaml  # To not clash with the current file name.

import dlite
from dlite.options import Options
from dlite.utils import instance_from_dict

if TYPE_CHECKING:  # pragma: no cover
    from typing import Generator, Optional


# Mapping from deprecated option names to their canonical names, for
# consistency with the json plugin.
_DEPRECATED_OPTION_NAMES = {
    "with_uuid": "with-uuid",
    "with_meta": "with-meta",
    "urikey": "uri-key",
}


def _update_deprecated_options(opts):
    """Replaces deprecated option names in dict-like `opts` with their
    canonical names, issuing a deprecation warning for each use.
    """
    for old, new in _DEPRECATED_OPTION_NAMES.items():
        if old in opts:
            dlite.deprecation_warning(
                "0.7.0",
                f"yaml option `{old}` is deprecated, use `{new}` instead.",
            )
            if new not in opts:
                opts[new] = opts[old]


def _get_options(options, defaults):
    """Returns an Options object for `options` with default values given by
    the `defaults` option string.

    Deprecated option names in `options` are translated to their canonical
    names before the defaults are applied, such that explicitly-provided
    deprecated options take precedence over the default values.
    """
    opts = Options(options)
    _update_deprecated_options(opts)
    for key, value in Options(defaults).items():
        opts.setdefault(key, value)
    return opts


class yaml(dlite.DLiteStorageBase):
    """DLite storage plugin for YAML."""

    _pyyaml = pyyaml  # Keep a reference to pyyaml to have it during shutdown

    def open(self, location: str, options=None):
        """Opens `location`.

        Arguments:
            location: Path to YAML file.
            options: Supported options:
            - `mode`: Mode for opening.  Valid values are:
                - `a`: Open for writing, add to existing `location` (default).
                - `r`: Open existing `location` for reading.
                - `w`: Open for writing. If `location` exists, it is truncated.
            - `soft7`: Whether to save using SOFT7 format.
            - `single`: Whether to save in single-instance form.
            - `with-uuid`: Whether to include UUID when saving.
            - `with-meta`: Whether to always include "meta" (even for
                metadata).
            - `with_parent`: Whether to include parent info for transactions.
            - `uri-key`: Whether the URI is the preferred keys in
                multi-instance format.

            The option names `with_uuid`, `with_meta` and `urikey` are
            deprecated aliases for `with-uuid`, `with-meta` and `uri-key`,
            respectively.
        """
        df = "mode=a;soft7=true;with-meta=false;with_parent=true;uri-key=false"
        self.options = _get_options(options, df)
        mode = self.options.mode
        self.writable = "w" in mode or "a" in mode
        self.generic = True
        self.location = location
        self.flushed = True  # whether buffered data has been written to file
        self._store = dlite.JStore()  # data buffer
        if "r" in mode or "a" in mode:
            with open(location, "r") as f:
                data = pyyaml.safe_load(f)
            if data:
                self._store.load_dict(data)

        self.with_uuid = None
        if "with-uuid" in self.options:
            self.with_uuid = dlite.asbool(self.options["with-uuid"])

        self.single = None
        if "single" in self.options:
            self.single = dlite.asbool(self.options.single)

    def flush(self):
        """Flush cached data to storage."""
        if self.writable and not self.flushed:
            with open(self.location, "w") as f:
                self._pyyaml.safe_dump(
                    self._store.get_dict(
                        soft7=dlite.asbool(self.options.soft7),
                        single=self.single,
                        with_uuid=self.with_uuid,
                        with_meta=dlite.asbool(self.options["with-meta"]),
                        with_parent=dlite.asbool(self.options.with_parent),
                        urikey=dlite.asbool(self.options["uri-key"]),
                    ),
                    f,
                    default_flow_style=False,
                    sort_keys=False,
                )
            self.flushed = True

    def load(self, id):
        """Loads `id` from current storage and return it as a new instance.

        Arguments:
            id: UUID or URI of DLite Instance to return from the storage.

        Returns:
            A DLite Instance corresponding to the given `id`.
        """
        inst = self._store.get(id)
        # Ensure metadata in single-instance form is always read-only
        if inst.is_meta and self.single:
            self.writable = False
        return inst

    def save(self, inst):
        """Stores `inst` in current storage.

        Arguments:
            inst: A DLite Instance to store in the storage.

        """
        self._store.add(inst)
        self.flushed = False

    def delete(self, id):
        """Delete instance with given `id` from storage.

        Arguments:
            id: UUID or URI of instance to delete.
        """
        self._store.remove(id)
        self.flushed = False

    def query(self, pattern=None):
        """Generator method that iterates over all UUIDs in the storage
        who"s metadata URI matches glob pattern `pattern`.

        Arguments:
            pattern: A glob pattern to filter the yielded UUIDs.

        Yields:
            DLite Instance UUIDs based on `pattern`.
            If no `pattern` is given, all UUIDs are yielded from within the
            storage.

        """
        for id in self._store.get_ids(pattern):
            yield id

    @classmethod
    def from_bytes(cls, buffer, id=None, options=None):
        """Load instance with given `id` from `buffer`.

        Arguments:
            buffer: Bytes or bytearray object to load the instance from.
            id: ID of instance to load.  May be omitted if `buffer` only
                holds one instance.
            options: Unused.

        Returns:
            New instance.
        """
        return instance_from_dict(
            pyyaml.safe_load(buffer),
            id,
            check_storages=False,
        )

    @classmethod
    def to_bytes(cls, inst, options=None):
        """Save instance `inst` to bytes (or bytearray) object.  Optional.

        Arguments:
            inst: Instance to save.
            options: Supported options:
            - `soft7`: Whether to structure metadata as SOFT7.
            - `with-uuid`: Whether to include UUID in the output.
            - `single`: Whether to write in single-entity form.

        The option names `with_uuid` is a deprecated alias for
        `with-uuid`.

        Returns:
            The bytes (or bytearray) object that the instance is saved to.
        """
        opts = _get_options(
            options, defaults="soft7=true;with-uuid=false;single=true"
        )
        return pyyaml.safe_dump(
            inst.asdict(
                soft7=dlite.asbool(opts.soft7),
                uuid=dlite.asbool(opts["with-uuid"]),
                single=dlite.asbool(opts.single),
            ),
            default_flow_style=False,
            sort_keys=False,
        ).encode()
