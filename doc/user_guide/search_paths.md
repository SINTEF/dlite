Search paths
============
It is possible to extend DLite with new datamodels, code generation templates and plugins, by appending to corresponding search path.  The table below lists the different types of search paths that are available in DLite.

| Search path type        | Description                                                    |
|-------------------------|----------------------------------------------------------------|
| storages                | Storage URLs or directory paths to datamodels                  |
| templates               | Directory paths to code generator templates                    |
| storage_plugins         | Directory paths to storage plugins (drivers) written in C      |
| mapping-plugins         | Directory paths to mapping plugins written in C                |
| python-storage-plugins  | Directory paths to storage plugins (drivers) written in Python |
| python-mapping-plugins  | Directory paths to mapping plugins written in Python           |
| python-protocol-plugins | Directory paths to protocol plugins written in Python          |

Search paths can be extended in different three ways:
* [setting environment variables]
* [appending to DLite path variables] (from Python)
* [using entry points] (only in user-defined Python packages)

The table below lists the lists the name of the environment variables and Python path variables corresponding to the different types of search paths.

| Search path type        | Environment variable name         | Python variable name              |
|-------------------------|-----------------------------------|-----------------------------------|
| storages                | DLITE_STORAGES                    | dlite.storage_path                |
| templates               | DLITE_TEMPLATE_DIRS               | dlite.template_path               |
| storage_plugins         | DLITE_STORAGE_PLUGIN_DIRS         | dlite.storage_plugin_path         |
| mapping-plugins         | DLITE_MAPPING_PLUGIN_DIRS         | dlite.mapping-plugin_path         |
| python-storage-plugins  | DLITE_PYTHON_STORAGE_PLUGIN_DIRS  | dlite.python-storage-plugin_path  |
| python-mapping-plugins  | DLITE_PYTHON_MAPPING_PLUGIN_DIRS  | dlite.python-mapping-plugin_path  |
| python-protocol-plugins | DLITE_PYTHON_PROTOCOL_PLUGIN_DIRS | dlite.python-protocol-plugin_path |


Setting environment variables
-----------------------------
This is typically done where you set up your environment, like in a virtualenv activate script or the users `~/.bashrc` file.

:::{note}
All the path variables, except for `DLITE_STORAGES`, uses (`:`) colon as path separator.
However, since colon may appears in URLs, `DLITE_STORAGES` uses instead the pipe symbol (`|`) as path separator.
:::

See [environment variables] for more details.


Appending to DLite path variables
---------------------------------
The `dlite` Python module defines the path variables listed in the table above.
A Python script or application can configure new datamodels and plugins by appending to these variables.

:::{example}
Adding the sub-directory `datamodels` to the search path for datamodels, can be done with:

```python
>>> from pathlib import Path
>>> import dlite
>>> dlite.storage_path.append(Path("my_package_root") / "datamodels")
```
:::


Using entry points
------------------
Entry points for DLite search paths uses the following conventions:

* The **group** for all DLite search path entry points is `dlite.paths`.
* The **name** of the entry points in the `dlite.paths` group is one of the search path variables (`storage_path`, `template_path`, `storage_plugin_path`, `mapping_plugin_path`,
`python_storage_plugin_path`, `python_mapping_plugin_path`, `python_protocol_plugin_path`).
* The **value** of the entry points in the `dlite.paths` group is a sequence of `module:path` pairs separated by "|" characters.

How to use [entry points] is easiest described with an example.
Let us assume you have a package with the following directory layout:

```
project_root
├── mypackage
│   ├── __init__.py
│   ├── mymodule.py
│   └── data
│       ├── python_storage_plugins
│       │   ├── myplugin.py
│       │   └── anotherplugin.py
│       ├── datamodels
│       │   ├── mydatamodel.yaml
│       │   └── anotherdatamodel.yaml
│       └── special_datamodels
│           └── specialdatamodel.yaml
├── pyproject.toml
├── README.md
└── LICENSE
```

To make your datamodels and Python storage plugins available for users of your package, you can add the following section to your `pyproject.toml` file:

```toml
[tool.setuptools.package-data]
"mypackages.data.datamodels" = ["*.json", "*.yaml"]
"mypackages.data.python_storage_plugins" = ["*.py"]

# Note the quotes around dlite.python_storage_plugins to escape the embedded dot
[project.entry-points."dlite.paths"]
storage_path = "mypackage:data/datamodels|mypackage:data/special_datamodels"
python_storage_plugin_path = "mypackage:data/python_storage_plugins"
```

See the [Setuptools documentation] for how to this can be done with `setup.py` or `setup.cfg`.



[setting environment variables]: #setting-environment-variables
[appending to DLite path variables]: #appending-to-dlite-path-variables
[using entry points]: #using-entry-points
[environment variables]: https://sintef.github.io/dlite/user_guide/environment_variables.html
[entry points]: https://setuptools.pypa.io/en/latest/userguide/entry_point.html
[Setuptools documentation]: https://setuptools.pypa.io/en/latest/userguide/index.html
