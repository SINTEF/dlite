# More advanced example

Let's say that you have the following Python class:

```python
class Person:
    """A person.

    Arguments:
        name: Full name.
        age: Age of person.
        skills: List of skills.

    """

    def __init__(self, name: str, age: float, skills: list[str]) -> None:
        self.name = name
        self.age = age
        self.skills = skills
```

You want to describe this class semantically.
This is done by defining the following metadata (using JSON) identifying the Python attributes with DLite properties.
Here we define `name` to be a string, `age` to be a float and `skills` to be an array of `N` strings, where `N` is a name of a dimension.
The metadata uniquely identifies itself with the "name", "version" and "namespace" fields and "meta" refers to the metadata schema (meta-metadata) that this metadata is described by.
Finally, human descriptions of the metadata itself, its dimensions and its properties are provided in the "description" fields.

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
      "shape": ["N"],
      "description": "List of skills."
    }
  ]
}
```

The metadata can be saved in file ``Person.json``.
In Python we can now make a DLite-aware subclass of `Person`, instantiate it, and serialise it to a storage:

```python
import dlite

# Create a DLite-aware subclass of Person
DLitePerson = dlite.classfactory("Person", url="json://Person.json")

# Instantiate
person = DLitePerson(
    "Sherlock Holmes",
    34.,
    ["observing", "chemistry", "violin", "boxing"],
)

# Write to storage (here a JSON file)
person.dlite_inst.save('json://homes.json?mode=w')
```

To access this new instance from C, you can first generate a header file from the meta data

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

Now run the Python file and it will create a ``homes.json`` file, which contains a serialized DLite entity.
Use the UUID of the entity from the ``homes.json`` file and update the ``url`` variable in the ``homes.c`` file.

Since we are using `dlite_instance_load_url()` to load the instance, you must link to DLite when compiling this program.
Assuming you are using Linux and DLite is installed in `$HOME/.local`, compiling with gcc would look like:

```shell
gcc homes.c -o homes -I$HOME/.local/include/dlite -L$HOME/.local/lib -ldlite -ldlite-utils
```

Or if you are using the development environment, you can compile using:

```shell
gcc -I/tmp/dlite-install/include/dlite -L/tmp/dlite-install/lib -o homes homes.c -ldlite -ldlite-utils
```

Finally, you can run the program with

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

Note that in this case it is necessary to define the environment variable `DLITE_STORAGES` in order to let DLite find the metadata stored in ``Person.json``.
There are ways to avoid this, e.g., by hardcoding the metadata in C using `dlite-codegen -f c-source` or in the C program explicitely load ``Person.json`` before ``homes.json``.

This was just a brief example.
There is much more to DLite as will be revealed in the following parts of the tutorial.
