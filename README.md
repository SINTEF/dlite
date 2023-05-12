<img src="doc/_static/logo.svg" align="right" />


DLite
=====
> A lightweight data-centric framework for semantic interoperability

[![PyPi](https://img.shields.io/pypi/v/dlite-python.svg)](https://pypi.org/project/DLite-Python/)
[![CI tests](https://github.com/sintef/dlite/workflows/CI%20tests/badge.svg)](https://github.com/SINTEF/dlite/actions)
[![Documentation](https://img.shields.io/badge/documentation-informational?logo=githubpages)](https://sintef.github.io/dlite/index.html)


DLite is a C implementation of the [SINTEF Open Framework and Tools
(SOFT)][SOFT], which is a set of concepts and tools for using data
models (aka Metadata) to efficiently describe and work with scientific
data.

![DLite overview](doc/_static/overview.svg)

The core of DLite is a framework for formalised representation of data
described by data models (called Metadata or Entity in DLite).
On top of this, DLite has a plugin system for various representations of
the data in different formats and storages, as well as bindings to popular
languages like Python, mappings to ontological concepts for enhanced
semantics and a set of tools.


Documentation
-------------
The official documentation for DLite can be found on https://sintef.github.io/dlite/.


Installation
------------
DLite is available on PyPI and can be installed with pip

```shell
pip install dlite-python
```

For alternative installation methods, see the [installation instructions].


Usage
-----
All data in DLite is represented by a instance, which is described by
a simple data model (aka Metadata).  An Instance is identified by a
unique UUID and have a set of named dimensions and properties.  The
dimensions are used to describe the shape of multi-dimensional
properties.

DLite Metadata are identified by an URI and have an (optional) human
readable description.  Each dimension is given a name and description
(optional) and each property is given a name, type, shape (optional),
unit (optional) and description (optional).  The shape of a property
refers to the named dimensions.  Foe example, a Metadata for a person
serialised in YAML may look like:

```yaml
uri: http://onto-ns.com/meta/0.1/Person
description: A person.
dimensions:
  nskills: Number of skills.
properties:
  name:
    type: string
    description: Full name.
  age:
    type: float32
    unit: year
    description: Age of person.
  skills:
    type: string
    shape: [nskills]
    description: List of skills.
```

Assume that you have file `Person.yaml` with this content.
In Python, you can load this Metadata with

```python
import dlite
Person = dlite.Instance.from_location("yaml", "Person.yaml", options="mode=r")
```

where the first argument is the "driver", i.e. the name of storage
plugin to use for loading the Metadata.  The `options` argument is
optional.  By providing `"mode=r"` you specify that the storage is
opened in read-only mode.

You can verify that Person is a Metadata

```python
>>> isinstance(Person, dlite.Metadata)
True
```

We can create an instance of `Person` with

```python
homes = Person(
    dimensions={"nskills": 4},
    properties={
      "name": "Sherlock Homes",
      "skills": ["observing", "chemistry", "violin", "boxing"],
    }
)
```

The `dimensions` argument must be supplied when a Metadata is
instantiated.  It ensures that the shape of all properties are
initialised consistently.  The `properties` argument is optional.
By specifying it, we initialise the properties to the provided values
(otherwise, they will be initialised to zero).

In this case we didn't initialised the age
```python
>>> homes.age
0.0
>>> homes.age = 34  # Assign the age
```

We can view (a JSON representation of) the instance with

```python
>>> print(homes)
{
  "uuid": "314ac1ad-4a7e-477b-a56c-939121355112",
  "meta": "http://onto-ns.com/meta/0.1/Person",
  "dimensions": {
    "nskills": 4
  },
  "properties": {
    "name": "Sherlock Homes",
    "age": 34.0,
    "skills": [
      "observing",
      "chemistry",
      "violin",
      "boxing"
    ]
  }
}
```

The instance can also be stored using the `save()` method

```python
homes.save("yaml", "homes.yaml", "mode=w")
```

which will produce the a YAML file with the following content

```yaml
8cbd4c09-734d-4532-b35a-1e0dd5c3e8b5:
  meta: http://onto-ns.com/meta/0.1/Person
  dimensions:
    nskills: 4
  properties:
    name: Sherlock Homes
    age: 34.0
    skills:
    - observing
    - chemistry
    - violin
    - boxind
```

This was just a brief example.
There is much more to DLite as will be revealed in the [documentation].


License
-------
DLite is licensed under the [MIT license](LICENSE).  However, it
include a few third party source files with other permissive licenses.
All of these should allow dynamic and static linking against open and
propritary codes.  A full list of included licenses can be found in
[LICENSES.txt](src/utils/LICENSES.txt).


Acknowledgment
--------------
In addition from internal funding from SINTEF and NTNU this work has
been supported by several projects, including:

  - [AMPERE](https://www.sintef.no/en/projects/2015/ampere-aluminium-alloys-with-mechanical-properties-and-electrical-conductivity-at-elevated-temperatures/) (2015-2020) funded by Forskningsrådet and Norwegian industry partners.
  - FICAL (2015-2020) funded by Forskningsrådet and Norwegian industry partners.
  - [Rational alloy design (ALLDESIGN)](https://www.ntnu.edu/digital-transformation/alldesign) (2018-2022) NTNU internally funded project.
  - [SFI Manufacturing](https://www.sfimanufacturing.no/) (2015-2023) funded by Forskningsrådet and Norwegian industry partners.
  - [SFI PhysMet](https://www.ntnu.edu/physmet) (2020-2028) funded by Forskningsrådet and Norwegian industry partners.
  - [OntoTrans](https://cordis.europa.eu/project/id/862136) (2020-2024) that receives funding from the European Union’s Horizon 2020 Research and Innovation Programme, under Grant Agreement n. 862136.
  - [OpenModel](https://www.open-model.eu/) (2021-2025) that receives funding from the European Union’s Horizon 2020 Research and Innovation Programme, under Grant Agreement n. 953167.
  - [DOME 4.0](https://dome40.eu/) (2021-2025) that receives funding from the European Union’s Horizon 2020 Research and Innovation Programme, under Grant Agreement n. 953163.
  - [VIPCOAT](https://www.vipcoat.eu/) (2021-2025) that receives funding from the European Union’s Horizon 2020 Research and Innovation Programme, under Grant Agreement n. 952903.


---

DLite is developed with the hope that it will be a delight to work with.

[installation instructions]: https://sintef.github.io/dlite/getting_started/installation.html
[documentation]: https://sintef.github.io/dlite/index.html
[SOFT]: https://www.sintef.no/en/publications/publication/1553408/
[UUID]: https://en.wikipedia.org/wiki/Universally_unique_identifier
