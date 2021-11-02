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
    def __init__(self, url=None, storage=None, id=None, lazy=False):
      """Returns a collection instance.

      Args:
          url: Loads collection from `url`, which should be of the form
            ``driver://location?options#id``.
          storage: Loads collection from `storage` object.
          id: The id of the collection to load (if either `url` or `storage`
            are given) or create (if neither `url` or `storage` are given).
          laze: Whether to load instances on demand.  If False, all instances
            are loaded immediately.

      If neither `url` or `storage` are given, a new empty collection
      is created.
      """
      if url and storage:
          raise ValueError(f'url={url} and storage={storage} cannot be given '
                           'simuntaniously.')
      if url:
          _dlite.Collection_swiginit(self, _dlite.new_Collection(
              url, int(lazy)))
      elif storage:
          _dlite.Collection_swiginit(self, _dlite.new_Collection(
              storage, id, int(lazy)))
      else:
          _dlite.Collection_swiginit(self, _dlite.new_Collection(id))

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

  %}
}
