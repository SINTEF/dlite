# Tutorial
<!-- 
## Questions
1. Should we use the ontotrans namespace? Do we have anything more general?

## Notes to selves
1. We should say something about what onto-ns is (where EntitySchema is, or at least should, be hosted).
2. Must find somewhere to mention that entities are immutable while instances are not.

## Introduction -->
This tutorial shows the steps for creating and instantiating a Dlite entity, and connecting . For this, we will use an example `csv` file containing data corresponding to time measurements from a solar panel. The example file

| time | Impp [A] | Isc [A] | MPP [W] | Vmpp [V] | Voc [V] |
|------|----------|---------|---------|----------|---------|
|   0  |          |         |         |          |         |
|  10  |          |         |         |          |         |

## Writing a DLite Entity
We will now write a DLite Entity for our example data. 

### **Step 1**: Giving it a unique identifier
Let us write an Entity representing our solar panel measurement. Firstly, we must provide a unique identifier for our Entity. There are several ways to do this:

#### **1.  Universally Unique Identifier (UUID) with `python`**
We can generate and assign a universally unique identifier (UUID). In Python, this can be achieved by the use of the [UUID module](https://docs.python.org/3/library/uuid.html)

```python
import uuid

# Generate a random UUID
uuid = uuid.uuid4()
```

#### **2.  Uniform Resource Identifier (URI) in a separate `json` file**
In a json file, we can create and assign a unique Uniform Resource Identifier (URI). An URI has the form **namespace/version/nameOfEntity**, and it can be either written directly in the json file as 

```json
"uri": "http://www.ontotrans.eu/0.1/solarPanelMeasurement",
```

or, alternatively, we can give the **namespace**, **version**, and **name** of our Entity as separate fields

```json
"name": "solarPanelMeasurement",
"namespace": "http://www.ontotrans.eu/",
"version": "0.1"
```

### **Step 2**: Providing a link to the metadata
<!-- <span style="color:red">Really bad paragraph, needs work.</span> -->

All Instances in DLite have a defined metadata. Let us assume that we have provided an URI using a separate `json` file. The next step towards creating a Dlite entity is linking the Entity to metadata. This is done by adding a **meta** field to the `json` file
```json
"uri": "http://www.ontotrans.eu/0.1/solarPanelMeasurement",
"meta": "http://onto-ns.com/meta/0.3/EntitySchema",
```
While DLite data instances are instances of entities, the entities themselves are instances of the EntitySchema. In order for DLite to "understand" that what we are creating is in fact an Entity, we need to "tell it" that the EntitySchema is the metadata of our thingy.

### **Step 3**: Adding a human-understandable description
Next, we want to include a human-understandable description of what the Entity represents. In our example case, such a **description** field could be

```json
"uri": "http://www.ontotrans.eu/0.1/solarPanelMeasurement",
"meta": "http://onto-ns.com/meta/0.3/EntitySchema",
"description": "Measurement data from one solar panel."
```

### **Step 4**: Defining the dimensions
We continue by defining the dimensions for our Entity. Each dimension should have a **name** and a human-understandable **description**. In our example case, we only have one dimension since all the quantities we are interested in are measured with the same frequency (i.e. we have the same number of data points for all quantities). Note that even though we only have one dimension, we need to give it in a list format.

We will give our dimension the generic name "N", and describe it as the number of rows in our csv file

```json
"uri": "http://www.ontotrans.eu/0.1/solarPanelMeasurement",
"meta": "http://onto-ns.com/meta/0.3/EntitySchema",
"description": 
 "dimensions": [
      {
      "name": "N",
      "description": "Number of time points."
      }
    ]
```
<!-- Note also that if the data file only contains one data point of measurements, the dimension should be set to 1. -->
### **Step 5**: Defining the properties
Now it is time to define the properties of our Entity. Here, we can describe what our data signifies. As for the dimensions, we add the properties as a list of `json` structures, each one having a **name**, **type**, **unit**, **description**, and if relevant, a list of **dimension**(s) (abbreviated to shape). The `shape` field can be omitted if the property has a dimensionality of zero (i.e., it is a scalar). Inserting the properties displayed in the table above, our Entity is complete and may look like

```json
{
    "uri": "http://www.ontotrans.eu/0.1/solarPanelMeasurement",
    "meta": "http://onto-ns.com/meta/0.3/EntitySchema",
    "description": "Measurement data from one solar panel.",
    "dimensions": [
      {
      "name": "N",
      "description": "Number of rows."
      }
    ],
    "properties": [
        {
            "name": "impp",
            "type": "float",
            "unit": "A",
            "shape": ["N"],
            "description": "Maximum power point current."
        },
        {
            "name": "isc",
            "type": "float",
            "unit": "A",
            "shape": ["N"],
            "description": "Short circuit current."
        },
        {
            "name": "mpp",
            "type": "float",
            "unit": "W",
            "shape": ["N"],
            "description": "Maximum power."
        },
        {
            "name": "vmpp",
            "type": "float",
            "unit": "V",
            "shape": ["N"],
            "description": "Maximum power point voltage."
        },
        {
            "name": "voc",
            "type": "float",
            "unit": "V",
            "shape": ["N"],
            "description": "Open circuit voltage."
        }
    ]
}
```

## Instantiating an Entity with DLite
We will now instantiate our Entity in Python. There are several ways of doing this. 

### 1. Using the storage plugin
DLite has an interface that allows implicitly creating [storage](link_to_storage). The class method `dlite.Instance.from_location()` allows to read an entity file from a given location. Here, we need to specify the entity file and file type (`json`, in our example) 
```python
import dlite

Entity = dlite.Instance.from_location('json', path_to_entity_file)
```

### 2. Adding the file to `storage_path`
The variable `storage_path` contains a list of all entities in [storage](link_to_storage). In this method, we [append](link_to_append) the new entity `json` file to `storage_path` and then use the [get_instance](link_to_get_instance) function to define an instance out of the provided URI. This may be done through
``` python
import dlite
import json

f = open('entity_file.json')
data = json.load(f)

dlite.storage_path.append('path_to_entity_file.json')

Entity = dlite.get_instance(data['uri']) 
# data['uri'] = http://www.ontotrans.eu/0.1/solarPanelMeasurement
```


 <!-- * Instantiate entity with DLite
    1. (Entity =) dlite.Instance.from_location('json', path_to_entity_file, ). Here we are using the json storage plugin. Explain what a storage plugin is \ref.
    2. Add filepath to storage path, then use dlite.get_instance() to fetch
    entity.
    3. there are more ways to do this ...  -->

## Connect instantiated entity to data

Now that we have instantiated an Entity, we can connect it to the `csv` data file described in the introduction. This can be done in two ways

### 1. Instantiating from URL

In this method, we use the function `dlite.Instance.from_url()` and evaluate it with file type (**csv**, in our example) and the data file. If nothing else is added, `dlite.Instance.from_url(f'csv://path_to_csv')` will create a new entity instance, which will not be connected to Entity. To amend this, we need to add a **meta** field in the argument of `dlite.Instance.from_url()` and evaluate it with the URI of Entity.  All together may look like
<!-- Here, we access the URI of our entity the **uri** attribute of Entity and we use it to evaluate the **meta** field in the argument of `dlite.Instance.from_url()` -->
``` python
import dlite

my_Instance = dlite.Instance.from_url(f'csv://path_to_csv'+'?meta='+Entity.uri)
# Entity.uri = http://www.ontotrans.eu/0.1/solarPanelMeasurement
```

### 2. Instantiating from location

In this method we use `dlite.Instance.from_location` described above to directly access the `csv` file. In the **options** field, the URI of Entity needs to be provided so that the instance created from the `csv` file is connected to Entity. This yields
``` python
import dlite

my_Instance = dlite.Instance.from_location('csv', location='path_to_csv', options='meta'+Entity.uri+';infer=False')
# Entity.uri = http://www.ontotrans.eu/0.1/solarPanelMeasurement
```