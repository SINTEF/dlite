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

Collection entities are, in YAML representation, defined as follows:
```yaml

  uri: http://onto-ns.com/meta/0.1/Collection
  description: Meta-metadata description a collection.
  dimensions:
    nrelations: Number of relations.
  properties:
    relations:
      type: relation
      shape: [nrelations]
      description: Array of relations (s-p-o triples).
```

Example
-------
```python
>>> import dlite

# Create some instances
>>> Blob = dlite.get_instance('http://onto-ns.com/meta/0.1/Blob')  # metadata
>>> blob1 = Blob(dims={"n": 2}, id="ex:blob1")
>>> blob2 = Blob(dims={"n": 1}, id="ex:blob2")
>>> blob3 = Blob(dims={"n": 5}, id="ex:blob3")

# Create a collection
>>> coll = dlite.Collection(id="ex:coll")

# Add instances to the collection
>>> coll.add("blob1", blob1)
>>> coll.add("blob2", blob2)
>>> coll.add("blob3", blob3)

# Add relations between the instances
>>> coll.add_relation("blob1", "emmo:hasPart", "blob2")
>>> coll.add_relation("blob1", "emmo:hasPart", "blob3")
>>> coll.add_relation("blob2", "emmo:hasPart", "blob3")

# Print the collection
>>> print(coll)
{
  "uuid": "622d1aa3-4233-56bf-bab9-9b9d8c2aeab0",
  "uri": "ex:coll",
  "meta": "http://onto-ns.com/meta/0.1/Collection",
  "dimensions": {
    "nrelations": 12
  },
  "properties": {
    "relations": [
      [
        "blob1",
        "_is-a",
        "Instance"
      ],
      [
        "blob1",
        "_has-uuid",
        "a8645591-cc43-563f-8005-8ec63852ab6f"
      ],
      [
        "blob1",
        "_has-meta",
        "http://onto-ns.com/meta/0.1/Blob"
      ],
      [
        "blob2",
        "_is-a",
        "Instance"
      ],
      [
        "blob2",
        "_has-uuid",
        "3fa5a23e-e744-5dd6-94b5-28de06017bef"
      ],
      [
        "blob2",
        "_has-meta",
        "http://onto-ns.com/meta/0.1/Blob"
      ],
      [
        "blob3",
        "_is-a",
        "Instance"
      ],
      [
        "blob3",
        "_has-uuid",
        "f6cc739b-db7a-5fa1-808e-a6788033077d"
      ],
      [
        "blob3",
        "_has-meta",
        "http://onto-ns.com/meta/0.1/Blob"
      ],
      [
        "blob1",
        "emmo:hasPart",
        "blob2"
      ],
      [
        "blob1",
        "emmo:hasPart",
        "blob3"
      ],
      [
        "blob2",
        "emmo:hasPart",
        "blob3"
      ]
    ]
  }
}

```

[datamodel ontology]: https://github.com/emmo-repo/datamodel/
