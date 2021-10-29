# A simple real world example for using dlite for chemical reaction and process

We want to be able to calculate the reaction energy of the following dehydrogenation reaction:

![C2H6(g) --> C2H4(g) + H2(g)](figs/reaction.png)

The structure of all the involved molecules are stored in the folder
[molecules](molecules) in the
[xyz](https://en.wikipedia.org/wiki/XYZ_file_format) file format.


## Metadata used in the example

### Molecule data model
First we define a DLite data model describing a molecule.
In addition to the information available in the structures, this data model also has a ground state energy property.


### Substance data model


### Reaction data model


## Steps

### Step 1: Create Molecule instances


### Step 2: Map Molecule instances to Substance instances


### Step 3: Calculate Reaction instances
