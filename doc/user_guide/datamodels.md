Representing a datamodel (entity)
----------------------------------

The underlying structure of DLite datamodels are described under [concepts].

Here, at set of rules on how to create a datamodel is presented.

Note that several other possibilities are avilable, and this can be seen in the
examples and tests present in the repository. 

We choose here to present only one method as mixing repsentation methods might 
be confusing. Note, however that yaml and json representations are interchangable.

A generic example with some comments for clarity can be seen below.

```yaml
uri: http://namespace/version/name
description: A description of what this datamodel represents.
dimensions: # Simplest to represent as a dict, set to {} if no dimensions
  name_of_dimension: description of dimension
properties:
  name_of_property1:
    description: What is this property
    type: ref # Can be any on string, float, double, int, ref ....
    unit: unit # can be ommitted, not relevant with type ref
    shape: [name_of_dimension] # ommit if shape is 1
    $ref:  http://namespace/version/name_of_referenceddatamodel # only if type is ref
```

A slightly more realistic example is the "Person" entity, where we want to describe his/her name, age and skills.

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

First we have "uri" identifying the entity, and a human description.
Then comes "dimensions".In this case one dimension named "N", which is the number of skills the person has.
Finally we have the properties; "name", "age" and "skills".
We see that "name" is represented as a string, "age" as a floating point number with unit years and "skills" as an array of strings, one for each skill.


dlite-validate
==============
The dlite-validate tool can be used to check if a specific representation (in a file) is a valid DLite datamodel

This can be run as follows
```bash
dlite-validate filename.yaml # or json
```

It will then return a list of errors if it is not a valid representation.
