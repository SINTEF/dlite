:py:mod:`dlite.datamodel`
=========================

.. py:module:: dlite.datamodel

.. autoapi-nested-parse::

   Module that simplifies creating a data model.



Module Contents
---------------

Classes
~~~~~~~

.. autoapisummary::

   dlite.datamodel.DataModelError
   dlite.datamodel.MissingDimensionError
   dlite.datamodel.UnusedDimensionError
   dlite.datamodel.DataModel




.. py:class:: DataModelError

   Bases: :py:obj:`dlite.DLiteError`

   Raised if the datamodel is inconsistent.


.. py:class:: MissingDimensionError

   Bases: :py:obj:`DataModelError`

   Raised if a dimension referred to in a property is not defined.


.. py:class:: UnusedDimensionError

   Bases: :py:obj:`DataModelError`

   Raised if a dimension is not referred to in any property.


.. py:class:: DataModel(uri, schema=None, description=None)

   Class for creating a data model.

   .. py:attribute:: uri
      

      

   .. py:attribute:: description
      :annotation: = None

      

   .. py:attribute:: dimensions
      

      

   .. py:attribute:: properties
      

      

   .. py:attribute:: relations
      :annotation: = []

      

   .. py:attribute:: schema
      

      

   .. py:method:: add_dimension(name, description)

      Add dimension with given `name` and description to data model.


   .. py:method:: add_property(name, type, shape=None, unit=None, description=None, dims=None)

      Add property to data model.

      :param name: Property label.
      :param type: Property type.
      :param shape: Shape of Property.  Default is scalar.
      :param unit: Unit. Default is dimensionless.
      :param description: Human description.
      :param dims: Deprecated alias for `shape`.


   .. py:method:: get_missing_dimensions()

      Returns a set of dimension names referred to in property shapes but
      is not in dimensions.


   .. py:method:: get_unused_dimensions()

      Returns a set of dimensions not referred to in any property shapes.



   .. py:method:: validate()

      Raises an exception if there are missing or unused dimensions.


   .. py:method:: get()

      Returns a DLite Metadata created from the datamodel.



