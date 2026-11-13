Representing a datamodel
========================

The underlying structure of DLite datamodels are described under [concepts].

Here, at set of rules on how to create a datamodel is presented.

Note that several other possibilities are avilable, and this can be seen in the
examples and tests present in the repository.

We choose here to present only one method as mixing reprentation methods might
be confusing. Note, however that yaml and json representations are interchangeable.


Generic example
---------------
A generic example with some comments for clarity can be seen below.
For brevity, we choose here to represent the datamodel in [YAML] format.
However, the structure is exactly the same for the [JSON] format.

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


Defining multiple datamodels in a table
---------------------------------------
The `dlite.table` Python module provides a simple tabular interface to define multiple datamodels. The table can either be an Excel or CSV file or an Python table (like a NumPy array or just a sequence of sequences).

An example of such a table is shown below.

| @id                                 | description        | title       | datumName[1] | datumType[1] | datumUnit[1] | datumMapping[1] | datumName[2] | datumType[2] | datumShape[2] |
|-------------------------------------|--------------------|-------------|--------------|--------------|--------------|-----------------|--------------|--------------|---------------|
| http://onto-ns.com/meta/test/0.1/m1 | First data model.  | Datamodel 1 | length       | float64      | cm           | emmo:Length     |              |              |               |
| http://onto-ns.com/meta/test/0.1/m2 | Second data model. | Datamodel 2 | key          | string       |              |                 | indices      | int          | N,M           |

To illustrate that table may contain additional columns, not used for the generation of datamodels, a column 'title' has been added to the above example.

The table defines the following two datamodels:

```yaml
uri: http://onto-ns.com/meta/test/0.1/m1
description: First data model.
properties:
  length:
    type: float64
    unit: cm

uri: http://onto-ns.com/meta/test/0.1/m2
description: Second data model.
dimensions:
  N: N dimension
  M: M dimension
properties:
  key:
    type: string
  indices:
    type: int64
    shape: ["N", "M"]
```

The fields in the datamodel are mapped to the table headers via the following default mappings.

Note that `dimensions` is inferred from the shapes of the properties.

```python
# Default mappings of DLite metadata fields to table header names
>>> DEFAULT_DATAMODEL_MAPPINGS = {
...     "uri": "@id",
...     "dimensions": None,
...     "description": "description",
... }
# Default mappings of DLite property fields to table header names
>>> DEFAULT_PROPERTY_MAPPINGS = {
...     "name": "datumName",
...     "type": "datumType",
...     "ref": "datumRef",
...     "unit": "datumUnit",
...     "shape": "datumShape",
...     "description": "datumDescription",
... }

```

> [!NOTE]
> The that a square bracket (`[...]`) is appended to the property header labels.
> The content of these brackets have (currently) no semantic meaning, but are used to indicate what columns that belong to the same property.

Assuming that the above table is stored in a CSV file called `datamodels.csv`.
Python objects `m1` and `m2` for these datamodels can then be created with the following code

```python
>>> from dlite.table import DMTable
>>> t2 = DMTable.from_csv(indir / "datamodels.csv")
>>> m1, m2 = t2.get_datamodels()

```

The optional `datamodel_mappings` and `property_mappings` arguments of `DMTable.from_csv()` allow the user to provide custom mappings for the datamodel (`uri`, `description`) and property (`name`, `type`, `ref`, `unit`, `shape`, `description`) fields.

It is also possible to to give just the name in the `@id` or `uri` column.
However, this requires that the `baseuri` argument is provided.
The above table could then be rewritten as follows:

| @id | description        | title       | datumName[1] | datumType[1] | datumUnit[1] | datumMapping[1] | datumName[2] | datumType[2] | datumShape[2] |
|-----|--------------------|-------------|--------------|--------------|--------------|-----------------|--------------|--------------|---------------|
| m1  | First data model.  | Datamodel 1 | length       | float64      | cm           | emmo:Length     |              |              |               |
| m2  | Second data model. | Datamodel 2 | key          | string       |              |                 | indices      | int          | N,M           |

and the python code to create the same datamodels as above would be:

```python
>>> from dlite.table import DMTable
>>> t2 = DMTable.from_csv(indir / "datamodels.csv", baseuri="http://onto-ns.com/meta/test/0.1")
>>> m1, m2 = t2.get_datamodels()

```

Note that if the baseuri is not given and the `uri` is not a true IRI, an error will be raised.


### Storing the datamodels to a triplestore
Continuing with example above, the DMTable class has a `to_triplestore()` method that can be used to store the datamodels to a triplestore using the [tripper] package.

For example will the following

```python
>>> from tripper import Triplestore
>>> ts = Triplestore("rdflib")
>>> t2.to_triplestore(ts)

```

store all the datamodels in `t2` to the triplestore, representing them as proper EMMO datasets.



The soft5 and soft7 formats
---------------------------
For historical reasons are there two formats for the YAML and JSON representations.
The examples above are represented in the `soft7` format.
This format works well with pydantic models and is slightly less verbose than the older `soft5` format, which works well with JSON-LD and JSON-SCHEMA.

The above `http://onto-ns.com/meta/0.1/Person` datamodel has the following representation in the `soft5` format:

```yaml
uri: http://onto-ns.com/meta/0.1/Person
description: A person.
dimensions:
  - name: N
    description: Number of skills.
properties:
  - name: name
    description: Full name.
    type: string
  - name: age
    description: Age of person.
    type: float
    unit: years
  - name: skills
    description: List of skills.
    type: string
    shape: [N]
```




Validating data models
----------------------
The [dlite-validate] tool can be used to check if a specific representation (in a file) is a valid DLite datamodel


[concepts]: https://sintef.github.io/dlite/user_guide/concepts.html
[JSON]: https://www.json.org/
[YAML]: https://yaml.org/
[dlite-validate]: https://sintef.github.io/dlite/user_guide/tools.html#dlite-validate
[tripper]: https://emmc-asbl.github.io/tripper/
