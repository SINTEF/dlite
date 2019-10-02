import os
import dlite


class DictDataModel:
    """
    """
    def __init__(self, d):
        self.d = d

    def getMetaURI(self):
        """Returns metadata uri for this datamodel."""
        obj = self.d['meta']
        if isinstance(obj, str):
            return obj
        else:
            return '/'.join([obj['namespace'], obj['version'], obj['name']])

    def getProperty(self, name, type, size, ndims, dims):
        """Returns value of property `name`.

        The expected type, size, number of dimensions and size of each
        dimension of the memory is described by `type`, `size`,
        `ndims` and `dims`, respectively.
        """
        uri = self.getMetaURI()
        meta = dlite.get_instance(uri)
        if meta.is_metameta:
            for p in self.d['properties']:
                if p['name'] == name:
                    prop = dlite.Property(name, type,
            raise ValueError('No such property: %s' % name)
        else:
            pass

    # Optional
    def setMetaURI(self, uri):
        """Sets the metadata uri for this datamodel."""
        pass

    def setDimensionSize(self, name, size):
        """Sets size of dimension `name` to `size`."""

    def setProperty(self, name, value, type, size, ndims, dims):
        """Sets property `name` to `value`.

        The expected type, size, number of dimensions and size of each
        dimension of the memory is described by `type`, `size`,
        `ndims` and `dims`, respectively."""
        pass

    def hasDimension(self, name):  # ???
        """Returns a positive value if dimension `name` is defined, zero if it
        isn't and a negative value on error."""
        pass

    def hasProperty(self, name):  # ???
        """Returns a positive value if property `name` is defined, zero if it
        isn't and a negative value on error."""
        pass

    def getURI(self):
        """Returns URI """
        pass
