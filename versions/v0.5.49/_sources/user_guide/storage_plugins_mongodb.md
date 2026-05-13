MongoDB storage plugin
======================

Quick guide
-----------

This is a quickguide for users wishing to connect to a mongodb server.
It is assumed that they have access to a server and have their own username
and password (or they have access to a connection string).

The MongoDB dlite plugin used the Python client
[pymongo.MongoClient](https://pymongo.readthedocs.io/en/stable/api/pymongo/mongo_client.html#pymongo.mongo_client.MongoClient)
to connect to the server. The main parameter to set-up a connection
to a MongoDB database is the URI, many options like the user name and the
password can be defined in the URI (see
[mongodb URI](https://www.mongodb.com/docs/manual/reference/connection-string/#connection-string-formats)
for more details).

First, set up the connection string:

```python
>>> user = "USERNAME"
>>> password = "PASSWORD"
>>> server = "MYSERVERADDRESS"
>>> uri =  f"mongodb+srv://{user}:{password}@{server}"
```

Then, you must define the name of the database and the name of the collection.
The dlite storage will store and retireve instances from a collection
(see the definitions of [Databases and Collections](https://www.mongodb.com/docs/manual/core/databases-and-collections/)).

```python
>>> database = "test-database"
>>> collection = "test-collection"
```

With those information you can create a dlite storage instance.

```python
>>> import dlite
>>> options = f"mode=w,database={database},collection={collection}"
>>> storage = dlite.Storage("mongodb", uri, options)
```

This storage instance can then be used like any other dlite storage instance.
For example saving and loading instances:

```python
>>> inst1 = dlite.Instance.from_location("json", "MY/INSTANCE/PATH.JSON")
>>> storage.save(inst1)
>>> storage.load(inst1.uuid)
```

Replace "MY/INSTANCE/PATH.JSON" with the path to your instance file,
and make sure to use the correct username, password, and server address
for your MongoDB server.

Initialize the MongoClient with more options
--------------------------------------------

All options/parameters of the MongoClient
[constructor](https://pymongo.readthedocs.io/en/stable/api/pymongo/mongo_client.html#pymongo.mongo_client.MongoClient)
can be passed via the dlite storage parameters ```options```, like shown below.

```python
>>> import dlite
>>> from dlite.options import Options
>>> options = Options(f"mode=w,database={database},collection={collection}")
>>> options.update(connect=False, maxPoolSize=200)
>>> storage = dlite.Storage("mongodb", uri, str(options))
```

Connect to Azure MongoDB
------------------------

In the [Azure portal](https://portal.azure.com/), create an **Azure CosmosDB for MongoDB** account, then open the new account, and open the menu Settings > Connection strings.
In the **Connection strings** page, show and copy to the clipboard the **PRIMARY CONNECTION STRING**. Then paste the string to a *.env* file.

```sh
# ".env" file
AZURE_MONGODB=mongodb://...
```

Then, in your Python code, load the connection string and use it to create a dlite storage.

```python
>>> from dotenv import dotenv_values
>>> config = dotenv_values(".env")
>>> options = "mode=w,database=test-database,collection=test-collection"
>>> storage = dlite.Storage("mongodb", config["AZURE_MONGODB"], options)
```
