:py:mod:`dlite.utils`
=====================

.. py:module:: dlite.utils


Module Contents
---------------

Classes
~~~~~~~

.. autoapisummary::

   dlite.utils.MissingDependencyError
   dlite.utils.CannotInferDimensionError
   dlite.utils.InvalidNumberOfDimensionsError
   dlite.utils.MetadataNotDefinedError
   dlite.utils.DictStore



Functions
~~~~~~~~~

.. autoapisummary::

   dlite.utils.uncaught_exception_hook
   dlite.utils.instance_from_dict
   dlite.utils.to_metadata
   dlite.utils.get_dataclass_entity_schema
   dlite.utils.pydantic_to_property
   dlite.utils.pydantic_to_metadata
   dlite.utils.pydantic_to_instance
   dlite.utils.get_pydantic_entity_schema
   dlite.utils.get_package_paths
   dlite.utils.infer_dimensions
   dlite.utils.get_referred_instances
   dlite.utils.get_cachedir



Attributes
~~~~~~~~~~

.. autoapisummary::

   dlite.utils.HAVE_DATACLASSES
   dlite.utils.HAVE_PYDANTIC


.. py:data:: HAVE_DATACLASSES
   :annotation: = False

   

.. py:data:: HAVE_PYDANTIC
   :annotation: = False

   

.. py:class:: MissingDependencyError

   Bases: :py:obj:`dlite.DLiteError`

   The feature you request requires installing an external package.

   Install it with

       pip install <package>

   .. py:attribute:: exit_code
      :annotation: = 44

      


.. py:class:: CannotInferDimensionError

   Bases: :py:obj:`dlite.DLiteError`

   Cannot infer instance dimensions.


.. py:class:: InvalidNumberOfDimensionsError

   Bases: :py:obj:`dlite.DLiteError`

   Invalid number of instance dimensions.


.. py:class:: MetadataNotDefinedError

   Bases: :py:obj:`dlite.DLiteError`

   Metadata is not found in the internal instance store.


.. py:exception:: PermissionWarning

   Bases: :py:obj:`Warning`

   Mission permissions.


.. py:function:: uncaught_exception_hook(exetype, value, trace)

   A exception hook that allows exceptions to define an
   `exit_code` attribute that will make Python exit with that code if
   the exception is uncaught.


.. py:class:: DictStore(*args)

   Bases: :py:obj:`dict`

   A dict subclass providing a simple store for dict-representations
   of instances (including metadata).

   The internal layout follows the multi-instance representation,
   where each top-level key-value pair is an UUID mapped to a
   dict-representation of an instance.  Metadata is represented in
   soft7 format.

   :param args: One or more to initialise the store with.

   .. py:method:: add(d, id=None)

      Add dict-representation `d` of an instance to the store.

      :param d: A dict to add to the store.
      :param id: Optional `id` of `d`.  The `id` must be consistent with
                 any 'uuid', 'uri' or 'identity' key.  This option is
                 useful in the case `d` doesn't have an 'uuid', 'uri' or
                 'identity' key.

      .. admonition:: Notes

         The layout of `d` may be any of the supported representations
         for data instances or metadata, including any combination of
         single/multi-instance and soft7 representations.  The internal
         layout is always multi-instance soft7.



.. py:function:: instance_from_dict(d, id=None, single=None, check_storages=True)

   Returns a new DLite instance created from dict.

   :param d: Dict to parse.  It should be of the same form as returned
             by the Instance.asdict() method.
   :param id: ID (URI or UUID) if instance to return.
   :param single: Unsused. Provided for backward compabitility.
   :param check_storages: bool
                          Whether to check if the instance already exists in storages
                          specified in `dlite.storage_path`.


.. py:function:: to_metadata(obj)

   Converts `obj` to dlite Metadata.


.. py:function:: get_dataclass_entity_schema()

   Returns the datamodel for dataclasses in Python standard library.


.. py:function:: pydantic_to_property(name, propdict, dimensions = None, namespace = 'http://onto-ns.com/meta', version = '0.1')

   Return a dlite property from a name and a pydantic property dict.

   :param name: Name of the property to create.
   :param propdict: Pydantic property dict.
   :param dimensions: If given, the dict will be updated with new dimensions
                      from array properties.
   :param namespace: For a reference property use this as the namespace of
                     the property to refer to.
   :param version: For a reference property use this as the version of
                   the property to refer to.

   :returns: New DLite property.


.. py:function:: pydantic_to_metadata(model, uri=None, default_namespace='http://onto-ns.com/meta', default_version='0.1', metaid=dlite.ENTITY_SCHEMA)

   Create a new dlite metadata from a pydantic model.

   :param model: A pydantic model or an instance of one to create the
                 new metadata from.
   :param uri: URI of the created metadata.  If not given, it is
               inferred from `default_namespace`, `default_version` and
               the title of the model schema.
   :param default_namespace: Default namespace used if `uri` is None.
   :param default_version: Default version used if `uri` is None.
   :param metaid: Metadata for the created metadata.  Defaults to
                  dlite.ENTITY_SCHEMA.


.. py:function:: pydantic_to_instance(meta, pydinst)

   Return a new dlite instance from a pydantic instance `pydinst`.


.. py:function:: get_pydantic_entity_schema()

   Returns the datamodel for dataclasses in Python standard library.


.. py:function:: get_package_paths()

   Returns a dict with all the DLite builtin path variables.


.. py:function:: infer_dimensions(meta, values, strict=True)

   Infer the dimensions if we should create an instance of `meta` with
   the given `values`.

   :param meta: URI or metadata object.
   :param values: Dict mapping property names to values.  Not all property
                  names needs to be mapped.
   :param strict: Whether to require that all keys in `values` correspond
                  to a property name in `meta`.

   :returns: Dict mapping dimension names to dimension values.

   :raises InvalidNumberOfDimensionsError: Inconsistent number of dimensions.
   :raises CannotInferDimensionError: Cannot infer instance dimensions.


.. py:function:: get_referred_instances(inst, include_meta=False)

   Return a set with all instances that are directly or indirectly
   referred to by `inst`.

   This function follows instances referred to by collections and via
   properties of type 'ref'.  Transaction parents are not included,
   hence the returned set only includes instances in the current
   snapshot.

   Cyclic references are handled correctly.

   If `include_meta` is true, also return metadata.

   .. admonition:: Example

      If you have a collection 'coll' with three instances 'inst1',
      'inst2' and 'inst3' and 'inst2' has a 'ref' property, which is
      an array `[inst4, inst5]`, then

          get_referred_instances(coll)

      would return the set `{coll, inst1, inst2, inst3, inst4, inst5}`.

      The same set would be returned even if one of the instances would
      have a 'ref' property referring back to 'coll'.


.. py:function:: get_cachedir(create = True)

   Returns cross-platform path to dlite cache directory.

   If `create` is true, create the cache directory if it doesn't exists.

   The XDG_CACHE_HOME environment variable is used if it exists.


