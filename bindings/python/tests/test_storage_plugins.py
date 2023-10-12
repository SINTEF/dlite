import textwrap

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

# Show documentation for all plugins
undoc = []
print()
print("Plugin documentation")
print("====================")
for name in dlite.StoragePluginIter():
    try:
        doc = dlite.Storage.plugin_help(name)
    except dlite.DLiteUnsupportedError:
        undoc.append(name)
    else:
        lst = doc.split("\n")
        for i, line in enumerate(lst):
            if line.strip() and line.startswith(" "):
                break
        print()
        print(name)
        print("-" * len(name))
        print("*** i:", i)
        print("\n".join(lst[:i]) + "\n" + textwrap.dedent("\n".join(lst[i:])))

print()
print("Undocumented plugins")
print("====================")
print("  - " + "\n  - ".join(undoc))



# Unload json plugin
dlite.Storage.unload_plugin("json")
assert "json" not in set(dlite.StoragePluginIter())
