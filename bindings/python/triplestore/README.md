Triplestore
===========
> A Python package encapsulating different triplestores using the strategy
> design pattern.

This package has by itself no dependencies outside the standard library,
but the triplestore backends may have.

The main class is Triplestore, who's `__init__()` method takes the name of the
backend to encapsulate as first argument.  Its interface is strongly inspired
by rdflib.Graph, but simplified when possible to make it easy to use.  Some
important differences:
- all IRIs are represented by Python strings
- blank nodes are strings starting with "_:"
- literals are constructed with `Literal()`

```python
from triplestore import Triplestore
ts = Triplestore(backend="rdflib")
```

The module already provides a set of pre-defined namespaces that simplifies
writing IRIs. For example:

```python
from triplestore import RDFS, OWL
RDFS.subClassOf
# -> 'http://www.w3.org/2000/01/rdf-schema#subClassOf'
```

New namespaces can be created using the Namespace class, but are usually
added with the `bind()` method:

```python
ONTO = ts.bind("onto", "http://example.com/onto#")
ONTO.MyConcept
# -> 'http://example.com/onto#MyConcept'
```

Namespace also support access by label and IRI checking.  Both of these features
requires loading an ontology.  The following example shows how to create an EMMO
namespace with IRI checking.  The keyword argument `label_annotations=True` enables
access by `skos:prefLabel`, `rdfs:label` or `skos:altLabel`.  The `check=True`
enables checking for existing IRIs.  The `triplestore_url=...` is a resolvable URL
that can be read by the 'rdflib' backend.  It is needed, because the 'rdflib'
backend is currently not able to load EMMO from the "http://emmo.info/emmo#"
namespace.

```python
EMMO = ts.bind(
    "emmo", "http://emmo.info/emmo#",
    label_annotations=True,
    check=True,
    triplestore_url="https://emmo-repo.github.io/versions/1.0.0-beta4/emmo-inferred.ttl",
)
EMMO.Atom
# -> 'http://emmo.info/emmo#EMMO_eb77076b_a104_42ac_a065_798b2d2809ad'
EMMO.invalid_name
# -> NoSuchIRIError: http://emmo.info/emmo#invalid_name
```

New triples can be added either with the `parse()` method (for
backends that support it) or the `add()` and `add_triples()` methods:

```python
# en(msg) is a convenient function for adding english literals.
# It is equivalent to ``triplestore.Literal(msg, lang="en")``.
from triplestore import en
ts.parse("onto.ttl", format="turtle")
ts.add_triples([
    (ONTO.MyConcept, RDFS.subClassOf, OWL.Thing),
    (ONTO.MyConcept, RDFS.label, en("My briliant ontological concept.")),
])
```

For backends that support it the triplestore can be serialised using
`serialize()`:

```python
ts.serialize("onto2.ttl")
```

A set of convenient functions exists for simple queries, including
`triples()`, `subjects()`, `predicates()`, `objects()`, `subject_predicates()`,
`subject_objects()`, `predicate_objects()` and `value()`.  Except for `value()`,
they return the result as generators. For example:

```python
ts.objects(subject=ONTO.MyConcept, predicate=RDFS.subClassOf)
# -> <generator object Triplestore.objects at 0x7fa502590200>
list(ts.objects(subject=ONTO.MyConcept, predicate=RDFS.subClassOf))
# -> ['http://www.w3.org/2002/07/owl#Thing']
```

The `query()` and `update()` methods can be used to query and update the
triplestore using SPARQL.

Finally Triplestore has two specialised methods `add_mapsTo()` and
`add_function()` that simplify working with mappings.  `add_mapsTo()` is
convinient for defining new mappings:

```python
from triplestore import Namespace
META = Namespace("http://onto-ns.com/meta/0.1/MyEntity#")
ts.add_mapsTo(ONTO.MyConcept, META.my_property)
```

It can also be used with DLite and SOFT7 data models.  Here we repeat
the above with DLite:

```python
import dlite
meta = dlite.get_instance("http://onto-ns.com/meta/0.1/MyEntity")
ts.add_mapsTo(ONTO.MyConcept, meta, "my_property")
```

The `add_function()` describes a function and adds mappings for its
arguments and return value(s).  Currently it only supports the [Function
Ontology (FnO)](https://fno.io/).

```python
def mean(x, y):
    """Returns the mean value of `x` and `y`."""
    return (x + y)/2

ts.add_function(
    mean,
    expects=(ONTO.RightArmLength, ONTO.LeftArmLength),
    returns=ONTO.AverageArmLength,
)
```


Further development
-------------------
* Update the `query()` method to return the SPARQL result in a backend-
  independent way.
* Add additional backends. Candidates include:
    - list of tuples
    - owlready2/EMMOntoPy
    - OntoRec/OntoFlowKB
    - Stardog
    - DLite triplestore (based on Redland librdf)
    - Redland librdf
    - Apache Jena Fuseki
    - Allegrograph
    - Wikidata
* Add ontological validation of physical dimension to Triplestore.mapsTo().
