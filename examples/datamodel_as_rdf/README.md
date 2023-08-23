Datamodels as RDF
=================
This example will show how DLite and Pydantic data models can be
serialised to RDF.

Two examples are included:
- **pydantic_nested**: Shows serialisation of nested pydantic models into RDF.
  Works for both Pydantic v1 and v2.
- **dataresource**: Shows serialisation of a Pydantic model of a OTEAPI
  dataresource to RDF.  This example has two versions, one for Pydantic v1
  ([dataresource.py](dataresource.py)) and one for Pydantic v2
  ([dataresource_pydantic2.py](dataresource_pydantic2.py)).
