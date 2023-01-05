* Outline the tutorial. What will the user learn and how is it useful? 

* How to write an entity (based on data source e.g. solar cell data)
    - mention that entities are immutable
    - instances are mutable (unless they are snapshot of a transaction \ref)

    - identity of entity: uri or uuid

* Instantiate entity with DLite
    1. (Entity =) dlite.Instance.from_location('json', path_to_entity_file, ). Here we are using the json storage plugin. Explain what a storage plugin is \ref.
    2. Add filepath to storage path, then use dlite.get_instance() to fetch
    entity.
    3. there are more ways to do this ... 

* How to create data instance(s) from the entity
    1. Instantiate an (empty) instance of the entity, as you would instantiate an
    object of a python class. NB! Have to give dimensions.

    ( e = Entity(dims=[1]) or e = Entity(dims={nrows:1}) )
    Note!! If meta is not specified, a new entity will be created every time we 
    instantiate an instance. 


* Save data instances(s) using storage plugin


