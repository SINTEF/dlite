:py:mod:`dlite.factory`
=======================

.. py:module:: dlite.factory

.. autoapi-nested-parse::

   A module that makes it easy to add dlite functionality to existing classes.

   Class customisations
   --------------------
   In order to handle special cases, the following methods may be
   defined/overridden in the class that is to be extended:

   ```python
   _dlite_get_<name>(self, name)
   _dlite_set_<name>(self, name, value)
   _dlite__new__(cls, inst)
   ```



Module Contents
---------------

Classes
~~~~~~~

.. autoapisummary::

   dlite.factory.MetaExtension
   dlite.factory.BaseExtension



Functions
~~~~~~~~~

.. autoapisummary::

   dlite.factory.instancefactory
   dlite.factory.objectfactory
   dlite.factory.classfactory



.. py:exception:: FactoryError

   Bases: :py:obj:`Exception`

   Base exception for factory errors.


.. py:exception:: IncompatibleClassError

   Bases: :py:obj:`FactoryError`

   Raised if an extended class is not compatible with its dlite
   metadata.


.. py:class:: MetaExtension(name, bases, attr)

   Bases: :py:obj:`type`

   Metaclass for BaseExtension.


.. py:class:: BaseExtension(*args, instanceid = None, **kwargs)

   Base class for extension.

   Except for `dlite_id`, all arguments are passed further to the `__init__()`
   function of the class we are inheriting from.

   If `instanceid` is given, the id of the underlying dlite instance
   will be set to it.

   .. py:method:: _dlite__new__(inst=None)
      :classmethod:

      Class method returning a new uninitialised instance of the class
      that is extended.

      This method simply returns `cls.__new__(cls)`.

      Override this method if the extended class already defines a
      `__new__()` method.


   .. py:method:: dlite_assign(inst)

      Assigns self from dlite instance `inst`.


   .. py:method:: dlite_load(*args)

      Loads dlite instance from storage and assign self from it.
      The arguments `args` are passed to dlite.Instance.from_storage().



.. py:function:: instancefactory(theclass, inst)

   Returns an extended instance of `theclass` initiated from dlite
   instance ``inst``.

   :param theclass: The class to instantiate an object from using `inst`.
   :param inst: A DLite Instance to use as source for a `theclass` instance
                object.

   :returns: A `theclass` instance object based on the DLite Instance `inst`.


.. py:function:: objectfactory(obj, meta = None, deepcopy = False, cls = None, url = None, storage = None, id = None, instanceid = None)

   Returns an extended copy of `obj`.

   The `url`, `storage`, and `id` arguments are passed to `classfactory()`.

   :param obj: A Python object.
   :param meta: A DLite Metadata Instance.
   :param deepcopy: Whether to perform a deep copy.  Otherwise a shallow copy
                    is performed.
   :param cls: A class to use for the new object. If this is not supplied,
               the new object will be of the same class as the original.
   :param url: If given and `meta` is not given, load metadata from this URL.
               It should be of the form `driver://location?options#id`.
   :param storage: If given and `meta` and `url` are not given, load metadata
                   from this storage.
   :param id: A unique ID referring to the metadata if `storage` is provided.
   :param instanceid: A DLite ID (UUID) to use for the underlying DLite Instance.

   :returns: A new, extended copy of the Python object `obj`.


.. py:function:: classfactory(theclass, meta = None, url = None, storage = None, id = None)

   Factory function that returns a new class that inherits from both
   `theclass` and `BaseExtension`.

   :param theclass: The class to extend.
   :param meta: Metadata instance.
   :param url: If given and `meta` is not given, load metadata from this URL.
               It should be of the form `driver://location?options#id`.
   :param storage: If given and `meta` and `url` are not given, load metadata
                   from this storage.
   :param id: A unique ID referring to the metadata if `storage` is provided.

   :returns: A new class based on `theclass` and `BaseExtensions`.


