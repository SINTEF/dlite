# A simple real world example for using dlite for chemical reaction and process

We want to be able to calculate the reaction energy of the following dehydrogenation reaction:

<img src="figs/reaction.png" alt="C2H6(g) --> C2H4(g) + H2(g)" width="500px">

The structure of all the involved molecules are stored in the folder
[molecules](molecules) in the
[xyz](https://en.wikipedia.org/wiki/XYZ_file_format) file format.



## Workflows
We will divide this example into three workflows:

1. simple workflow that explicitly calculates the reaction energy of the `C2H6(g) --> C2H4(g) + H2(g)` reaction
2. like workflow 1, but replaces the explicit calculation of the reaction energy
   in the second step with dlite mappings
3. like workflow 2, but uses ontological mappings to generate the dlite mappings


### 1. Simple workflow
Lets start with a very simple workflow, consisting of two steps:

#### Step 1.1: Create Molecule instances
Run the script

    python 1-simple-workflow/energies_from_atomscalecalcs.py

It will:
- read all the molecule structures in the [molecules](#molecules) directory
- calculate the corresponding molecule ground state energies using the [ASE EMT](https://wiki.fysik.dtu.dk/ase/ase/calculators/emt.html#module-ase.calculators.emt) calculator
- for each molecule, instantiate a DLite Molecule instance populated with the structure information and calculated energy
- add all the Molecule instances to a new collection
- write the collection to disk: [atomscaledata.json](#1-simple-workflow/atomscaledata.json)

The generated [atomscaledata.json](#1-simple-workflow/atomscaledata.json) file can be seen as a database of molecule structures with energies calculated using the EMT calculator.


#### Step 1.2: Explicit calculation of reaction energy
By running the script

    python 1-simple-workflow/calculate_reaction.py

you will:
- load the [atomscaledata.json](#1-simple-workflow/atomscaledata.json) database
- use that to calculate the energy of the chemical reaction `C2H6(g) --> C2H4(g) + H2(g)`
- populate a new [Reaction](#entities/Reaction.json) instance
- write the new Reaction instance to [file](#ethane-dehydrogenation.json)
- prints the calculated reaction energy to screen


### 2. Workflow using DLite mappings
This is very similar to workflow 1.  It starts from the same [collection of molecules](#1-simple-workflow/atomscaledata.json) as generated in step 1.1.  However, step 1.2 is replaced by running the script

    python 2-dlite-mappings/create_substances.py

The main difference is that the Molecule instances are implicitly converted Substance instances by providing the Substance URI as a second argument to `coll.get()`:

```python
    substance = coll.get(label, substance_id)
```

This conversion is performed with the `molecule2substance.py` dlite mapping plugin in the `python-mapping-plugins/` subdirectory.


### 3. Workflow based on ontological mappings
Two examples are provided:

1. The mappings are hard coded into the run script, which can be run directly with python.
Note that the data are obtained from same [collection of molecules](#1-simple-workflow/atomscaledata.json) as in the two
previous cases.

The actual mapping betweeen the instances of required molecules in the [collection of molecules](#1-simple-workflow/atomscaledata.json)  and the Substance instances needed in the reaction calculation (`get_energy`) is done with the dlite function `make_instance`.
Arguments provided are the target metadata (here Substance), instance(s) that the new Substance instance should be populated from and the mappings.
See documentation for other possible arguments.

2. The mappings are obtained by use of ontologies in a two step process.
This step requires the python package EMMOntoPy.
Install EMMOntoPy from github and not PyPi to obtain some needed functionality not yet in the released version.


The mappings to the common ontology (chemistry.ttl) are first done with the scripts map_molecule.py and map_substance.py, resulting in two ontologies with the actual mappings.
In the second step these mappings are read into the run script and combined into a list of triples for all relevant mappings.


In this final example a situation in which two separate processes are mapped to the same ontology is showcased, thus enabling interoperability even though the users do not have detailed knowledge about both cases. 







## Metadata used in the example
You find all metadata in this example in the [entities](#entities) folder.

### Molecule data model
First we define a DLite data model describing a [molecule](#entities/Molecule.json).
In addition to the information available in the structures, this data model also has a ground state energy property.

### Substance data model
The [substance data model](#entities/Substance.json) is very simple. It only defines two properties, an id (or name) identifying a substance and its molecular energy.

### Reaction data model
The [reaction data model](#entities/Reaction.json) defines a chemical reaction including its reaction energy.  It has two dimensions; the number of reactants and the number of products.
