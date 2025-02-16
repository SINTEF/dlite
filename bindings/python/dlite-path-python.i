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
import sys
from pathlib import Path
from importlib import import_module
from importlib.metadata import entry_points

def _create_path(name):
    """Return new DLite search path object, with given name."""

    # Create FUPath object
    path = FUPath(name)
    path.name = name

    # Access "dlite.paths" entry points
    if sys.version_info < (3, 10):  # Fallback for Python < 3.10
        eps = entry_points().get(f"dlite.paths", ())
    else:  # For Python 3.10+
        eps = entry_points(group=f"dlite.paths")

    # Use entry points to populate default paths (should work from Python 3.8)
    for ep in eps:
        if ep.name != name:
            continue
        for value in ep.value.split("|"):
            module, filepath = value.split(":", 1)
            fullpath = Path(import_module(module).__file__).parent / filepath
            path.append(str(fullpath))

    return path

# Create DLite search paths objects
storage_path = _create_path("storage_path")
template_path = _create_path("template_path")
storage_plugin_path = _create_path("storage_plugin_path")
mapping_plugin_path = _create_path("mapping_plugin_path")
python_storage_plugin_path = _create_path("python_storage_plugin_path")
python_mapping_plugin_path = _create_path("python_mapping_plugin_path")
python_protocol_plugin_path = _create_path("python_protocol_plugin_path")

%}
