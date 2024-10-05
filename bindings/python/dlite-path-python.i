/* -*- python -*-  (not really, but good for syntax highlighting) */

%extend struct _FUPaths {

  %pythoncode %{
    def __contains__(self, value):
        return value in self.aslist()

    def __getitem__(self, index):
        n = len(self)
        if index < 0:
            index += n
        if index < 0 or index >= n:
            raise IndexError(f'index out of range: {index}')
        return self.getitem(index)

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
python_protocol_plugin_path = FUPath("python-protocol-plugins")

%}
