Mongodb Storage plugin
===============

Quick Guide
-------

This is a quickguide for users wishing to connect to a mongodb atlas server. It is assumed that they have access 
to a server and have their own username and password.

First, set up the connection information:

```python
    >>> user = "USERNAME"
    >>> password = "PASSWORD"
    >>> server = "MYSERVERADDRESS"
    >>> uri =  f"mongodb://{server}"
```
With this information one can then create a dlite storage instance. 

```python
    >>> import dlite
    >>> storage = dlite.Storage(
    >>>     'mongodb', uri,
    >>>     options=f'user={user};password={password};schema=mongodb+srv'
    >>> )
```

This storage instance can then be used like any other dlite storage instance.
For example saving and loading instances: 

```python
    >>> inst1 = dlite.Instance.from_location("json", "MY/INSTANCE/PATH.JSON")
    >>> storage.save(inst1)
    >>> storage.load(inst1.uuid)
```

Replace "MY/INSTANCE/PATH.JSON" with the path to your instance file, and make sure to use the correct username, password, and server address for your MongoDB Atlas server.