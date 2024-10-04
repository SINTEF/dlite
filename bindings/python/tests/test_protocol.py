"""Implements a protocol plugins."""

import dlite


class Protocol:
    """Provides an interface to protocol plugins.

    Arguments:
        name: Name of protocol.
    """

    def __init__(self, name, options=None):
        pass

    def load_plugins(self):
        pass
