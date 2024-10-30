Tutorial
========

Introduction
------------
This tutorial shows the steps for creating and instantiating a Dlite entity, and populate it with data.
For this, we will use an example `csv` file containing data corresponding to time measurements from a solar panel.
The example file has the following format:


| time                 | Impp [A] | Isc [A] | MPP [W] | Vmpp [V] | Voc [V] |
|----------------------|----------|---------|---------|----------|---------|
| 2022-08-01 00:41:00  |  1.188   |   1.25  |  38.182 |   32.14  |  37.33  |
|  ...                 |   ...    |   ...   |   ...   |    ...   |   ...   |



Writing a DLite Entity
----------------------
Let us write an Entity representing our solar panel measurement.
We start by creating a `json` file called `myEntity.json`.

### **Step 1**: Giving it a unique identifier
First, we must provide a unique identifier for our Entity.
There are several ways of doing this:

#### **1.  Uniform Resource Identifier (URI)**
We can create and assign a unique Uniform Resource Identifier (URI).
A URI has the form **namespace/version/nameOfEntity**, and it can be either written directly in the json file:

```json
"uri": "http://www.ontotrans.eu/0.1/solarPanelMeasurement",
```
or we can give the **namespace**, **version**, and **name** of our Entity as separate fields:

```json
"name": "solarPanelMeasurement",
"namespace": "http://www.ontotrans.eu/",
"version": "0.1"
```

#### **2. Universally Unique Identifier (UUID)**
Alternatively, we can generate and assign a universally unique identifier (UUID).
In Python, this can be achieved by the use of the `dlite.get_uuid()` function or the [dlite-getuuid] tool.

```python
 >>> import uuid

 # Generate a random UUID
 >>> entity_uuid = uuid.uuid4()

 >>> with open("myEntity.json", "w") as entity_file:
 ...     entity_file.write('"uuid": {}'.format(entity_uuid))

```

Our json file now contains the following:

```json
"uuid": "<uuid>"
```

where `<uuid>` is a unique identifier for our Entity.


### **Step 2**: Providing a link to the metadata
Let us assume that we have followed option 1 and provided a URI.
The next step consists in adding a **meta** field that will link the Entity to its metadata.
In DLite, all Entities are instances of the *EntitySchema*.
The *EntitySchema* defines the structure of the Entity.
In the **meta** field of our Entity, we need to provide the URI of the EntitySchema.
For a schematic overview of the DLite data structures and further detail, see [Concepts section of the DLite User Guide].
We add this information to our json file in the following manner:

```json
"uri": "http://www.ontotrans.eu/0.1/solarPanelMeasurement",
"meta": "http://onto-ns.com/meta/0.3/EntitySchema",
```

:::{note}
The **meta** field actually defaults to `http://onto-ns.com/meta/0.3/EntitySchema`. This means that the **meta** field is optional for entities.
:::

### **Step 3**: Adding a human-understandable description
Next, we want to include a human-understandable description of what the Entity represents.
In our example case, such a **description** field could be

```json
"uri": "http://www.ontotrans.eu/0.1/solarPanelMeasurement",
"meta": "http://onto-ns.com/meta/0.3/EntitySchema",
"description": "Measurement data from one solar panel."
```

### **Step 4**: Defining the dimensions
We continue by defining the dimensions for our Entity.
Each dimension should have a **name** and a human-understandable **description**.
In our example case, we only have one dimension since all the quantities we are interested in are measured with the same frequency (i.e., we have the same number of data points for all the measured quantities).
Note that even though we only have one dimension, we need to give it in a list format.

We will give our dimension the generic name "N", and describe it as the number of measurements:

```json
"uri": "http://www.ontotrans.eu/0.1/solarPanelMeasurement",
"meta": "http://onto-ns.com/meta/0.3/EntitySchema",
"description": "Measurement data from one solar panel.",
"dimensions": {
      "N": "Number of measurements."
}
```

### **Step 5**: Defining the properties
Now it is time to define the properties of our Entity.
Here is where we can give meaning to our data.
As for the dimensions, we add the properties as a list of `json` structures, each one having a **name**, **type**, **description**, and if relevant, **unit**, **shape** (the dimensionality of the property) and **ref** (the metadata for *ref* types).

Inserting the properties displayed in the table above, our Entity is complete and may look like

```json
{
    "uri": "http://www.ontotrans.eu/0.1/solarPanelMeasurement",
    "meta": "http://onto-ns.com/meta/0.3/EntitySchema",
    "description": "Measurement data from one solar panel.",
    "dimensions": {
        "N": "Number of measurements."
    },
    "properties": {
        "t": {
            "type":"str",
            "unit":"ns",
            "shape": ["N"],
            "description": "Time"
        },
        "MPP": {
            "type":"float64",
            "unit":"W",
            "shape": ["N"],
            "description": "Maximum Power"
        },
        "impp": {
            "type": "float64",
            "unit": "A",
            "shape": ["N"],
            "description": "Maximum power point current."
        },
        "isc": {
            "type": "float64",
            "unit": "A",
            "shape": ["N"],
            "description": "Short circuit current."
        },
        "vmpp": {
            "type": "float64",
            "unit": "V",
            "shape": ["N"],
            "description": "Maximum power point voltage."
        },
        "voc": {
            "type": "float64",
            "unit": "V",
            "shape": ["N"],
            "description": "Open circuit voltage."
        }
    }
}
```

:::{note}
Both dimensions and properties can also be provided as arrays using a `dict` with the name as key. This may look like

```json
{
    "dimensions": [
        {
            "name": "N",
            "description": "Number of measurements."
        }
    ],
    "properties": [
        {
            "name": "t",
            "type":"str",
            "unit":"ns",
            "shape": ["N"],
            "description": "Time"
        },
        {
            "name": "MPP",
            "type":"float64",
            "unit":"W",
            "shape": ["N"],
            "description": "Maximum Power"
        },
        {
            "name": "impp",
            "type": "float64",
            "unit": "A",
            "shape": ["N"],
            "description": "Maximum power point current."
        },
        {
            "name": "isc",
            "type": "float64",
            "unit": "A",
            "shape": ["N"],
            "description": "Short circuit current."
        },
        {
            "name": "vmpp",
            "type": "float64",
            "unit": "V",
            "shape": ["N"],
            "description": "Maximum power point voltage."
        },
        {
            "name": "voc",
            "type": "float64",
            "unit": "V",
            "shape": ["N"],
            "description": "Open circuit voltage."
        }
    ]
}
```

DLite supports both syntaxes.
:::


Loading an Entity with DLite
----------------------------
We will now load our Entity in Python. There are several ways of doing this.

### 1. Using the json storage plugin:

```python
    >>> import dlite

    >>> SolarPanelEntity = dlite.Instance.from_location('json', 'myEntity.json')

```

### 2. Adding the file to `storage_path`
In this method, we append the `json` file to `storage_path` and then use the [get_instance()] function to load the Entity using its URI.
In Python, this may be done by

``` python
    >>> import dlite

    # Append the entity file path to the search path for storages
    >>> dlite.storage_path.append('myEntity.json')

    # Load Entity using its uri
    >>> SolarPanelEntity = dlite.get_instance("http://www.ontotrans.eu/0.1/solarPanelMeasurement")

```

**Note:** The same approach applies if a UUID is used.


Connect loaded entity to data
-----------------------------
Now that we have loaded our Entity, we can instantiate it, and populate with the data from the `csv` file mentioned in the introduction.
Note that you may have to provide a full file path to the `read_csv()` function, depending on where you have stored your `csv` file.


```python
    >>> import pandas as pd

    # Read csv file using pandas, store its contents in a pandas DataFrame
    >>> solar_panel_dataframe = pd.read_csv('data.csv', sep=',', header = 0)

    # Instantiate an instance of the Entity that we loaded above
    >>> inst = SolarPanelEntity({"N": len(solar_panel_dataframe)})

    # Create a dicitionary that defines the mapping between property names and column headers
    >>> mapping = {
    ...     "t": "time",
    ...     "MPP": "MPP [W]",
    ...     "voc": "Voc [V]",
    ...     "vmpp": "Vmpp [V]",
    ...     "isc" : "Isc [A]",
    ...     "impp": "Impp [A]",
    ... }

    # Loop through the dictionary keys and populate the instance with data
    >>> for key in mapping:
    ...     inst[key] = solar_panel_dataframe[mapping[key]]

```


More examples of how to use DLite are available in the [examples] directory.


Working with physical quantities
--------------------------------
As you can read in the previous sections, a property of a dlite entity can be
defined with a **unit** (a unit of measurement).
The dlite.Instance object can set (or get) their properties from (or to) physical quanities (the product of a numerical value and a unit of measurement).
The dlite library uses the physical quantities defined and implemented by the Python package [pint].

```{note}
You must install pint if you want to to work with physical quantities,
try to run the command `pip install pint`, see the page [pint installation].
```

This section illustrates how to work with physical quantities when manipulating
the dlite **instance** objects.
The code example will use the following data model:

```yaml
uri: http://onto-ns.com/meta/0.1/BodyMassIndex
description: The body mass index of a person.
properties:
  name:
    type: string
    description: Name of the person.
  age:
    type: float32
    unit: year
    description: Age of the person.
  height:
    type: float
    unit: cm
    description: Height of the person.
  weight:
    type: float
    unit: kg
    description: Weight of the person.
  bmi:
    type: float
    unit: kg/m^2
    description: Body mass index.
  category:
    type: string
    description: Underweight, normal, or overweight.
```

The code below will create one BodyMassIndex instance object (variable `person`) and assign the properties of the `person` using physical quantities.
You can use the method `person.set_quantity` to set a property with a quantity or you can use the shortcut `person.q` like shown below:

```python
    >>> BodyMassIndex = dlite.get_instance("http://onto-ns.com/meta/0.1/BodyMassIndex")

    # create a BodyMassIndex instance
    >>> person = BodyMassIndex()

    # assign the name of the person
    >>> person.name = "John Doe"

    # assign the age of the person without a physical quantity
    >>> person.age = 5
    >>> assert person.age == 5.0

    # assign the age of the person with a physical quantity
    >>> person.set_quantity("age", 3.0, "years")
    >>> assert person.age == 3.0
    >>> person.q.age = (1461, "days")
    >>> assert person.age == 4.0

    # assign the height and the weight
    >>> person.q.height = "1.72m"
    >>> person.q.weight = "63kg"
    >>> assert person.height == 172.0
    >>> assert person.weight == 63.0

    # compute the BMI
    >>> person.q.bmi = person.q.weight.to("kg") / person.q.height.to("m") ** 2
    >>> assert np.round(person.get_quantity("bmi").magnitude) == 21.0

    # the following line will give the same result as the line above
    >>> person.q.bmi = person.q.weight / person.q.height ** 2

    # the following line will raise a TypeError exception, because the property
    # "category" with the type "string" cannot be converted into a quantity.
    >>> category = person.q.category

    # assign the category
    >>> person.category = "normal"
    >>> if person.bmi < 18.5:
    ...     person.category = = "underweight"
    ... elif person.bmi >= 25.0:
    ...     person.category = "overweight"

```

```note
If you need in your program to use a specific units registry, use the function
[`pint.set_application_registry()`]
```

The Python property `q` of the dlite.Instance objects as some other methods:

```python
    >>> assert person.q.names() == ["age", "height", "weight", "bmi"]
    >>> assert person.q.units() == ["year", "cm", "kg", "kg/m^2"]
    >>> for name, quantity in persion.q.items():
    ...     quantity.defaut_format = "~"  # format the unit with a compact format
    ...     print(f"{name} = {quantity}")

```

See [quantity formatting].

You should not keep the Python property `q` as a local variable, it will result with wrong assignment if you work with several instances.

```python
    # Don't do this!
    >>> p1 = person1.q
    >>> p2 = person2.q
    # this will assign the weight to person2, and not to person1
    >>> p1.weight = "34kg"

    # Instead, you should write the code like this
    >>> person1.q.weight = "34kg"
    >>> person2.q.weight = "32kg"

    # For more complex formulas, you can assign the quantities to local variables
    >>> w1, h1 = person1.q.get("weight", "height")
    >>> w2, h2, a2 = person2.q.get("weight", "height", "age")

```

The first line of the code above prepare the [singleton] `dlite.quantity.QuantityHelper` for the person1 and return the singleton.
The second line do the same as the first line, so the variable `p2` is the singleton and is prepared to work on `person2`.



[dlite-getuuid]: https://github.com/SINTEF/dlite/blob/master/doc/user_guide/tools.md#dlite-getuuid
[Concepts section of the DLite User Guide]: https://sintef.github.io/dlite/user_guide/concepts.html
[get_instance()]: https://sintef.github.io/dlite/autoapi/dlite/index.html?highlight=get_instance#dlite.get_instance

[examples]: https://github.com/SINTEF/dlite/tree/master/examples
[pint]: https://pint.readthedocs.io/en/stable/getting/overview.html
[singleton]: https://www.geeksforgeeks.org/singleton-pattern-in-python-a-complete-guide/
[quantity formatting]: https://pint.readthedocs.io/en/stable/user/formatting.html
[`pint.set_application_registry()`]: https://pint.readthedocs.io/en/0.10.1/tutorial.html#using-pint-in-your-projects
[pint installation]: https://pint.readthedocs.io/en/stable/getting/index.html#installation
