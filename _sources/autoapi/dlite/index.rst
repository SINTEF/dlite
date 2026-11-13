:py:mod:`dlite`
===============

.. py:module:: dlite


Submodules
----------
.. toctree::
   :titlesonly:
   :maxdepth: 1

   datamodel/index.rst
   dataset/index.rst
   dlite/index.rst
   factory/index.rst
   mappings/index.rst
   options/index.rst
   paths/index.rst
   protocol/index.rst
   quantity/index.rst
   rdf/index.rst
   table/index.rst
   testutils/index.rst
   utils/index.rst


Package Contents
----------------

Classes
~~~~~~~

.. autoapisummary::

   dlite.DLiteProtocolBase
   dlite.DLiteSearchError
   dlite.errctl
   dlite.HideDLiteWarnings
   dlite.StorageIterator
   dlite.Storage
   dlite.StoragePluginIter
   dlite.Dimension
   dlite.Property
   dlite.Relation
   dlite.Instance
   dlite.Metadata
   dlite.Collection
   dlite.FUPath



Functions
~~~~~~~~~

.. autoapisummary::

   dlite.errgetexc
   dlite.run_file
   dlite.instance_cast
   dlite.get_version
   dlite.get_license
   dlite.get_uuid
   dlite.get_idtype
   dlite.normalise_id
   dlite.join_meta_uri
   dlite.split_meta_uri
   dlite.join_url
   dlite.split_url
   dlite.globmatch
   dlite.errclr
   dlite.errval
   dlite.errmsg
   dlite.err_get_levelname
   dlite.err_set_levelname
   dlite.err_get_stream
   dlite.err_set_stream
   dlite.err_set_file
   dlite.errcheck
   dlite.get_warnings_hide
   dlite.set_warnings_hide
   dlite.asbool
   dlite.chk_semver
   dlite.cmp_semver
   dlite.uriencode
   dlite.uridecode
   dlite.deprecation_warning
   dlite.from_typename
   dlite.to_typename
   dlite.to_typenumber
   dlite.get_alignment
   dlite.get_instance
   dlite.has_instance
   dlite.istore_get_uuids
   dlite.standardise
   dlite.get_collection
   dlite.mapping
   dlite.python_mapping_unload
   dlite.mapping_plugin_unload
   dlite.JStore
   dlite.format_dict
   dlite.classfactory
   dlite.objectfactory
   dlite.instancefactory



Attributes
~~~~~~~~~~

.. autoapisummary::

   dlite.__version__
   dlite.is_conda_base
   dlite.msg
   dlite.dlite_INSTALL_ROOT
   dlite.exc
   dlite.DLiteStorageBase
   dlite.DLiteMappingBase
   dlite.DATA_NS
   dlite.silent
   dlite.BlobType
   dlite.BoolType
   dlite.IntType
   dlite.UIntType
   dlite.FloatType
   dlite.FixStringType
   dlite.StringPtrType
   dlite.RefType
   dlite.DimensionType
   dlite.PropertyType
   dlite.RelationType
   dlite.IDTranslateToUUID
   dlite.IDRequireUUID
   dlite.IDKeepID
   dlite.BASIC_METADATA_SCHEMA
   dlite.ENTITY_SCHEMA
   dlite.COLLECTION_ENTITY
   dlite.storage_path
   dlite.template_path
   dlite.storage_plugin_path
   dlite.mapping_plugin_path
   dlite.python_storage_plugin_path
   dlite.python_mapping_plugin_path
   dlite.python_protocol_plugin_path
   dlite.Behavior


.. py:data:: __version__
   :annotation: = '0.5.49'

   

.. py:data:: is_conda_base
   :annotation: = False

   

.. py:data:: msg
   :annotation: = Multiline-String

    .. raw:: html

        <details><summary>Show Value</summary>

    .. code-block:: text
        :linenos:

        """WARNING: You are using the Anaconda Python base environment on Windows.
        """

    .. raw:: html

        </details>

   

.. py:data:: dlite_INSTALL_ROOT
   :annotation: = '/home/runner/.local'

   

.. py:function:: errgetexc(code)

   Returns DLite exception corresponding to error code.


.. py:function:: run_file(path, globals=None, locals=None)

   Exposing PyRun_File from the Python C API.


.. py:data:: exc
   

   

.. py:data:: DLiteStorageBase
   

   

.. py:data:: DLiteMappingBase
   

   

.. py:class:: DLiteProtocolBase

   Base class for Python protocol plugins.


.. py:function:: instance_cast(inst, newtype=None)

   Return instance converted to a new instance subclass.

   By default the convertion is done to the type of the underlying
   instance object.

   Any subclass of `dlite.Instance` can be provided as `newtype`.
   Only downcasting to subclasses of `inst` are permitted.  For
   casting to other types, `dlite.DLiteTypeError` is raised.


.. py:class:: DLiteSearchError(*args, **kwargs)

   Bases: :py:obj:`dlite._dlite.DLiteLookupError`

   Deprecated.  Has been renamed to DLiteLookupError.


.. py:data:: DATA_NS
   

   

.. py:function:: get_version()

   get_version() -> char const *
   Returns the current version of DLite.



.. py:function:: get_license()

   get_license() -> char const *
   Returns DLite licenses information.



.. py:function:: get_uuid(id=None)

   get_uuid(id=None)
   Returns an UUID, depending on:

   - If `id` is NULL or empty, generates a new random version 4 UUID.
   - If `id` is not a valid UUID string, generates a new version 5 sha1-based
     UUID from `id` using the DNS namespace.

   Otherwise return `id` (which already must be a valid UUID).



.. py:function:: get_idtype(id)

   get_idtype(id) -> char const *
   Returns the type of `id`. Either Random, Hash or Copy.


.. py:function:: normalise_id(id, uri=None)

   normalise_id(id, uri=None) -> char *
   Returns normalised copy of `id` which is a valid IRI.


.. py:function:: join_meta_uri(name, version, namespace)

   join_meta_uri(name, version, namespace) -> char *
   Returns a (metadata) uri by combining `name`, `version` and `namespace` as:

       namespace/version/name



.. py:function:: split_meta_uri(uri)

   split_meta_uri(uri)


.. py:function:: join_url(driver, location, options=None, fragment=None)

   join_url(driver, location, options=None, fragment=None) -> char *
   Returns an url constructed from the arguments of the form:

       driver://location?options#fragment

   The `driver`, `options` and `fragment` arguments may be None.



.. py:function:: split_url(url)

   split_url(url)
   Returns a (driver, location, options, fragment)-tuplet by splitting
   `url` of the form

       driver://location?options#fragment

   into four parts.



.. py:function:: globmatch(pattern, s)

   globmatch(pattern, s) -> int
   Match string 's' against glob pattern 'pattern' and return zero on
   match.

   Understands the following patterns:

   - `*`: Any number of characters.
   - `?`: Any single character.
   - `[a-z]`: Any single character in the range a-z.
   - `[^a-z]`: Any single character not in the range a-z.
   - `\x`: Match x.



.. py:function:: errclr()

   errclr()
   Clear the last error (setting its error code to zero).



.. py:function:: errval()

   errval() -> int
   Returns the error code (error value) or the last error or zero if no errors
   have occured since the last call to dlite.errclr().



.. py:function:: errmsg()

   errmsg() -> char const *
   Returns the error message of the last error.  An empty string is returned if
   no errors have occured since the last call to dlite.errclr().



.. py:function:: err_get_levelname()

   err_get_levelname() -> char const *
   Get current lowest error level to report.


.. py:function:: err_set_levelname(name)

   err_set_levelname(name) -> char const *
   Set current lowest error level to report.


.. py:function:: err_get_stream()

   err_get_stream() -> FILE *


.. py:function:: err_set_stream(stream=None)

   err_set_stream(stream=None) -> FILE *


.. py:function:: err_set_file(filename)

   err_set_file(filename)
   Set error log file. Special values includes:

   - `None` or empty: Turn off error output.
   - `<stderr>`: Standard error.
   - `<stdout>`: Standard output.

   All other values are treated as a filename that will be opened in append mode.



.. py:function:: errcheck()

   errcheck()
   Just check for errors, do nothing else.


.. py:function:: get_warnings_hide()

   get_warnings_hide() -> int
   Return a list with the `[hide, pattern]` parameters for controlling whether
   warnings are shown.



.. py:function:: set_warnings_hide(hide, pattern=None)

   set_warnings_hide(hide, pattern=None)
   Sets parameters controlling whether to hide warnings.


.. py:function:: asbool(str)

   asbool(str) -> bool
   Set error stream.



.. py:function:: chk_semver(version, n=-1)

   chk_semver(version, n=-1) -> int
   Check if `version` is a valid semantic version 2.0.0 number.

   By default, the whole string is checked.  If `n` is non-negative, only
   the initial part of `version` is checked, at most `n` characters.

   Returns the length of the valid part of `version` or -1 if `version` is
   not a valid semantic number.



.. py:function:: cmp_semver(v1, v2, n=-1)

   cmp_semver(v1, v2, n=-1) -> int
   Compare semantic version strings `v1` and `v2`.

   Returns -1 if `v1 < v2`, 0 if `v1 == v2` and 1 if `v1 > v2`.

   By default, the whole of `v1` and `v2` are compared.  If `n` is non-negative,
   only the initial part of `v1` and `v2` are compared, at most `n` characters.



.. py:function:: uriencode(INPUT)

   uriencode(INPUT) -> char *
   Return percent-encoded copy of input.


.. py:function:: uridecode(INPUT)

   uridecode(INPUT) -> status_t
   Return percent-decoded copy of input.


.. py:class:: errctl(hide=(), show=(), filename='<stderr>')

   Context manager for temporary disabling specific DLite error
   messages or redirecting them.

   :param hide: Sequence of DLiteException subclasses or exception names
                corresponding to error messages to hide.  A single class or
                name is also allowed.
                May also be a boolean, in which case error messages will be
                hidden/shown.
   :param show: Sequence of DLiteException subclasses or exception names
                corresponding to error messages to show.  A single class or
                name is also allowed.
                May also be a boolean, in which case error messages will be
                shown/hidden.
   :param filename: Filename to redirect errors to.  The following values
                    are handled specially:
                    - "None" or empty: No output is written.
                    - "<stderr>": Write errors to stderr (default).
                    - "<stdout>": Write errors to stdout.

   .. py:attribute:: filename
      :annotation: = '<stderr>'

      

   .. py:method:: __enter__()


   .. py:method:: __exit__(*exc)



.. py:data:: silent
   

   Context manager for a silent code block.  Same as `errctl(hide=True)`.

.. py:class:: HideDLiteWarnings(hide=True, pattern=None)

   Context manager for hiding warnings.

   :param hide: If true, hide warnings if `pattern` is None or `pattern`
                matches the warning message.
                If false, hide warnings if `pattern` is given and don't
                match the warning message.
   :param pattern: Optional glob pattern matching the warning message.

   .. py:attribute:: hide
      :annotation: = 1

      

   .. py:attribute:: pattern
      :annotation: = None

      

   .. py:method:: __enter__()


   .. py:method:: __exit__(*exc)



.. py:function:: deprecation_warning(version_removed, descr)

   Issues a deprecation warning.

   :param version_removed: The version the deprecated feature is
                           expected to be finally removed.
   :param descr: Description of the deprecated feature.

   Returns non-zero on error.


.. py:data:: BlobType
   

   

.. py:data:: BoolType
   

   

.. py:data:: IntType
   

   

.. py:data:: UIntType
   

   

.. py:data:: FloatType
   

   

.. py:data:: FixStringType
   

   

.. py:data:: StringPtrType
   

   

.. py:data:: RefType
   

   

.. py:data:: DimensionType
   

   

.. py:data:: PropertyType
   

   

.. py:data:: RelationType
   

   

.. py:function:: from_typename(typename)

   from_typename(typename) -> posstatus_t
   Returns type number and size from given type name.


.. py:function:: to_typename(type, size=-1)

   to_typename(type, size=-1) -> char *
   Returns type name for given type number and size. If `size` is negative, only the name of `type` is returned.


.. py:function:: to_typenumber(dtypename)

   to_typenumber(dtypename) -> int
   Returns DLite type number corresponding to `dtypename`.


.. py:function:: get_alignment(type, size)

   get_alignment(type, size) -> size_t


.. py:class:: StorageIterator(s, pattern=None)

   Bases: :py:obj:`object`

   Iterates over instances in storage `s`.  If `pattern` is given, only
   instances whos metadata URI matches `pattern` are returned.


   .. py:attribute:: thisown
      

      

   
   .. py:attribute:: s
      

      

   .. py:attribute:: state
      

      

   .. py:attribute:: __swig_destroy__
      

      

   .. py:method:: next()

      next(self) -> Instance
      Returns next instance or None if exhausted.


   .. py:method:: __iter__()

      __iter__(self) -> StorageIterator


   .. py:method:: __next__()



.. py:data:: IDTranslateToUUID
   

   

.. py:data:: IDRequireUUID
   

   

.. py:data:: IDKeepID
   

   

.. py:class:: Storage(driver_or_url, location=None, options=None)

   Bases: :py:obj:`object`

   Represents a data storage.

   :param driver_or_url: Name of driver used to connect to the storage or, if `location` is not
                         given, the URL to the storage:

                             driver://location?options
   :type driver_or_url: :py:class:`string`
   :param location: The location to the storage. For file storages, this is the file name.
   :type location: :py:class:`string`
   :param options: Additional options passed to the driver as a list of semicolon-separated
                   ``key=value`` pairs. See the documentation of the individual drivers to
                   see which options they support.
   :type options: :py:class:`string`

   .. py:attribute:: thisown
      

      

   
   .. py:attribute:: location
      

      

   .. py:attribute:: options
      

      

   .. py:attribute:: flags
      

      

   .. py:attribute:: idflag
      

      

   .. py:attribute:: __swig_destroy__
      

      

   .. py:attribute:: readable
      

      

   .. py:attribute:: writable
      

      

   .. py:attribute:: generic
      

      

   .. py:attribute:: driver
      

      

   .. py:method:: flush()

      flush(self)
      Flush storage.


   .. py:method:: delete(id)

      delete(self, id)
      Delete instance with given `id`.


   .. py:method:: get_driver()

      get_driver(self) -> char const *
      Returns name of driver for this storage.


   .. py:method:: help()

      help(self) -> char *
      Returns documentation for storage plugin.


   .. py:method:: get_uuids(pattern=None)

      get_uuids(self, pattern=None) -> char **
      Returns a list of UUIDs of all instances in the storage whos metadata matches `pattern`. If `pattern` is None, all UUIDs will be returned.


   .. py:method:: __enter__()


   .. py:method:: __exit__(*exc)


   .. py:method:: __iter__()


   .. py:method:: load_plugins()
      :classmethod:

      Load all storage plugins.


   .. py:method:: plugin_help(name)
      :classmethod:

      Return documentation of storage plogin with this name.


   .. py:method:: instances(pattern=None)

      Returns an iterator over all instances in storage whos
      metadata URI matches `pattern`.


   .. py:method:: load(id, metaid=None)

      Loads instance `id` from this storage and return it.

      If `metaid` is provided, the returned instance will be
      mapped to an instance of this type (if appropriate mappings
      are available).


   .. py:method:: save(inst)

      Stores instance `inst` in this storage.



.. py:class:: StoragePluginIter

   Bases: :py:obj:`object`

   Iterates over loaded storage plugins.


   .. py:attribute:: thisown
      

      

   
   .. py:attribute:: iter
      

      

   .. py:attribute:: __swig_destroy__
      

      

   .. py:method:: next()

      next(self) -> char const *
      Returns name of next plugin or None if exhausted.



   .. py:method:: __iter__()

      __iter__(self) -> StoragePluginIter


   .. py:method:: __next__()



.. py:class:: Dimension(name, description=None)

   Bases: :py:obj:`object`

   A dimension represented by its name and description.
   Metadata can define a set of common dimensions for its properties that
   are referred to by the shape of each property.

   :param name: Dimension name.
   :param description: Dimension description.

   .. py:attribute:: thisown
      

      

   
   .. py:attribute:: name
      

      

   .. py:attribute:: description
      

      

   .. py:attribute:: __swig_destroy__
      

      

   .. py:method:: __eq__(other)

      Returns true if `other` equals self.


   .. py:method:: asdict()

      Returns a dict representation of self.


   .. py:method:: asstrings()

      Returns a representation of self as a tuple of strings.



.. py:class:: Property(name, type, ref=None, shape=None, unit=None, description=None)

   Bases: :py:obj:`object`

   Represents a property.
   All metadata must have one or more properties that define the instances
   of the metadata.

   :param name: Property name.
   :param type: Property type. Ex: 'int', 'blob14', 'float64', 'ref'...
   :param ref: Optional. URL to metadata. Only needed for `type='ref'`.
   :param shape: Optional. Specifies the dimensionality of property.  If `shape`
                 is not given, the property is a scalar (dimensionality zero).
                 It should be a sequence of dimension names.
   :param unit: Optional. The unit for properties with a unit. The unit should
                be a valid unit label defined by EMMO or a custom ontology.
   :param description: Optional: A human description of the property.

   .. py:property:: type

      Type name.

   .. py:property:: dtype

      Type number.

   .. py:property:: shape

      Array of dimension indices.

   .. py:attribute:: thisown
      

      

   
   .. py:attribute:: name
      

      

   .. py:attribute:: size
      

      

   .. py:attribute:: ref
      

      

   .. py:attribute:: ndims
      

      

   .. py:attribute:: unit
      

      

   .. py:attribute:: description
      

      

   .. py:attribute:: __swig_destroy__
      

      

   .. py:attribute:: dims
      

      

   .. py:method:: get_type()

      get_type(self) -> char *


   .. py:method:: get_dtype()

      get_dtype(self) -> int


   .. py:method:: get_shape()

      get_shape(self) -> obj_t *


   .. py:method:: set_shape(arr)

      set_shape(self, arr)


   .. py:method:: __eq__(other)

      Returns true if `other` equals self.


   .. py:method:: asdict(soft7=True, exclude_name=False)

      Returns a dict representation of self.

      :param soft7: Whether to use new SOFT7 format.
      :param exclude_name: Whether to exclude "name" from the returned dict.


   .. py:method:: asstrings()

      Returns a representation of self as a tuple of strings.



.. py:class:: Relation(s, p, o, d=None, id=None)

   Bases: :py:obj:`object`

   A DLite relation representing an RDF triple.

   :param s: Subject IRI.
   :param p: Predicate IRI.
   :param o: Either an IRI for non-literal objects or the literal value for
             literal objects.
   :param d: The datatype IRI for literal objects.  It may have three forms:

             - None: object is an IRI (rdfs:Resource).
             - Starts with '@': object is a language-tagged plain literal
               (rdf:langString). The language identifier follows the '@'-sign.
               Ex: '@en' for english.
             - Otherwise: object is a literal with datatype `d`. Ex: 'xsd:int'.

   As an internal implementation detail, relations also have an `id` field.
   It may change in the future, so please don't rely on it.


   .. py:attribute:: thisown
      

      

   
   .. py:attribute:: s
      

      

   .. py:attribute:: p
      

      

   .. py:attribute:: o
      

      

   .. py:attribute:: d
      

      

   .. py:attribute:: id
      

      

   .. py:attribute:: __swig_destroy__
      

      

   .. py:method:: __eq__(other)


   .. py:method:: copy()

      Returns a copy of self.


   .. py:method:: aspreferred()

      Returns preferred Python representation.


   .. py:method:: asdict()

      Returns a dict representation of self.


   .. py:method:: asstrings()

      Returns a representation of self as a tuple of strings.



.. py:class:: Instance(metaid=None, dims=None, id=None, url=None, storage=None, driver=None, location=None, options=None, uri=None, jsoninput=None, bsoninput=None, dimensions=None, properties=None, description=None)

   Bases: :py:obj:`object`

   Represents a DLite instance.

   This is the most central class in DLite.  It has a complex `__init__()`
   method and is intended to be instantiated with one of the following class
   methods:

   - from_metaid(cls, metaid, dimensions, id=None)
   - from_url(cls, url, metaid=None)
   - from_storage(cls, storage, id=None, metaid=None)
   - from_location(cls, driver, location, options=None, id=None)
   - from_json(cls, jsoninput, id=None, metaid=None)
   - from_bson(cls, bsoninput)
   - from_dict(cls, d, id=None, single=None, check_storages=True)
   - create_metadata(cls, uri, dimensions, properties, description)

   For details, see the documentation of the individual class methods.

   :param uuid: The UUID of the instance.
   :param uri: The URI of the instance.

   .. py:property:: meta

      Reference to the metadata of this instance.

   .. py:property:: dimensions

      Dictionary with dimensions name-value pairs.

   .. py:property:: properties

      Dictionary with property name-value pairs.

   .. py:property:: is_data

      Whether this is a data instance.

   .. py:property:: is_meta

      Whether this is a metadata instance.

   .. py:property:: is_metameta

      Whether this is a meta-metadata instance.

   .. py:property:: q

      to work with quantities

   .. py:attribute:: thisown
      

      

   .. py:attribute:: uuid
      

      

   .. py:attribute:: uri
      

      

   .. py:attribute:: __swig_destroy__
      

      

   .. py:method:: save(driver_or_url=None, location=None, options=None, storage=None)

      save(self, driver_or_url=None, location=None, options=None, storage=None)
      Saves this instance to url or storage.

      Call signatures:
          save(url)
          save(driver, location, options=None)
          save(storage=storage)



   .. py:method:: save_to_url(url)

      save_to_url(self, url)


   .. py:method:: save_to_storage(storage)

      save_to_storage(self, storage)


   .. py:method:: get_hash()

      get_hash(self) -> char *
      Returns a hash of the instance.


   .. py:method:: verify_hash(*args, **kwargs)

      verify_hash(self, hash=None, recursive=false)
      Verify the hash of the instance.

      If `hash` is not None, this function verifies that the hash of
      `inst` corresponds to the memory pointed to by `hash`.  The size
      of the memory should be `DLITE_HASH_SIZE` bytes.

      If `hash` is None and `inst` has a parent, this function will
      instead verify that the parent hash stored in `inst` corresponds
      to the value of the parent.

      If `recursive` is true, all ancestors of `inst` are also
      included in the verification.

      Raises a DLiteVerifyError exception if the hash is not valid.


   .. py:method:: snapshot()

      snapshot(self)
      Make a snapshot of the current state of the instance.  It can
      be retrieved with `get_snapshot()`.

      The instance will be a transaction whose parent is the snapshot.
      If `inst` already has a parent, that will now be the parent of
      the snapshot.

      The reason that `inst` must be mutable, is that its hash will
      change due to a change in its parent.

      The snapshot will be assigned an URI of the form
      "snapshot-XXXXXXXXXXXX" (or inst->uri#snapshot-XXXXXXXXXXXX
      if inst->uri is not NULL) where each X is replaces with a
      random character.


   .. py:method:: get_snapshot(n=1)

      get_snapshot(self, n=1) -> Instance
      Returns shapshot number `n` of the current instance, where `n`
      counts backward.  Hence, `n=0` returns the current instance,
      `n=1` returns its parent, etc...

      This function may pull snapshots back into memory. Use
      `dlite_instance_pull()` if you know the storage where the snapshots are stored.


   .. py:method:: pull_snapshot(storage, n)

      pull_snapshot(self, storage, n) -> Instance
      Like `dlite_instance_get_snapshot()`, except that possible stored
      snapshots are pulled from a specified storage to memory.

      Returns shapshot number `n` of the current instance, where `n`
      counts backward.  Hence, `n=0` returns the current instance,
      `n=1` returns its parent, etc...


   .. py:method:: push_snapshot(storage, n)

      push_snapshot(self, storage, n)
      Push all ancestors of snapshot `n` from memory to storage,
      where `n=0` corresponds to `inst`, `n=1` to the parent of
      `inst`, etc...

      No snapshot is pulled back from storage, so if the snapshots are
      already in storage, this function has no effect.

      This function starts pushing closest to the root to ensure
      that the transaction is in a consistent state at all times.


   .. py:method:: set_parent(parent)

      set_parent(self, parent)
      Turn instance `inst` into a transaction node with parent
      `parent`.  This requires that `inst` is mutable, and `parent`
      is immutable.  If `inst` already has a parent, it will be
      replaced.

      Use `dlite_instance_freeze()` and `dlite_instance_is_frozen()` to
      make and check that an instance is immutable, respectively.


   .. py:method:: freeze()

      freeze(self)
      Mark the instance as immutable.  This can never be reverted.


   .. py:method:: is_frozen()

      is_frozen(self) -> bool
      Returns whether the instance is frozen and immutable.


   .. py:method:: verify_transaction()

      verify_transaction(self) -> bool
      Verifies a transaction.


   .. py:method:: get_dimensions()

      get_dimensions(self) -> obj_t *
      Returns array with dimension sizes.


   .. py:method:: get_dimension_size(name)

      get_dimension_size(self, name) -> int
      Returns the size of dimension with given name or index.


   .. py:method:: get_dimension_size_by_index(i)

      get_dimension_size_by_index(self, i) -> int


   .. py:method:: get_property(name)

      get_property(self, name) -> obj_t *
      Returns property with given name.


   .. py:method:: get_property_by_index(i)

      get_property_by_index(self, i) -> obj_t *
      Returns property with given index.


   .. py:method:: set_property(name, obj)

      set_property(self, name, obj)
      Sets property with given name to `obj`.


   .. py:method:: set_property_by_index(i, obj)

      set_property_by_index(self, i, obj)
      Sets property with given index to `obj`.


   .. py:method:: get_property_as_string(name, width=0, prec=-2, flags=0)

      get_property_as_string(self, name, width=0, prec=-2, flags=0) -> char *
      Return property `name` as a string.

      :param width: Minimum field width. Unused if 0, auto if -1.
      :param prec: Precision. Auto if -1, unused if -2.
      :param flags: Or'ed sum of formatting flags:

                    - `0`: Default (json).
                    - `1`: Raw unquoted output.
                    - `2`: Quoted output.

      :returns: Property as a string.


   .. py:method:: set_property_from_string(name, s, flags=0)

      set_property_from_string(self, name, s, flags=0)
      Set property `name` to the value of string `s`.

      `flags` is the or'ed sum of:
        0  default (json)
        1  raw unquoted input
        2  quoted input
        4  strip initial and final spaces


   .. py:method:: has_property(name)

      has_property(self, name) -> bool
      Returns true if this instance has a property with given name.


   .. py:method:: has_property_by_index(i)

      has_property_by_index(self, i) -> bool
      Returns true if this instance has a property with given index.


   .. py:method:: has_dimension(name)

      has_dimension(self, name) -> bool
      Returns true if this instance has a dimension with given name.


   .. py:method:: has_dimension_by_index(i)

      has_dimension_by_index(self, i) -> bool
      Returns true if this instance has a dimension with given index.


   .. py:method:: asbson()

      asbson(self)
      Returns a BSON representation of self.


   .. py:method:: get_uri()

      get_uri(self) -> char *
      Returns instance uri.


   .. py:method:: __len__()

      __len__(self) -> int


   .. py:method:: __repr__()

      __repr__(self) -> char *


   .. py:method:: get_meta()

      Returns reference to metadata.


   .. py:method:: get_property_descr(name)

      Return a Property object for property `name`.


   .. py:method:: from_metaid(metaid, dimensions, id=None)
      :classmethod:

      Create a new instance of metadata `metaid`.  `dimensions` must be a
      sequence with the size of each dimension.  All values initialized
      to zero.  If `id` is None, a random UUID is generated.  Otherwise
      the UUID is derived from `id`.


   .. py:method:: load(protocol, driver, location, options=None, id=None, metaid=None)
      :classmethod:

      Load the instance from storage:

      :param protocol: Name of protocol plugin used for data transfer.
      :param driver: Name of storage plugin for data parsing.
      :param location: Location of resource.  Typically a URL or file path.
      :param options: Options passed to the protocol and driver plugins.
      :param id: ID of instance to load.
      :param metaid: If given, the instance is tried mapped to this metadata
                     before it is returned.

      :returns: Instance loaded from storage.


   .. py:method:: from_url(url, metaid=None)
      :classmethod:

      Load the instance from `url`.  The URL should be of one of the
      the following forms:

          driver://location?options#id
          protocol+driver://location?options#id
          protocol://location?driver=<driver>;options#id

      where `protocol`, `driver`, `location`, `options` and `id` are
      documented in the load() method.

      If `metaid` is provided, the instance is tried mapped to this
      metadata before it is returned.


   .. py:method:: from_storage(storage, id=None, metaid=None)
      :classmethod:

      Load the instance from `storage`.  `id` is the id of the instance
      in the storage (not required if the storage only contains more one
      instance).
      If `metaid` is provided, the instance is tried mapped to this
      metadata before it is returned.


   .. py:method:: from_location(driver, location, options=None, id=None, metaid=None)
      :classmethod:

      Load the instance from storage specified by `driver`, `location`
      and `options`.  `id` is the id of the instance in the storage (not
      required if the storage only contains more one instance).


   .. py:method:: from_json(jsoninput, id=None, metaid=None)
      :classmethod:

      Load the instance from json input.


   .. py:method:: from_bson(bsoninput)
      :classmethod:

      Load the instance from bson input.


   .. py:method:: from_dict(d, id=None, single=None, check_storages=True)
      :classmethod:

      Load the instance from dictionary.

      :param d: Dict to parse.  It should be of the same form as returned
                by the Instance.asdict() method.
      :param id: Identity of the returned instance.

                 If `d` is in single-entity form with no explicit 'uuid' or
                 'uri', its identity will be assigned by `id`.  Otherwise
                 `id` must be consistent with the 'uuid' and/or 'uri'
                 fields of `d`.

                 If `d` is in multi-entity form, `id` is used to select the
                 instance to return.
      :param single: A boolean, None or "auto".  Determines whether to
                     assume that the dict is in single-entity form.
                     If `single` is None or "auto", the form is inferred.
      :param check_storages: Whether to check if the instance already exists
                             in storages specified in `dlite.storage_path`.

      :returns: New instance.


   .. py:method:: from_bytes(driver, buffer, options=None, id=None, metaid=None)
      :classmethod:

      Load the instance with ID `id` from bytes `buffer` using the
      given storage driver.


   .. py:method:: to_bytes(driver, options=None)

      Serialise this instance to a bytes object using the given
      storage driver.

      :param driver: Storage driver to use.
      :param options: Driver options.  May be an URL query string or a dict.


   .. py:method:: create_metadata(uri, dimensions, properties, description)
      :classmethod:

      Create a new metadata entity (instance of entity schema) casted
      to an instance.


   .. py:method:: create_from_metaid(metaid, dimensions, id=None)
      :classmethod:

      Create a new instance of metadata `metaid`.  `dimensions` must be a
      sequence with the size of each dimension.  All values initialized
      to zero.  If `id` is None, a random UUID is generated.  Otherwise
      the UUID is derived from `id`.


   .. py:method:: create_from_url(url, metaid=None)
      :classmethod:

      Load the instance from `url`.  The URL should be of the form
      ``driver://location?options#id``.
      If `metaid` is provided, the instance is tried mapped to this
      metadata before it is returned.


   .. py:method:: create_from_storage(storage, id=None, metaid=None)
      :classmethod:

      Load the instance from `storage`.  `id` is the id of the instance
      in the storage (not required if the storage only contains more one
      instance).
      If `metaid` is provided, the instance is tried mapped to this
      metadata before it is returned.


   .. py:method:: create_from_location(driver, location, options=None, id=None)
      :classmethod:

      Load the instance from storage specified by `driver`, `location`
      and `options`.  `id` is the id of the instance in the storage (not
      required if the storage only contains more one instance).


   .. py:method:: get_uuids(driver, location, options=None, pattern=None)
      :classmethod:

      Returns a iterator over matching UUIDs in storage.

      :param driver: Name of storage plugin for data parsing.
      :param location: Location of resource.  Typically a URL or file path.
      :param options: Options passed to the protocol and driver plugins.
      :param pattern: A glob pattern matching metadata UUIDs.  If given,
                      only matching UUIDs will be returned.

      :returns: Iterator over all matching UUIDs in storage.


   .. py:method:: __getitem__(ind)


   .. py:method:: __setitem__(ind, value)


   .. py:method:: __contains__(item)


   .. py:method:: __getattr__(name)


   .. py:method:: __setattr__(name, value)


   .. py:method:: __dir__()


   .. py:method:: __hash__()


   .. py:method:: __eq__(other)


   .. py:method:: __str__()


   .. py:method:: copy(newid=None)

      Returns a copy of this instance.  If `newid` is given, it
      will be the id of the new instance, otherwise it will be given
      a random UUID.


   .. py:method:: __reduce__()


   .. py:method:: asdict(soft7=True, uuid=None, single=None)

      Returns a dict representation of self.

      :param soft7: Whether to structure metadata as SOFT7.
      :param uuid: Whether to include UUID in the dict.  The default is true
                   if `single=True` and URI is None, otherwise it is false.
      :param single: Whether to return in single-entity format.
                     If None, single-entity format is used for metadata and
                     multi-entity format for data instances.


   .. py:method:: asjson(indent=0, single=None, urikey=False, with_uuid=False, with_meta=False, with_arrays=False, no_parent=False, soft7=None, uuid=None)

      Returns a JSON representation of self.

      :param indent: Number of spaces to indent each line with.
      :param single: Whether to return single-entity format.  Default is
                     single-entity format for metadata and multi-entity format
                     for data instances.
      :param urikey: Use uri (if it exists) as JSON key in multi-entity format.
      :param with_uuid: Whether to include UUID in output.
      :param with_meta: Always include "meta" field, even for entities.
      :param with_arrays: Write metadata dimensions and properties as jSON
                          arrays (old format).
      :param no_parent: Do not include transaction parent info.
      :param soft7: Whether to structure metadata as SOFT7. Deprecated.
                    Use `with_arrays=False` instead.
      :param uuid: Alias for `with_uuid`. Deprecated.


   .. py:method:: tojson(indent=0, single=False, urikey=False, with_uuid=False, with_meta=False, with_arrays=False, no_parent=False, soft7=None, uuid=None)

      Deprecated alias for asjson().


   .. py:method:: get_copy()

      Returns a copy of self.

      This method is deprecated.  Use copy() instead.


   .. py:method:: get_quantity(name)


   .. py:method:: set_quantity(name, value, unit)



.. py:function:: get_instance(id, metaid = None, check_storages = True)

   Return instance with given id.

   :param id: ID of instance to return.
   :param metaid: If given, dlite will try to convert the instance to a new
                  instance of ``metaid``.
   :param check_storages: Whether to check for the instance in storages listed
                          in dlite.storage_path if the instance is not already in memory.

   :returns: DLite Instance.


.. py:function:: has_instance(*args, **kwargs)

   has_instance(id, check_storages=true) -> bool
   Returns whether an instance with `id` exists.

   If `check_storages` is true, the instance is also searched for
   in the storage plugin path.



.. py:function:: istore_get_uuids()

   istore_get_uuids() -> char **
   Returns a list of in-memory stored ids.




.. py:data:: BASIC_METADATA_SCHEMA
   

   

.. py:data:: ENTITY_SCHEMA
   

   

.. py:data:: COLLECTION_ENTITY
   

   

.. py:class:: Metadata(*args, **kwargs)

   Bases: :py:obj:`Instance`

   A subclass of Instance for metadata.

   :param uri: URI of the new metadata.
   :param dimensions: Sequence of Dimension instances describing each dimension.
   :param properties: Sequence of Property instances describing each property.
   :param description: Description of metadata.

   .. py:property:: props

      A dict mapping property name to the `Property` object for the
      described metadata.

   .. py:method:: __repr__()

      __repr__(self) -> char *


   .. py:method:: __call__(dimensions=(), properties=None, id=None, dims=None)

      Returns a new instance of this metadata.

      By default the instance is uninitialised, but with the `properties`
      argument it can be either partly or fully initialised.

      :param dimensions: Either a dict mapping dimension names to values or
                         a sequence of dimension values.
      :param properties: Dict of property name-property value pairs.  Used
                         to initialise the instance (fully or partly).  A KeyError
                         is raised if a key is not a valid property name.
      :param id: Id of the new instance.  The default is to create a
                 random UUID.
      :param dims: Deprecated alias for `dimensions`.

      :returns: New instance.


   .. py:method:: getprop(name)

      Returns the metadata property object with the given name.


   .. py:method:: dimnames()

      Returns a list of all dimension names in this metadata.


   .. py:method:: propnames()

      Returns a list of all property names in this metadata.



.. py:function:: standardise(v, prop, asdict=False)

   Represent property value `v` as a standard python type.
   If `asdict` is true, dimensions, properties and relations will be
   represented with a dict, otherwise as a list of strings.


.. py:class:: Collection(id=None)

   Bases: :py:obj:`Instance`

   A collection of instances and relations between them.

   Instances added to a collection are referred to with a label,
   which is local to the collection.

   Iteration over a collection will iterate over the instances added
   to it.  Likewise, the len() of a collection will return the number
   of instances.

   Use the `get_relations()` method (or the convenience methods
   `get_subjects()`, `get_predicates()` and `get_objects()`) to iterate
   over relations.  The number of relations is available via the
   `nrelations` property.

   Relations are `(s, p, o, d=None)`-triples with an optional fourth field
   `d`, specifying the datatype of the object.  The datatype may have the
   following values:

   - None: object is an IRI.
   - Starts with '@': object is a language-tagged plain literal.
     The language identifier follows the '@'-sign. e.g. `@en`
   - Otherwise: object is a literal with datatype `d`.

   :param id: URI or UUID of the new instance.  The default is to create a
              collection with no URI and random (version 4) UUID.

   .. py:attribute:: properties
      

      Dictionary with property name-value pairs.

   .. py:method:: load(src, location=None, options=None, id=None, lazy=True)
      :classmethod:

      Loads a collection from storage.

      :param src: Storage instance | url | driver
      :param location: File path to load from when `src` is a driver.
      :param options: str
                      Options passed to the storage plugin when `src` is a driver.
      :param id: str
                 Id of collection to load.  Needed if there are more than
                 one instance in the storage.
      :param lazy: bool
                   Whether to also save all instances referred to by the
                   collection.


   .. py:method:: save(dst, location=None, options=None, include_instances=True)

      Save collection.

      :param dst: Storage instance | url | driver
      :param location: str
                       File path to write to when `dst` is a driver.
      :param options: str
                      Options passed to the storage plugin when `dst` is a driver.
      :param include_instances: bool
                                Whether to also save all instances referred to by the
                                collection.


   .. py:method:: __repr__()

      __repr__(self) -> char *


   .. py:method:: __iter__()


   .. py:method:: __len__()

      __len__(self) -> int


   .. py:method:: __getitem__(label)


   .. py:method:: __setitem__(label, inst)


   .. py:method:: __delitem__(label)


   .. py:method:: __contains__(label)


   .. py:method:: add(label, inst, force=False)

      Add `inst` to collection with given label.
      If `force` is true, a possible existing instance will be replaced.


   .. py:method:: remove(label)

      Remove instance with given label from collection.


   .. py:method:: get(label, metaid=None)

      Return instance with given label.

      If `metaid` is given, the instance is converted to an instance
      of `metaid` using instance mappings.  If no such instance mapping
      is registered, a DLiteError is raised.


   .. py:method:: get_id(id)

      Return instance with given id.


   .. py:method:: has(label)

      Returns true if an instance has been added with the given label.


   .. py:method:: has_id(id)

      Returns true if an instance has been added with the given id.


   .. py:method:: add_relation(s, p, o, d=None)

      Add (subject, predicate, object) RDF triple to collection.


   .. py:method:: remove_relations(s=None, p=None, o=None, d=None)

      Remove all relations matching `s`, `p` and `o`.


   .. py:method:: get_first_relation(s=None, p=None, o=None, d=None)

      Returns the first relation matching `s`, `p` and `o`.
      None is returned if there are no matching relations.


   .. py:method:: get_instances(metaid=None, property_mappings=False, allow_incomplete=False, ureg=None, function_repo=None, **kwargs)

      Returns a generator over all instances in collection.

      :param metaid: If given, only instances of this metadata will be
                     returned.
      :param property_mappings: Whether to also iterate over new instances
                                of type `metaid` instantiated from property mappings.
                                Hence, if `property_mappings` is true, `metaid` is required.
      :param allow_incomplete: Allow "incomplete" property mappings.
                               Properties with no mappings will be initialised to zero.
      :param ureg: Optional custom Pint unit registry.  By default the builtin
                   Pint unit registry is used.
      :param function_repo: Repository for mapping functions.  Should map
                            function IRIs to function implementations.
      :param kwargs: Additional keyword arguments passed to mapping_route().


   .. py:method:: get_labels()

      Returns a generator over all labels.


   .. py:method:: get_relations(s=None, p=None, o=None, d=None, rettype='t')

      Returns a generator over all relations matching the given
      values of `s` (subject), `p` (predicate), `o` (object) and `d`
      (datatype). See docstring for dlite.Collection for more info.

      The following values for `rettype` are supported:
      - 'I': Iterate over instances.
      - 'R': Iterate over relation objects.
      - 'T': Iterate over (s,p,o,d) tuples.
      - 't': Iterate over (s,p,o) tuples.
      - 's': Iterate over subjects.
      - 'p': Iterate over predicates.
      - 'o': Iterate over objects.
      - 'd': Iterate over datatypes.


   .. py:method:: get_subjects(p=None, o=None, d=None)

      Returns a generator over all subjects of relations matching the
      given values of `p`, `o` and `d`.


   .. py:method:: get_predicates(s=None, o=None, d=None)

      Returns a generator over all predicates of relations matching the
      given values of `s`, `o` and `d`.


   .. py:method:: get_objects(s=None, p=None, d=None)

      Returns a generator over all subjects of relations matching the
      given values of `s`, `p` and `d`.


   .. py:method:: value(s=None, p=None, o=None, d=None, default=None, any=False)

      Return pointer to the value for a pair of two criteria.

      Useful if one knows that there may only be one value.

      :param s: Criteria to match. Two of these must not be None.
      :param p: Criteria to match. Two of these must not be None.
      :param o: Criteria to match. Two of these must not be None.
      :param d: If not None, the required datatype of literal objects.
      :param default: Value to return if no matches are found.
      :param any: If true, return first matching value.

      :returns: A pointer to the value of the `s`, `p` or `o` that is None.

      :raises DLiteTypeError: Not exactly two of `s`, `p` and `o` are None.
      :raises DLiteLookupError: No match can be found or more that one match
          if `any` is true.



.. py:function:: get_collection(id)

   Returns a new reference to a collection with given id.


.. py:class:: FUPath(pathtype)

   Bases: :py:obj:`object`

   Proxy of C _FUPaths struct.

   .. py:attribute:: thisown
      

      

   .. py:attribute:: n
      

      

   .. py:attribute:: __swig_destroy__
      

      

   .. py:method:: __repr__()

      __repr__(self) -> char *


   .. py:method:: __len__()

      __len__(self) -> int


   .. py:method:: getitem(index)

      getitem(self, index) -> char const *


   .. py:method:: __setitem__(index, path)

      __setitem__(self, index, path)


   .. py:method:: __delitem__(index)

      __delitem__(self, index)


   .. py:method:: insert(index, path)

      insert(self, index, path)


   .. py:method:: append(path)

      append(self, path)


   .. py:method:: extend(paths, pathsep=None)

      extend(self, paths, pathsep=None)


   .. py:method:: get_pathsep()

      get_pathsep(self) -> char const *


   .. py:method:: get_platform()

      get_platform(self) -> char const *


   .. py:method:: set_platform(platform)

      set_platform(self, platform)


   .. py:method:: __contains__(value)


   .. py:method:: __getitem__(index)


   .. py:method:: __iter__()


   .. py:method:: aslist()



.. py:data:: storage_path
   

   

.. py:data:: template_path
   

   

.. py:data:: storage_plugin_path
   

   

.. py:data:: mapping_plugin_path
   

   

.. py:data:: python_storage_plugin_path
   

   

.. py:data:: python_mapping_plugin_path
   

   

.. py:data:: python_protocol_plugin_path
   

   

.. py:function:: mapping(output_uri, instances)

   mapping(output_uri, instances) -> Instance
     Returns a new instance of metadata `output_uri` by mapping the
     input instances.



.. py:function:: python_mapping_unload()

   python_mapping_unload()
     Unloads all currently loaded mappings.



.. py:function:: mapping_plugin_unload(name=None)

   mapping_plugin_unload(name=None) -> int


.. py:data:: Behavior
   

   

.. py:function:: JStore()

   JStore() -> _JStore
   Store for JSON data.


.. py:function:: format_dict(d, id=None, soft7=True, single=None, with_uuid=None, with_meta=False, with_parent=True, urikey=False)

   Return a copy of `d` formatted according to the given options.

   :param d: Input dict. This should be a dict-representation of a DLite
             instance.
   :param id: If given, return dict-representation of this id.
              Otherwise, return dict-representation of the store.
   :param soft7: Whether to use soft7 formatting.
   :param single: Whether to return in single-instance format.
                  If None, single-instance format is used for metadata and
                  multi-instance format for data instances.
   :param with_uuid: Whether to include UUID in the dict.  The default
                     is true if `single=True` and URI is None, otherwise it
                     is false.
   :param with_meta: Whether to always include "meta" (even for metadata)
   :param with_parent: Whether to include parent info for transactions.
   :param urikey: Whether the URI is the preferred keys in multi-instance
                  format.

   .. admonition:: Notes

      This method works with the dict-representation and does not
      access instances.  The only exception is when `d` corresponds to
      a data instance who's dimensions is a list of dimension lengths.
      In this case the metadata is needed to get dimension names.


.. py:function:: classfactory(theclass, meta = None, url = None, storage = None, id = None)

   Factory function that returns a new class that inherits from both
   `theclass` and `BaseExtension`.

   :param theclass: The class to extend.
   :param meta: Metadata instance.
   :param url: If given and `meta` is not given, load metadata from this URL.
               It should be of the form `driver://location?options#id`.
   :param storage: If given and `meta` and `url` are not given, load metadata
                   from this storage.
   :param id: A unique ID referring to the metadata if `storage` is provided.

   :returns: A new class based on `theclass` and `BaseExtensions`.


.. py:function:: objectfactory(obj, meta = None, deepcopy = False, cls = None, url = None, storage = None, id = None, instanceid = None)

   Returns an extended copy of `obj`.

   The `url`, `storage`, and `id` arguments are passed to `classfactory()`.

   :param obj: A Python object.
   :param meta: A DLite Metadata Instance.
   :param deepcopy: Whether to perform a deep copy.  Otherwise a shallow copy
                    is performed.
   :param cls: A class to use for the new object. If this is not supplied,
               the new object will be of the same class as the original.
   :param url: If given and `meta` is not given, load metadata from this URL.
               It should be of the form `driver://location?options#id`.
   :param storage: If given and `meta` and `url` are not given, load metadata
                   from this storage.
   :param id: A unique ID referring to the metadata if `storage` is provided.
   :param instanceid: A DLite ID (UUID) to use for the underlying DLite Instance.

   :returns: A new, extended copy of the Python object `obj`.


.. py:function:: instancefactory(theclass, inst)

   Returns an extended instance of `theclass` initiated from dlite
   instance ``inst``.

   :param theclass: The class to instantiate an object from using `inst`.
   :param inst: A DLite Instance to use as source for a `theclass` instance
                object.

   :returns: A `theclass` instance object based on the DLite Instance `inst`.


