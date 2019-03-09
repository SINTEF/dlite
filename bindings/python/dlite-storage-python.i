/* -*- c -*-  (not really, but good for syntax highlighting) */

/* Python-spesific extensions to dlite-storage.i */

%extend _DLiteStorage {

  void _close(void) {
    dlite_storage_close($self);
  }

  %pythoncode %{
      def __enter__(self):
          return self

      def __exit__(self, *exc):
          self._close()

      def __repr__(self):
          options = '?%s' % self.options if self.options else ''
          return "Storage('%s://%s%s')" % (self.driver, self.uri, options)

      driver = property(get_driver,
                        doc='Name of driver associated with this storage')
  %}
}
