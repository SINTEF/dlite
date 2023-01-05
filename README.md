<!-- markdownlint-disable-next-line MD033 MD041 -->
<img src="doc/figs/logo.svg" align="right"/>

DLite
=====

> A lightweight data-centric framework for semantic interoperability

[![PyPi](https://img.shields.io/pypi/v/dlite-python.svg)](https://pypi.org/project/DLite-Python/)
[![CI tests](https://github.com/sintef/dlite/workflows/CI%20tests/badge.svg)](https://github.com/SINTEF/dlite/actions)
[![Documentation](https://img.shields.io/badge/documentation-informational?logo=githubpages)](https://sintef.github.io/dlite/index.html)

Content
-------

* ~About DLite~
  * [Example](#example)
  * ~Main features~
* ~Installing DLite~
  * ~Installing with pip~
  * ~Docker image~
  * ~Compile from sources~
    * ~Dependencies~
      * ~Runtime dependencies~
      * ~Build dependencies~
    * ~Build and install with Python~
    * ~Build on Linux~
    * ~Build with VS Code on Windows~
      * ~Quick start with VS Code and Remote Container~
    * ~Build documentation~
  * ~Setting up the environment~
* [Short vocabulary](#short-vocabulary)
* [Developer documentation](#developer-documentation)
* [License](#license)
* [Acknowledgment](#acknowledgment)

Example
-------

Let's say that you have the following Python class

```python
class Person:
    def __init__(self, name, age, skills):
        self.name = name
        self.age = age
        self.skills = skills
```

that you want to describe semantically.  We do that by defining the
following metadata (using json) identifying the Python attributes with
DLite properties.  Here we define `name` to be a string, `age` to be a
float and `skills` to be an array of `N` strings, where `N` is a name
of a dimension.  The metadata uniquely identifies itself with the
"name", "version" and "namespace" fields and "meta" refers the the
metadata schema (meta-metadata) that this metadata is described by.
Finally are human description of the metadata itself, its dimensions
and its properties provide in the "description" fields.

```json
{
  "name": "Person",
  "version": "0.1",
  "namespace": "http://onto-ns.com/meta",
  "meta": "http://onto-ns.com/meta/0.3/EntitySchema",
  "description": "A person.",
  "dimensions": [
    {
      "name": "N",
      "description": "Number of skills."
    }
  ],
  "properties": [
    {
      "name": "name",
      "type": "string",
      "description": "Full name."
    },
    {
      "name": "age",
      "type": "float",
      "unit": "year",
      "description": "Age of person."
    },
    {
      "name": "skills",
      "type": "string",
      "dims": ["N"],
      "description": "List of skills."
    }
  ]
}
```

We save the metadata in file "Person.json".  Back in Python we can now
make a DLite-aware subclass of `Person`, instantiate it and serialise
it to a storage:

```python
import dlite

# Create a DLite-aware subclass of Person
DLitePerson = dlite.classfactory(Person, url='json://Person.json')

# Instantiate
person = DLitePerson('Sherlock Holmes', 34., ['observing', 'chemistry',
    'violin', 'boxing'])

# Write to storage (here a json file)
person.dlite_inst.save('json://homes.json?mode=w')
```

To access this new instance from C, you can first generate a header
file from the meta data

```shell
dlite-codegen -f c-header -o person.h Person.json
```

and then include it in your C program:

```c
// homes.c -- sample program that loads instance from homes.json and prints it
#include <stdio.h>
#include <dlite.h>
#include "person.h"  // header generated with dlite-codegen

int main()
{
  /* URL of instance to load using the json driver.  The storage is
     here the file 'homes.json' and the instance we want to load in
     this file is identified with the UUID following the hash (#)
     sign. */
  char *url = "json://homes.json#315088f2-6ebd-4c53-b825-7a6ae5c9659b";

  Person *person = (Person *)dlite_instance_load_url(url);

  int i;
  printf("name:  %s\n", person->name);
  printf("age:   %g\n", person->age);
  printf("skills:\n");
  for (i=0; i<person->N; i++)
    printf("  - %s\n", person->skills[i]);

  return 0;
}
```

Now run the python file and it would create a homes.json file, which contains an entity information.
Use the UUID of the entity from the homes.json file, and update the url variable in the homes.c file.

Since we are using `dlite_instance_load_url()` to load the instance,
you must link to DLite when compiling this program.  Assuming you are
using Linux and DLite in installed in `$HOME/.local`, compiling with
gcc would look like:

```shell
gcc homes.c -o homes -I$HOME/.local/include/dlite -L$HOME/.local/lib -ldlite -ldlite-utils
```

Or if you are using the development environment , you can compile using:

```shell
gcc -I/tmp/dlite-install/include/dlite -L/tmp/dlite-install/lib -o homes homes.c -ldlite -ldlite-utils
```

Finally you can run the program with

```console
$ DLITE_STORAGES=*.json ./homes
name:  Sherlock Holmes
age:   34
skills:
  - observing
  - chemistry
  - violin
  - boxing
```

Note that we in this case have to define the environment variable
`DLITE_STORAGES` in order to let DLite find the metadata we stored in
'Person.json'.  There are ways to avoid this, e.g. by hardcoding the
metadata in C using `dlite-codegen -f c-source` or in the C program
explicitely load 'Person.json' before 'homes.json'.

This was just a brief example.  There is much more to DLite.  Since
the documentation is still not complete, the best source is the code
itself, including the tests and examples.

Short vocabulary
================

The following terms have a special meaning in DLite:

* **Basic metadata schema**: Toplevel meta-metadata which describes itself.
* **Collection**: A specialised instance that contains references to set
  of instances and relations between them.  Within a collection instances
  are labeled.  See also the [SOFT5 nomenclauture].
* **Data instance**: A "leaf" instance that is not metadata.
* **Entity**: May be any kind of instance, including data instances,
  metadata instances or meta-metadata instances.  However, for historical
  reasons it is often used for "standard" metadata that are instances of
  meta-metadata "http://onto-ns.com/meta/0.3/EntitySchema".
* **Instance**: The basic data object in DLite.  All instances are described
  by their metadata which itself are instances.  Instances are identified
  by an UUID.
* **Mapping**: A function that maps one or more input instances to an
  output instance.  They are an important mechanism for interoperability.
  Mappings are called translators in SOFT5.
* **Metadata**: a special type of instances that describe other instances.
  All metadata are immutable and has an unique URI in addition to their
  UUID.
* **Meta-metadata**: metadata that describes metadata.
* **Relation**: A subject-predicate-object triplet. Relations
  are immutable.
* **Storage**: A generic handle encapsulating actual storage backends.
* **Transaction**: An instance that has a reference to an immutable
  (frozen) parent instance is called a *transaction*.  Transactions are
  very useful for ensuring data provenance and makes it easy to work
  with time series.  Conceptually, they share many similarities with
  git.  See also the [SOFT5 nomenclauture].
* **uri**: A [uniform resource identifier (URI)][URI] is a
  generalisation of URL, but follows the same syntax rules.  In
  DLite, the term "uri" is used as an human readable identifier for
  instances (optional for data instances) and has the form
  `namespace/version/name`.
* **url**: A [uniform resource locator (URL)][URL] is an reference
  to a web resource, like a file (on a given computer), database
  entry, web page, etc.  In DLite url's refer to a storage or even
  an specific instance in a storage using the general syntax
  `driver://location?options#fragment`, where `options` and `fragment`
  are optional.  If `fragment` is provided, it should be the uuid or
  uri of an instance.
* **uuid**: A [universal unique identifier (UUID)][UUID] is commonly
  used to uniquely identify digital information.  DLite uses the 36
  character string representation of uuid's to uniquely identify
  instances.  The uuid is generated from the uri for instances that
  has an uri, otherwise it is randomly generated.

Developer documentation
=======================

* [Create a new release](doc/developers/release_instructions.md)

License
=======

DLite is licensed under the [MIT license](LICENSE).  However, it
include a few third party source files with other permissive licenses.
All of these should allow dynamic and static linking against open and
propritary codes.  A full list of included licenses can be found in
[LICENSES.txt](src/utils/LICENSES.txt).

Acknowledgment
==============

In addition from internal funding from SINTEF and NTNU this work has
been supported by several projects, including:

* [AMPERE](https://www.sintef.no/en/projects/2015/ampere-aluminium-alloys-with-mechanical-properties-and-electrical-conductivity-at-elevated-temperatures/) (2015-2020) funded by Forskningsrådet and Norwegian industry partners.
* FICAL (2015-2020) funded by Forskningsrådet and Norwegian industry partners.
* [Rational alloy design (ALLDESIGN)](https://www.ntnu.edu/digital-transformation/alldesign) (2018-2022) NTNU internally funded project.
* [SFI Manufacturing](https://www.sfimanufacturing.no/) (2015-2023) funded by Forskningsrådet and Norwegian industry partners.
* [SFI PhysMet](https://www.ntnu.edu/physmet) (2020-2028) funded by Forskningsrådet and Norwegian industry partners.
* [OntoTrans](https://cordis.europa.eu/project/id/862136) (2020-2024) that receives funding from the European Union’s Horizon 2020 Research and Innovation Programme, under Grant Agreement n. 862136.
* [OpenModel](https://www.open-model.eu/) (2021-2025) that receives funding from the European Union’s Horizon 2020 Research and Innovation Programme, under Grant Agreement n. 953167.
* [DOME 4.0](https://dome40.eu/) (2021-2025) that receives funding from the European Union’s Horizon 2020 Research and Innovation Programme, under Grant Agreement n. 953163.
* [VIPCOAT](https://www.vipcoat.eu/) (2021-2025) that receives funding from the European Union’s Horizon 2020 Research and Innovation Programme, under Grant Agreement n. 952903.

DLite is developed with the hope that it will be a delight to work with.

[SOFT5 nomenclauture]: https://confluence.code.sintef.no/display/SOFT/Nomenclature
[UUID]: https://en.wikipedia.org/wiki/Universally_unique_identifier
[URL]: https://en.wikipedia.org/wiki/URL
[URI]: https://en.wikipedia.org/wiki/Uniform_Resource_Identifier
<!-- [IRI]: https://en.wikipedia.org/wiki/Internationalized_Resource_Identifier -->
