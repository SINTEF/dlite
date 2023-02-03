# Tutorial

## Introduction
This tutorial shows the steps for creating and instantiating a Dlite entity and populating it with data. For this, we will use an example `csv` file containing data corresponding to time measurements from a solar panel. The example file has the following format:


| time                 | Impp [A] | Isc [A] | MPP [W] | Vmpp [V] | Voc [V] |
|----------------------|----------|---------|---------|----------|---------|
| 2022-08-01 00:41:00  |  1.188   |   1.25  |  38.182 |   32.14  |  37.33  |
|  ...                 |   ...    |   ...   |   ...   |    ...   |   ...   |



## Writing a DLite Entity
Let us write an Entity representing our solar panel measurement. We start by creating a `json` file called `myEntity.json`.

### **Step 1**: Giving it a unique identifier
Firstly, we must provide a unique identifier for our Entity. There are several ways of doing this:

#### **1.  Uniform Resource Identifier (URI)**
We can create and assign a unique Uniform Resource Identifier (URI). A URI has the form **namespace/version/nameOfEntity**, and it can be either written directly in the json file:

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
Alternatively, we can generate and assign a universally unique identifier (UUID). In Python, this can be achieved by the use of the `dlite.get_uuid()` function or the [dlite-getuuid](https://github.com/SINTEF/dlite/blob/master/doc/user_guide/tools.md#dlite-getuuid) tool.

```python
import uuid

# Generate a random UUID
entity_uuid = uuid.uuid4()

with open("myEntity.json", "w") as entity_file:
    entity_file.write('"uuid": {}'.format(entity_uuid))
```
Our json file now contains the following:

```json
"uuid": <uuid>
```
where `<uuid>` is a unique identifier for our Entity. 


### **Step 2**: Providing a link to the metadata
Let us assume that we have followed option 1 and provided a URI. The next step consists in adding a **meta** field that will link the Entity to its metadata. In DLite, all Entities are instances of the *EntitySchema*. The *EntitySchema* defines the structure of the Entity. In the **meta** field of our Entity, we need to provide the URI of the EntitySchema. For a schematic overview of the DLite data structures and further detail, see [Concepts section of the DLite User Guide](https://sintef.github.io/dlite/user_guide/concepts.html). We add this information to our json file in the following manner:
```json
"uri": "http://www.ontotrans.eu/0.1/solarPanelMeasurement",
"meta": "http://onto-ns.com/meta/0.3/EntitySchema",
```
**Note** The **meta** field actually defaults to "http://onto-ns.com/meta/0.3/EntitySchema". This means that the **meta** field is optional for entities.
### **Step 3**: Adding a human-understandable description
Next, we want to include a human-understandable description of what the Entity represents. In our example case, such a **description** field could be

```json
"uri": "http://www.ontotrans.eu/0.1/solarPanelMeasurement",
"meta": "http://onto-ns.com/meta/0.3/EntitySchema",
"description": "Measurement data from one solar panel."
```

### **Step 4**: Defining the dimensions
We continue by defining the dimensions for our Entity. Each dimension should have a **name** and a human-understandable **description**. In our example case, we only have one dimension since all the quantities we are interested in are measured with the same frequency (i.e., we have the same number of data points for all the measured quantities). Note that even though we only have one dimension, we need to give it in a list format.

We will give our dimension the generic name "N", and describe it as the number of measurements:

```json
"uri": "http://www.ontotrans.eu/0.1/solarPanelMeasurement",
"meta": "http://onto-ns.com/meta/0.3/EntitySchema",
"description": 
 "dimensions": [
      {
      "name": "N",
      "description": "Number of measurements."
      }
    ]
```

### **Step 5**: Defining the properties
Now it is time to define the properties of our Entity. Here is where we can give meaning to our data. As for the dimensions, we add the properties as a list of `json` structures, each one having a **name**, **type**, **unit**, **description**, and if relevant, a list of **dimension**(s) (here called **dims**). Inserting the properties displayed in the table above, our Entity is complete and may look like

```json
{
    "uri": "http://www.ontotrans.eu/0.1/solarPanelMeasurement",
    "meta": "http://onto-ns.com/meta/0.3/EntitySchema",
    "description": "Measurement data from one solar panel.",
    "dimensions": [
      {
      "name": "N",
      "description": "Number of measurements."
      }
    ],
    "properties": [
        {
            "name":"t",
            "type":"str",
            "unit":"ns",
            "dims": ["N"],
            "description": "Time"
        },

        {
            "name":"MPP",
            "type":"float64",
            "unit":"W",
            "dims": ["N"], 
            "description": "Maximum Power" 
        },
        {
            "name": "impp",
            "type": "float64",
            "unit": "A",
            "dims": ["N"],
            "description": "Maximum power point current."
        },
        {
            "name": "isc",
            "type": "float64",
            "unit": "A",
            "dims": ["N"],
            "description": "Short circuit current."
        },

        {
            "name": "vmpp",
            "type": "float64",
            "unit": "V",
            "dims": ["N"],
            "description": "Maximum power point voltage."
        },
        {
            "name": "voc",
            "type": "float64",
            "unit": "V",
            "dims": ["N"],
            "description": "Open circuit voltage."
        }
    ]
}
```

### **Note**
Both dimensions and properties can also be provided using a `dict` with the name as key. This may look like
``` json
"dimensions": {"N": "Number of measurements."},


"properties": {
      "t": {
            "type":"str",
            "unit":"ns",
            "dims": ["N"],
            "description": "Time"
           },
    "MPP": {
            "type":"float64",
            "unit":"W",
            "dims": ["N"],
            "description": "Maximum Power"
            },
    ...
               }
```
DLite supports both syntaxes.
## Loading an Entity with DLite
We will now load our Entity in Python. There are several ways of doing this. 

### 1. Using the json storage plugin:
```python
import dlite

SolarPanelEntity = dlite.Instance.from_location('json', 'myEntity.json')

```

### 2. Adding the file to `storage_path`
In this method, we append the `json` file to `storage_path` and then use the [get_instance()](https://sintef.github.io/dlite/autoapi/dlite/index.html?highlight=get_instance#dlite.get_instance) function to load the Entity using its URI. In Python, this may be done by

``` python
import dlite

# Append the entity file path to the search path for storages
dlite.storage_path.append('myEntity.json')

# Load Entity using its uri
SolarPanelEntity = dlite.get_instance("http://www.ontotrans.eu/0.1/solarPanelMeasurement")
```

**Note:** The same approach applies if a UUID is used.


## Connect loaded entity to data

Now that we have loaded our Entity, we can instantiate it, and populate with the data from the `csv` file mentioned in the introduction. Note that you may have to provide a full file path to the `read_csv()` function, depending on where you have stored your `csv` file.


```python
import pandas as pd

# Read csv file using pandas, store its contents in a pandas DataFrame
solar_panel_dataframe = pd.read_csv('data.csv', sep=',', header = 0)

# Instantiate an instance of the Entity that we loaded above
inst = SolarPanelEntity({"N": len(solar_panel_dataframe)})

# Create a dicitionary that defines the mapping between property names and column headers
mapping = {
       "t":"time",
     "MPP":"MPP [W]",
     "voc":"Voc [V]",
    "vmpp":"Vmpp [V]",
    "isc" :"Isc [A]",
    "impp":"Impp [A]"
    
}

# Loop through the dictionary keys and populate the instance with data
for key in mapping:
    inst[key] = solar_panel_dataframe[mapping[key]]

```
