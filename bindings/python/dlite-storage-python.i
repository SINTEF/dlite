/* -*- Python -*-  (not really, but good for syntax highlighting) */

/* Python-spesific extensions to dlite-storage.i */

%extend _DLiteStorage {

  %pythoncode %{
      # Override default __init__()
      def __init__(self, driver_or_url, location=None, options=None):
          from dlite.options import make_query
          if options and not isinstance(options, str):
              options = make_query(options)
          loc = str(location) if location else None
          _dlite.Storage_swiginit(self, _dlite.new_Storage(
              driver_or_url=driver_or_url, location=loc, options=options))

      def __enter__(self):
          return self

      def __exit__(self, *exc):
          del self.this

      def __repr__(self):
          return (
              f"Storage('{self.driver}', location='{self.location}', "
              f"options='{self.options}')"
          )

      def __iter__(self):
          return StorageIterator(self)

      readable = property(
          fget=lambda self: self._get_readable(),
          doc="Whether the storage is readable.")

      writable = property(
          fget=lambda self: self._get_writable(),
          doc="Whether the storage is writable.")

      generic = property(
          fget=lambda self: self._get_generic(),
          doc="Whether the storage is generic, i.e. whether the storage can "
          "hold multiple instances, including both data and metadata."
      )

      @classmethod
      def load_plugins(cls):
          """Load all storage plugins."""
          _dlite._load_all_storage_plugins()

      @classmethod
      def unload_plugin(cls, name):
          """Unload storage plugin with this name."""
          _dlite._unload_storage_plugin(str(name))

      @classmethod
      def plugin_help(cls, name):
          """Return documentation of storage plogin with this name."""
          return _dlite._storage_plugin_help(name)

      def instances(self, pattern=None):
          """Returns an iterator over all instances in storage whos
          metadata URI matches `pattern`."""
          return StorageIterator(self, pattern=pattern)

      def load(self, id, metaid=None):
          """Loads instance `id` from this storage and return it.

          If `metaid` is provided, the returned instance will be
          mapped to an instance of this type (if appropriate mappings
          are available)."""
          return Instance.from_storage(self, id, metaid)

      def save(self, inst):
          """Stores instance `inst` in this storage."""
          inst.save_to_storage(self)

      driver = property(get_driver,
                        doc='Name of driver associated with this storage')

  %}
}

%extend StorageIterator {
  %pythoncode %{

      # Override default __init__()
      def __init__(self, s, pattern=None):
          """Iterates over instances in storage `s`.

          If `pattern` is given, only instances whos metadata URI
          matches glob pattern `pattern` are returned.
          """
          _dlite.StorageIterator_swiginit(
              self, _dlite.new_StorageIterator(s, pattern))

          # Keep a reference to self, such that it is not garbage-collected
          # before end of iterations
          if not hasattr(_dlite, "_storage_iters"):
              _dlite._storage_iters = {}
          _dlite._storage_iters[id(self.state)] = self

      def __next__(self):
          inst = self.next()
          if not inst:
              # Delete reference to iterator object stored away in __init__()
              _dlite._storage_iters.pop(id(self.this), None)
              raise StopIteration()
          return instance_cast(inst)
  %}
}

%extend StoragePluginIter {
  %pythoncode %{
      # Override default __init__()
      def __init__(self):
          """Iterator over loaded storage plugins."""
          _dlite.StoragePluginIter_swiginit(
              self, _dlite.new_StoragePluginIter())
          # Keep a reference to self, such that it is not garbage-collected
          # before end of iterations
          if not hasattr(_dlite, "_storage_iters"):
              _dlite._storage_iters = {}
          _dlite._storage_iters[id(self.iter)] = self

      def __next__(self):
          name = self.next()
          if not name:
              # Delete reference to iterator object stored away in __init__()
              _dlite._storage_iters.pop(id(self.this), None)
              raise StopIteration()
          return name
  %}
}
