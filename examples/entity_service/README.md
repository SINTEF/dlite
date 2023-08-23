Entity service example
======================

**Warning**: This example is work in progress...

This example contains a set of scripts.  `main.py` is the main script that runs
the rest in turns.  It is called when this example is run as a test.


Scripts
-------
- **start_service.sh**: Installs and starts the [dlite-entities-service] in a
  docker container.  It also starts a MongoDB instance listening to
  `localhost:27017`.
- **populate_database.py**: A Python script that populates the MongoDB database.
- **get_instance.py**: An example that shows how to append the MongoDB server
  to `dlite.storage_path` such that entities can be accessed seamlessly with
  `dlite.get_instance()`.
- **try_service.py**: An example that uses the [dlite-entities-service] to
  provide seamless access to entities.
- **stop_service.sh**: Stops the docker containers started by `start_service.sh`.



[dlite-entities-service]: https://github.com/SINTEF/dlite-entities-service
