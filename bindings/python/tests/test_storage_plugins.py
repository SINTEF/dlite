import dlite

# Storage plugins can be loaded
dlite.Storage.load_plugins()

# Now plugins are loaded
plugins = set(dlite.StoragePluginIter())
assert plugins
assert "json" in plugins

# Plugins can be iterated over
print("Storage plugins")
plugins2 = set()
for name in dlite.StoragePluginIter():
    print("  -", name)
    plugins2.add(name)
assert plugins2 == plugins

# Unload json plugin
dlite.Storage.unload_plugin("json")
assert "json" not in set(dlite.StoragePluginIter())
