"""Test plugin that only defines to_bytes() and from_bytes()."""

import dlite


class bufftest(dlite.DLiteStorageBase):
    """Test plugin that represents instances as byte-encoded json."""

    @classmethod
    def to_bytes(cls, inst, options=None):
        """Returns instance as bytes."""
        return str(inst).encode()

    @classmethod
    def from_bytes(cls, buffer, id=None, options=None):
        """Load instance from buffer."""
        return dlite.Instance.from_json(buffer.decode())
