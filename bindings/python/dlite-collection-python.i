/* -*- Python -*-  (not really, but good for syntax highlighting) */

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

    def __str__(self):
        return str(self.asinstance())

    def __iter__(self):
        return self.get_iter()

    def __getitem__(self, label):
        return self.get(label)

    def __setitem__(self, label, inst):
        self.add(label, inst)

    def __delitem__(self, label):
        self.remove(label)

    def __len__(self):
        return self.count()

    def __contains__(self, item):
        return self.has(item)

    meta = property(get_meta, doc='Reference to metadata of this collection.')

    @classmethod
    def create(cls, id=None, lazy=True):
        return cls(id=id, lazy=lazy)

    @classmethod
    def create_from_storage(cls, storage, id=None, lazy=True):
        return cls(storage=storage, id=id, lazy=lazy)

    @classmethod
    def create_from_url(cls, url, id=None, lazy=True):
        return cls(url=url, id=id, lazy=lazy)

    def asdict(self):
        """Returns a dict representation of self."""
        return self.asinstance().asdict()

    def asjson(self, **kwargs):
        """Returns a JSON-representation of self. Arguments are passed to
        json.dumps()."""
        return self.asinstance().asjson()

    def get_relations(self, s=None, p=None, o=None):
        """Returns a generator over all relations matching the given
        values of `s`, `p` and `o`."""
        itr = self.get_iter()
        while itr.poll():
            yield itr.find(s, p, o)

    def get_instances(self):
        """Returns a generator over all instances in this collection."""
        itr = self.get_iter()
        while itr.poll():
            yield itr.next()

    def get_labels(self):
        """Returns a generator over all instances in this collection."""
        for r in self.get_relations():
            if r.p == '_has-meta':
                yield r.s

  %}
}
