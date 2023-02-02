Mongodb Storage plugin
===============

Quick Guide
-------

This is a quickguide for users wishing to connect to a mongodb atlas server. It is assumed that they have access 
to a server and have their own username and password. 

```python
    >>> user = "USERNAME"
    >>> password = "PASSWORD"
    >>> server = "MYSERVERADDRESS"
    >>> uri = f"mongodb+srv://{server}"
```
With this information one can then create a dlite storage instance. 

```python
    >>> import dlite
    >>> storage = dlite.Storage(
    >>>     'mongodb', uri,
    >>>     options=f'user={user};password={password}'
    >>> )
```

This storage instance can then be used like any other dlite storage instance.
For example saving and loading instances: 

```python
    >>> inst1 = dlite.Instance.from_location("json", "MY/INSTANCE/PATH.JSON")
    >>> storage.save(inst1)
    >>> storage.load(inst1.uuid)
```