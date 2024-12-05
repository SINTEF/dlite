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


### Extra requirements
The complete list of extra requirements is as follows:

- [mappings]: Extra requirements needed for property mappings.
- [full]: All extra runtime requirements, needed by the various storage plugins or features like property mappings.
- [dev]: Extra requirements needed for development and testing.
- [doc]: Extra requirements needed for generating documentation.


### Supported binary releases
DLite uses [cibuildwheel] for providing binary releases for Linux and
Windows on various platforms and versions of Python.  The provided
wheels are shown in the following table.


| Platform         | Arch   | 3.8 | 3.9 | 3.10 | 3.11 | 3.12 | 3.13 |
|------------------|--------|-----|-----|------|------|------|------|
| [manylinux_2_34] | x86_64 | x   | x   | x    | x    | x    | x    |
| [manylinux2014]  | i686   |     |     | x    | x    | x    | x    |
| [musllinux_1_2]  | x86_64 | x   | x   | x    | x    | x    | x    |
| [musllinux_1_2]  | i686   |     | x   | x    | x    |      |      |
| windows          | x86_64 | x   | x   | x    | x    | x    | x    |
| windows          | i686   | x   | x   | x    |      |      |      |


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
[cibuildwheel]: https://github.com/pypa/cibuildwheel
[manylinux2014]: https://github.com/pypa/manylinux?tab=readme-ov-file#manylinux2014-centos-7-based
[manylinux_2_34]: https://github.com/pypa/manylinux?tab=readme-ov-file#manylinux_2_34-almalinux-9-base
[musllinux_1_2]: https://peps.python.org/pep-0656/
