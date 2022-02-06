/* -*- c -*-  (not really, but good for syntax highlighting) */

/* Python-spesific extensions to dlite-storage.i */

%extend _DLiteStorage {

  %pythoncode %{
      def __enter__(self):
          return self

      def __exit__(self, *exc):
          del self.this

      def __repr__(self):
          options = '?%s' % self.options if self.options else ''
          return "Storage('%s://%s%s')" % (self.driver, self.uri, options)

      def __iter__(self):
          return StorageIterator(self)

      @classmethod
      def create_from_url(cls, url):
          """Create a new storage from `url`."""
          return cls(url)


      def instances(self, pattern=None):
          """Returns an iterator over all instances in storage whos
          metadata URI matches `pattern`."""
          return StorageIterator(pattern)

      def load(self, id, metaid=None):
          """Loads instance `id` from this storage and return it.

          If `metaid` is provided, the returned instance will be
          mapped to an instance of this type (if appropriate mappings
          are available)."""
          return Instance.create_from_storage(self, id, metaid)

      def save(self, inst):
          """Stores instance `inst` in this storage."""
          inst.save(storage=self)

      driver = property(get_driver,
                        doc='Name of driver associated with this storage')
  %}
}

%extend StorageIterator {
  %pythoncode %{
      def __next__(self):
          inst = self.next()
          if not inst:
              raise StopIteration()
          return inst
  %}
}


%extend StoragePluginIter {
  %pythoncode %{
      def __next__(self):
          name = self.next()
          if not name:
              raise StopIteration()
          return name
  %}
}
