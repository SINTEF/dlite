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
storage_path = FUPath("storage")
mapping_path = FUPath("mapping")
python_storage_path = FUPath("python-storage")
python_mapping_path = FUPath("python-mapping")

%}
