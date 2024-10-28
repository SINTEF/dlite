Representing a datamodel (entity)
----------------------------------

The underlying structure of DLite datamodels are described under [concepts].

Here, at set of rules on how to create a datamodel is presented.

Note that several other possibilities are avilable, and this can be seen in the
examples and tests present in the repository. 

We choose here to present only one method as mixing reprentation methods might 
be confusing. Note, however that yaml and json representations are interchangable.

A generic example with some comments for clarity can be seen below.

```yaml
uri: http://namespace/version/name
description: A description of what this datamodel represents.
dimensions:  # Named dimensions referred to in the property shapes. Simplest to represent it as a dict, set to {} if there are no dimensions
  name_of_dimension: description of dimension
properties:
  name_of_property1:
    description: What is this property
    type: ref # Can be any on string, float, double, int, ref ....
    unit: unit  # Can be ommitted if the property has no unit
    shape: [name_of_dimension]  # Can be omitted if the property is a scalar
    $ref:  http://namespace/version/name_of_referenceddatamodel # only if type is ref
```

The keywords in the datamodel have the following meaning:
* `uri`: A URI that uniquely identifies the datamodel.
* `description`: A human description that describes what this datamodel represents.
* `dimensions`: Dimensions of the properties (referred to by the property shape). Properties can have the same dimensions, but not necessarily. Each dimension is described by:
  - name of the dimension 
  - a human description of the dimension 
  In the below example there is one dimension with name "N" and description "Number of skills."
* `properties`: Sub-parts of the datamodel that describe the individual data fields. A property has a name and is further specified by the following keywords:
  - `description`: Human description of the property.
  - `type`: Data type of the property. Ex: "blob5", "boolean", "uint", "int32", "string", "string10", "ref", ...
  - `$ref`: Optional. URI of a sub-datamodel. Only used if type is "ref".
  - `unit`: Optional. The unit. Ex: "kg", "km/h", ... Can be omitted if the property has no unit.
  - `shape`: Optional. Describes the dimensionality of the property as a list of dimension names. Ex: `[N]`. Can be omitted if the property has no shape, i.e. the instance always has only one value. This is equivalent to a 0-dimensional array, i.e. shape=[].
  The datamodel below has three properties; "name", "age" and "skills". We see that "name" is represented as a string, "age" as a floating point number with unit years and "skills" as an array of strings, one for each skill.


A slightly more realistic example is the "Person" entity, where we want to describe his/her name, age and skills:

```yaml
uri: http://onto-ns.com/meta/0.1/Person
description: A person.
dimensions: 
  N: Number of skills.
properties:
  name:
    description: Full name.
    type: string
  age:
    description: Age of person.
    type: float
    unit: years
  skills:
    description: List of skills.
    type: string
    shape: [N]
```


dlite-validate
==============
The [dlite-validate tool][./tools.md#dlite_validate] can be used to check if a specific representation (in a file) is a valid DLite datamodel


[concepts]: https://sintef.github.io/dlite/user_guide/concepts.html
