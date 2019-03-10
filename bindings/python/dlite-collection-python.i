/* -*- c -*-  (not really, but good for syntax highlighting) */

/* Python-spesific extensions to dlite-collection.i */

%extend struct _CollectionIter {

  %pythoncode %{
    def __iter__(self):
        return self

    def __next__(self):
        next = self.next()
        if next is None:
            raise StopIteration()
        return next
  %}
}


%extend struct _DLiteCollection {

  %pythoncode %{
    def __repr__(self):
        return "Collection(%r)" % (self.uri if self.uri else self.uuid)

    def __iter__(self):
        return self.get_iter()

    def relations(self, s=None, p=None, o=None):
        itr = self.get_iter()
        while itr.poll():
            yield itr.find(s, p, o)

  %}
}
