Collections
===========
A collection is a data instance that contain a set of (references to) other
instances and relationships between them.
It can contain (references to) other collections as well.
This is useful to represent the knowledge of the domain where
data exists, in order to find data that relates to other data, but
also to uniquely identify a complete data set with a single
identifier.

In DLite are collections simply implemented as a list of RDF relations
using the vocabulary defined in the [datamodel ontology].

Collection entities are defined as follows:
```json
{
  "uri": "http://onto-ns.com/meta/0.1/Collection",
  "description": "Meta-metadata description a collection.",
  "dimensions": {
    "nrelations": "Number of relations."
  },
  "properties": {
    "relations": {
      "type": "relation",
      "shape": ["nrelations"],
      "description": "Array of relations (s-p-o triples)."
    }
  }
}
```


<<<<<<< Updated upstream
Introduction
------------
xxx


Example
-------
xxx
=======
[datamodel ontology]: https://github.com/emmo-repo/datamodel/
>>>>>>> Stashed changes
