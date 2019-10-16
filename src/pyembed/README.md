The directory provides code for embedding Python into dlite.

This is used to make it possible for rapid scripting of
application-specific storage driver and mapping plugins by embedding
Python.

Instances/entities are communicated between C and Python by UUID
rather than by pointer, making it possible to safely catch errors
rather than getting segfaults.
