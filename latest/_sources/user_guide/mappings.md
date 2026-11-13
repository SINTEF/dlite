Mappings
========

Content
-------
  1. [Introduction](#introduction)
  2. [Instance mappings](#instance-mappings)
  3. [Property mappings](#property-mappings)
  4. [Which kind of mapping to choose](#which-kind-of-mapping-to-choose)


Introduction
------------
Mappings convert data described by one datamodel into data described by
another datamodel.  DLite distinguishes between two kinds of mappings:

- **Instance mappings** convert a whole instance of one metadata into an
  instance of another metadata.  They are implemented by *mapping
  plugins*, which may be written in C or Python, and are typically used
  for metadata that represent the same kind of thing at different levels
  of detail.
- **Property mappings** map individual properties between datamodels via
  an ontology, using the [tripper] library.  They make it possible to
  describe *how* the properties of a datamodel correspond to concepts in
  an ontology, and let DLite use these descriptions to automatically
  instantiate new instances from existing data.


Instance mappings
-----------------
An instance mapping plugin converts a sequence of input instances into
an instance of a given output metadata.

### Using instance mappings

Instance mappings are typically invoked via a [collection]:

```python
>>> substance = coll.get('C2H6', 'http://onto-ns.com/meta/0.1/Substance')
```

If the instance labelled `'C2H6'` in the collection is an instance of
some other metadata (say `Molecule`), DLite looks up a registered
mapping plugin that takes `Molecule` instances as input and produces
`Substance` instances, applies it and returns the result.  If no such
mapping is registered, a `DLiteMappingError` is raised.

The top-level mapping function is also available directly:

```python
>>> substance = dlite.mapping('http://onto-ns.com/meta/0.1/Substance',
...                           [molecule])
```

If several mappings can produce the requested output, the one with the
lowest cost is selected.  Mapping plugins have a `cost` attribute for
this purpose.  The default cost is 25, while the trivial mapping to an
existing input has cost zero.

### Writing an instance mapping plugin

A Python instance mapping plugin is a Python module defining a subclass
of `dlite.DLiteMappingBase` with the following class attributes:

| Attribute | Type | Description |
|-----------|------|-------------|
| `name` | str | Name of the mapping. |
| `input_uris` | sequence of str | URIs of the metadata of the input instances. |
| `output_uri` | str | URI of the metadata of the produced instance. |
| `cost` | int | Cost of the mapping (optional, default is 25). |

and a `map()` method that takes the sequence of input instances as
argument and returns a new instance of `output_uri`:

```python
import dlite


class Molecule2Substance(dlite.DLiteMappingBase):
    name = 'Molecule2Substance'
    input_uris = ['http://onto-ns.com/meta/0.1/Molecule']
    output_uri = 'http://onto-ns.com/meta/0.1/Substance'
    cost = 25

    def map(self, instances):
        molecule = instances[0]
        substance = dlite.Instance.from_metaid(self.output_uri, [])
        substance.id = molecule.name
        substance.molecule_energy = molecule.groundstate_energy
        return substance
```

For DLite to find the plugin, its module must be in the search path
given by the `DLITE_PYTHON_MAPPING_PLUGIN_DIRS` environment variable or
appended to `dlite.python_mapping_plugin_path` (see [Search paths]).

:::{attention}
The base class must be referred to by its **qualified name**
(`dlite.DLiteMappingBase`).  Since DLite v0.5.23, plugins are evaluated
in separate scopes, so unqualified names from the `dlite` module are not
available in a plugin module.
:::

A complete, runnable example is provided in
[examples/dehydrogenation], where `Molecule` instances from a simple
workflow are mapped to `Substance` instances to compute reaction
energies.

Mapping plugins can also be written in C, using the same plugin
mechanism as C storage plugins.  A C mapping plugin is a shared library
defining a `get_dlite_mapping_api()` function returning a
`DLiteMappingPlugin` struct.  See the [C reference manual] for details.

### Unloading mapping plugins

Loaded mapping plugins can be unloaded with:

```python
>>> dlite.mapping_plugin_unload('Molecule2Substance')  # unload one
>>> dlite.mapping_plugin_unload()                     # unload all
```


Property mappings
-----------------
Property mappings are based on [tripper] and describe, via an ontology,
how the properties of datamodels correspond to each other.  Combined
with mapping functions, this allows new instances to be automatically
instantiated from data described by other datamodels.

Property mappings require the optional dependencies listed in
`requirements_mappings.txt` in the [DLite repository] (most notably
[tripper] and [pint]).

### How it works

A property mapping is expressed as triples in a triplestore connecting
properties of datamodels to concepts in an ontology.  The example below
(from [examples/mappings]) maps properties of two datamodels to concepts
of the [Elementary Multiperspective Material Ontology (EMMO)] and
provides Python functions for the concepts that require computation:

```python
>>> from tripper import EMMO, Triplestore
>>> import dlite

>>> coll = dlite.Collection()          # our knowledge base
>>> ts = Triplestore(backend="collection", collection=coll)
>>> DON = ts.bind("don", "http://example.com/demo-ontology#")

# Add mappings from the input datamodels -- data provider
>>> ts.map(AT.symbols, DON.ChemicalSymbol)
>>> ts.map(RES.potential_energy, EMMO.PotentialEnergy)
>>> ts.map(RES.forces, EMMO.Force)

# Add mappings to the output datamodel -- modeller
>>> ts.map(MOL.energy, EMMO.PotentialEnergy)
>>> ts.map(MOL.maxforce, DON.MaxForce)

# Add mapping functions -- ontologist
>>> ts.add_function(norm, expects=[EMMO.Force], returns=[DON.ForceNorm])
```

With these mappings in place, an instance of `MOL` can be created
directly from the collection:

```python
>>> molecule, = coll.get_instances(metaid=MOL, property_mappings=True)
```

DLite resolves the mapping routes from the source properties to the
target properties, applies the mapping functions and returns a new
instance with all properties populated.

### Lower-level mapping functions

The `dlite.mappings` module provides the underlying functionality:

- `instance_routes(meta, instances, triplestore)`: returns a dict of
  all possible mapping routes for populating an instance of `meta`.
- `instantiate(meta, instances, triplestore)`: creates a new instance of
  `meta` populated with the selected mapping routes.
- `instantiate_all(meta, instances, triplestore)`: like `instantiate()`,
  but returns a generator over all possible instances.

See the docstrings of these functions for a complete description of
their arguments.  Units are handled with [pint] and shapes with numpy,
so mappings between properties with different units and dimensions are
resolved automatically.

:::{warning}
Some older examples in the repository (under
`examples/dehydrogenation/3-property-mappings/`) refer to a
`make_instance()` function that no longer exists.  Use
`instantiate()` or `coll.get_instances(..., property_mappings=True)`
instead.
:::


Which kind of mapping to choose
--------------------------------

| | Instance mappings | Property mappings |
|---|---|---|
| Granularity | Whole instances | Individual properties |
| Ontology | Not required | Required (tripper/triplestore) |
| Implementation | Mapping plugin (C/Python) | RDF triples + mapping functions |
| Units | Manual | Automatic (pint) |
| Best suited for | Connecting workflows with known datamodels | Semantic interoperability across domains |

In short: use an **instance mapping** when you know both datamodels and
just need to convert between them.  Use **property mappings** when the
correspondences should be expressed with proper semantics, e.g. in a
setting where datamodels and ontologies evolve independently.


[tripper]: https://github.com/EMMC-ASBL/tripper
[pint]: https://pint.readthedocs.io/
[collection]: https://sintef.github.io/dlite/user_guide/collections.html
[Search paths]: https://sintef.github.io/dlite/user_guide/search_paths.html
[examples/mappings]: https://github.com/SINTEF/dlite/tree/master/examples/mappings
[examples/dehydrogenation]: https://github.com/SINTEF/dlite/tree/master/examples/dehydrogenation
[Elementary Multiperspective Material Ontology (EMMO)]: https://emmo-repo.github.io/
[C reference manual]: https://sintef.github.io/dlite/dlite/mapping.html
[DLite repository]: https://github.com/SINTEF/dlite
