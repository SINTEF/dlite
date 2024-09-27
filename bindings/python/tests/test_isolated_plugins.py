"""Check that the plugins are not touching the main namespace"""
import dlite
from dlite.testutils import importskip

# Import yaml
yaml = importskip("yaml")


# At this point no plugins have been loaded
assert not dlite.DLiteStorageBase.__subclasses__()

# yaml is the module we imported above (PyYAML)
assert yaml.__package__ == "yaml"

# Load all plugins
dlite.Storage.load_plugins()

# Now the yaml plugin is loaded
assert "yaml" in set(m.__name__ for m in dlite.DLiteStorageBase.__subclasses__())

# yaml is still the module we imported above (PyYAML)
assert yaml.__package__ == "yaml"
