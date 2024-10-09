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

# Update default search paths
from pathlib import Path
pkgdir = Path(__file__).resolve().parent
sharedir = pkgdir / "share" / "dlite"
if (sharedir / "storages").exists():
    storage_path[-1] = sharedir / "storages"
    #storage_path.append(sharedir / "storages")
if (sharedir / "storage-plugins").exists():
    storage_plugin_path[-1] = sharedir / "storage-plugins"
    #storage_plugin_path.append(sharedir / "storage-plugins")
if (sharedir / "mapping-plugins").exists():
    mapping_plugin_path[-1] = sharedir / "mapping-plugins"
    #mapping_plugin_path.append(sharedir / "mapping-plugins")
if (sharedir / "python-storage-plugins").exists():
    python_storage_plugin_path[-1] = sharedir / "python-storage-plugins"
    #python_storage_plugin_path.append(sharedir / "python-storage-plugins")
if (sharedir / "python-mapping-plugins").exists():
    python_mapping_plugin_path[-1] = sharedir / "python-mapping-plugins"
    #python_mapping_plugin_path.append(sharedir / "python-mapping-plugins")
if (sharedir / "python-protocol-plugins").exists():
    python_protocol_plugin_path[-1] = sharedir / "python-protocol-plugins"
    #python_protocol_plugin_path.append(sharedir / "python-protocol-plugins")

%}
