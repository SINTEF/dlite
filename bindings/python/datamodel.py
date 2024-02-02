"""Module that simplifies creating a data model."""
import ast
import warnings

import dlite
from dlite.utils import instance_from_dict


class DataModelError(dlite.DLiteError):
    """Raised if the datamodel is inconsistent."""


class MissingDimensionError(DataModelError):
    """Raised if a dimension referred to in a property is not defined."""


class UnusedDimensionError(DataModelError):
    """Raised if a dimension is not referred to in any property."""


class DataModel:
    """Class for creating a data model."""

    def __init__(self, uri, schema=None, description=None):
        """Initialise

        Parameters:
            uri: URI identifying the data model.
            schema: URI of the metadata for the data model.  Defaults to
              EntitySchema.
            description: Human description of the data model.
        """
        self.uri = uri
        self._set_schema(schema)
        self.description = description
        self.dimensions = {}
        self.properties = {}
        self.relations = []

    schema = property(
        lambda self: self._schema,
        lambda self, schema: self._set_schema(schema),
        doc="Meta-metadata for the datamodel.",
    )

    def _set_schema(self, schema):
        if not schema:
            self._schema = dlite.get_instance(dlite.ENTITY_SCHEMA)
        elif isinstance(schema, dlite.Instance):
            self._schema = schema
        elif isinstance(schema, str):
            self._schema = dlite.get_instance(schema)
        else:
            TypeError("`schema` must be a string or a DLite metadata schema.")

    def add_dimension(self, name, description):
        """Add dimension with given `name` and description to data model."""
        if name in self.dimensions:
            raise KeyError(f'A dimension named "{name}" already exists')
        self.dimensions[name] = dlite.Dimension(name, description)

    def add_property(
        self, name, type, shape=None, unit=None, description=None, dims=None
    ):
        """Add property to data model.

        Parameters:
            name: Property label.
            type: Property type.
            shape: Shape of Property.  Default is scalar.
            unit: Unit. Default is dimensionless.
            description: Human description.
            dims: Deprecated alias for `shape`.
        """
        if dims:
            if not shape:
                shape = dims
            warnings.warn(
                "Argument `dims` is deprecated. Use `shape` instead.",
                DeprecationWarning,
                stacklevel=2,
            )

        if name in self.properties:
            raise KeyError(f'A property named "{name}" already exists')

        self.properties[name] = dlite.Property(
            name=name,
            type=type,
            shape=shape,
            unit=unit,
            description=description,
        )

    def _get_dims_variables(self):
        """Returns a set of all dimension names referred to in property shapes.
        """
        names = set()
        for prop in self.properties.values():
            if prop.shape is not None:
                for dim in prop.shape:
                    tree = ast.parse(dim)
                    names.update(
                        node.id
                        for node in ast.walk(tree)
                        if isinstance(node, ast.Name)
                    )
        return names

    def get_missing_dimensions(self):
        """Returns a set of dimension names referred to in property shapes but
        is not in dimensions.
        """
        return self._get_dims_variables().difference(self.dimensions)

    def get_unused_dimensions(self):
        """Returns a set of dimensions not referred to in any property shapes.
        """
        return set(self.dimensions).difference(self._get_dims_variables())

    def validate(self):
        """Raises an exception if there are missing or unused dimensions."""
        missing = self.get_missing_dimensions()
        if missing:
            raise MissingDimensionError(f"Missing dimensions: {missing}")
        unused = self.get_unused_dimensions()
        if unused:
            raise UnusedDimensionError(f"Unused dimensions: {unused}")

    def get(self):
        """Returns a DLite Metadata created from the datamodel."""
        self.validate()
        shape = [len(self.dimensions), len(self.properties)]
        if 'nrelations' in self.schema:
            shape.append(len(self.relations))

        # Hmm, there seems to be a bug when instantiating from schema.
        # The returned metadata seems not to be initialised, i.e.
        # dlite_meta_init() seems not be called on it...
        #
        # For now, lets assume that it is EntitySchema.
        if self.schema.uri != dlite.ENTITY_SCHEMA:
            raise NotImplementedError(
                f"Currently only entity schema is supported"
            )

        #meta = self.schema(shape, id=self.uri)
        #meta.description = self.description
        #meta['dimensions'] = list(self.dimensions.values())
        #meta['properties'] = list(self.properties.values())
        #if 'relations' in meta:
        #    meta['relations'] = self.relations
        # return meta

        return dlite.Instance.create_metadata(
            uri=self.uri,
            dimensions=list(self.dimensions.values()),
            properties=list(self.properties.values()),
            description=self.description,
        )
