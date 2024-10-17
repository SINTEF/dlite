"""Template for Python storage plugins."""


class plugin_driver_name(dlite.DLiteStorageBase):
    """General description of the Python storage plugin."""

    def open(self, location, options=None):
        """Open storage.  Optional.

        Must be defined if any of the methods close(), flush(), load(),
        save(), delete() or query() are defined.

        Arguments:
            location: Path to storage.
            options: Additional options for this storage driver.
        """

    def close(self):
        """Close the storage.  Optional.
        This will automatically call flush() if flush is defined.
        """

    def flush(self):
        """Flush cached data to the storage.  Optional."""

    def load(self, id=None):
        """Load an instance from storage and return it.  Optional.

        Arguments:
            id: ID of instance to load from the storage.

        Returns:
            New instance.
        """

    def save(self, inst):
        """Save instance `inst` to storage.  Optional.

        Arguments:
            inst: Instance to save.
        """

    def delete(self, uuid):
        """Delete instance with given `uuid` from storage.  Optional.

        Arguments:
            uuid: UUID of instance to delete.
        """

    def query(self, pattern=None):
        """Queries the storage for instance UUIDs.  Optional.

        A generator method that iterates over all UUIDs in the storage
        who"s metadata URI matches glob pattern `pattern`.

        Arguments:
            pattern: Glob pattern for matching metadata URIs.

        Yields:
            Instance UUIDs mabased on the `pattern` regular expression.
            If no `pattern` is given, the UUIDs of all instances in the
            storage are yielded.
        """

    @classmethod
    def from_bytes(cls, buffer, id=None, options=None):
        """Load instance with given `id` from `buffer`.  Optional.

        Arguments:
            buffer: Bytes or bytearray object to load the instance from.
            id: ID of instance to load.  May be omitted if `buffer` only
                holds one instance.
            options: Options string for this storage driver.

        Returns:
            New instance.
        """

    @classmethod
    def to_bytes(cls, inst, options=None):
        """Save instance `inst` to bytes (or bytearray) object.  Optional.

        Arguments:
            inst: Instance to save.
            options: Options string for this storage driver.

        Returns:
            The bytes (or bytearray) object that the instance is saved to.
        """
