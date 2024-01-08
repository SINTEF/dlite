/* -*- python -*-  (not really, but good for syntax highlighting) */

%extend struct _FUPaths {

  %pythoncode %{
    def __contains__(self, value):
        return value in self.aslist()

    def __getitem__(self, key):
        n = len(self)
        if key < 0:
            key += n
        if key < 0 or key >= n:
            raise IndexError(f'key out of range: {key}')
        return self.getitem(key)

    def __iter__(self):

        class Iter:
            def __init__(slf):
                slf.n = 0
            def __next__(slf):
                if slf.n < len(self):
                    path = self[slf.n]
                    slf.n += 1
                    return path
                else:
                    raise StopIteration()

        return Iter()

    def aslist(self):
        return [self[i] for i in range(len(self))]

  %}
}


%pythoncode %{
storage_path = FUPath("storages")
storage_plugin_path = FUPath("storage-plugins")
mapping_plugin_path = FUPath("mapping-plugins")
python_storage_plugin_path = FUPath("python-storage-plugins")
python_mapping_plugin_path = FUPath("python-mapping-plugins")

%}
