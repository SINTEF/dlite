Example package providing extra DLite datamodels and plugins
============================================================
This example shows how one can create a Python package that provides
new DLite datamodels and plugins and how these resources can be
accessed as entry points.

Our example package is here called `mypackage` and contains a
datamodel, a plugin and a dataset for temperature profiles.

The package layout is a follows.

```
.
├── mypackage
│   ├── __init__.py
│   ├── data
│   │   └── dataset.txt
│   ├── datamodels
│   │   └── TempProfile.json
│   └── plugins
│       └── tempprofile.py
├── pyproject.toml
├── MANIFEST.in
└── README.md
```

Create a virtual environment before running the `main.py` script.
It will install `mypackage` and run the `anotherpackage/main.py` script,
which will utilise the resources made available in `mypackage`.

See the documentation on [using-entry-points] for background.


[using-entry-points]: https://sintef.github.io/dlite/user_guide/search_paths.html#using-entry-points
