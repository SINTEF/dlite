Installing DLite
================
DLite is written C, but can be compiled with or without bindings to [Python] and [Fortran] and with or without support for [HDF5] and [librdf] storages.
A set of additional optional storages are available if compiled with Python support.
For a complete list, see [runtime dependencies].


Installing with pip
-------------------
If you are using Python, the easiest way to install DLite is with pip:

```shell
pip install DLite-Python[full]
```

The bracket `[full]` is optional, but ensures that you install DLite
together with all optional dependencies needed for additional features
and storage plugins. (see also [runtime dependencies]).


Development installation
------------------------
If you to contribute or develop DLite, you should [build from source].

Python dependencies for development can be installed with

```shell
pip install DLite-Python[dev]
```

Install additional Python packages for building documentation with

```shell
pip install DLite-Python[doc]
```


[Python]: https://www.python.org/
[Fortran]: https://en.wikipedia.org/wiki/Fortran
[HDF5]: https://support.hdfgroup.org/HDF5/
[librdf]: https://librdf.org/
[runtime dependencies]: https://sintef.github.io/dlite/getting_started/build/runtime_dependencies.html
[build from source]: https://sintef.github.io/dlite/getting_started/build/build.html
