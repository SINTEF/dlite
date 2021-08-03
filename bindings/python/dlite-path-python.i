/* -*- python -*-  (not really, but good for syntax highlighting) */

%extend struct _FUPaths {

  %pythoncode %{
    def __contains__(self, value):
        return value in self.aslist()

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
