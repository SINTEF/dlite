# Tutorial

## Questions
1. Should we use the ontotrans namespace? Do we have anything more general?

## Notes to selves
1. We should say something about what onto-ns is (where EntitySchema is, or at least should, be hosted).
2. Must find somewhere to mention that entities are immutable while instances are not.

## Introduction


## Writing a DLite Entity
We will now write a DLite Entity for our example data. 

### Step 1: Giving it a unique identifier
Let's write an Entity representing our solar panel measurement. 

Firstly, we must provide a unique identifier for our Entity. There are several ways to do this:

1.  We can generate and assign a universally unique identifier (UUID). In Python, this can be achieved by the use of the [UUID module](https://docs.python.org/3/library/uuid.html):

```python
import uuid

# Generate a random UUID
uuid = uuid.uuid4()
```

2.  We can create and assign a unique uri of the form namespace/version/nameOfEntity:

```json
"uri": "http://www.ontotrans.eu/0.1/solarPanelMeasurement",
```

3.  We can give the namespace, version, and name for our Entity as separate fields:

```json
"name": "solarPanelMeasurement",
"namespace": "http://www.ontotrans.eu/",
"version": "0.1"
```

### Step 2: Providing a link to the metadata
<span style="color:red">Really bad paragraph, needs work.</span>

We will continue using method 2. All Instances in DLite have a defined metadata. While DLite data instances are instances of entities, the entities themselves are instances of the EntitySchema. In order for DLite to "understand" that what we are creating is in fact an Entity, we need to "tell it" that the EntitySchema is the metadata of our thingy. In order words, we must add a "meta field" to our json file:

```json
"uri": "http://www.ontotrans.eu/0.1/solarPanelMeasurement",
"meta": "http://onto-ns.com/meta/0.3/EntitySchema",
```

### Step 3: Adding a human-understanable description
Next, we want to include a human-understandable description of what the Entity represents. In our example case, such a description could be <span style="color:red">blablabla</span>.

```json
"uri": "http://www.ontotrans.eu/0.1/solarPanelMeasurement",
"meta": "http://onto-ns.com/meta/0.3/EntitySchema",
"description":
```

### Step 4: Defining the dimensions
We continue by defining the dimensions for our Entity. Each dimension should have a name and a human-understandable description. In our example case we only have one dimension since all the quantities we are interested in are measured with the same frequency (i.e. we have the same number of data points for all quantities). Note that even though we only have one dimension, we need to give it in a list format.

We will give our dimension the generic name "N", and describe it as the number of rows in our csv file:

```json
"uri": "http://www.ontotrans.eu/0.1/solarPanelMeasurement",
"meta": "http://onto-ns.com/meta/0.3/EntitySchema",
"description": 
 "dimensions": [
      {
      "name": "N",
      "description": "Number of rows."
      }
    ]
```

### Step 5: Defining the properties
Now it is time to define the properties of our Entity. This is where we can describe what our data signifies. As for the dimensions we add the properties as a list of json structures, where each property should have a name, type, desciption, and if relevant, a list of dimension(s) (abbreviated to dims). "If relevant" here means that if a property has a dimensionality of zero (i.e., it is a scalar), the `dims` field can be ommited.

Inserting our properties, our Entity is complete and looks something like this:

```json
{
    "uri": "http://www.ontotrans.eu/0.1/solarPanelMeasurement",
    "meta": "http://onto-ns.com/meta/0.3/EntitySchema",
    "description": "Measurement data from one x solar panel.",
    "dimensions": [
      {
      "name": "N",
      "description": "Number of rows."
      }
    ],
    "properties": [
        {
            "name": "Impp [A]",
            "type": "float",
            "dims": ["N"],
            "description": " "
        },
        {
            "name": "irradiance [W/m²]",
            "type": "int",
            "dims": ["N"],
            "description": " "
        },
        {
            "name": "Irradiation Yield ¹ [Wh/m²]",
            "type": "float",
            "dims": ["N"],
            "description": " "
        },
        {
            "name": "isc [A]",
            "type": "float",
            "dims": ["N"],
            "description": " "
        },
        {
            "name": "MPP Yield ¹ [Wh]",
            "type": "float",
            "dims": ["N"],
            "description": " "
        },
        {
            "name": "MPP ¹ [W]",
            "type": "float",
            "dims": ["N"],
            "description": " "
        },
        {
            "name": "vmpp [V]",
            "type": "float",
            "dims": ["N"],
            "description": " "
        },
        {
            "name": "voc [V]",
            "type": "float",
            "dims": ["N"],
            "description": " "
        },
    ]
}

```

## Instantiating an Entity with DLite