:py:mod:`dlite.quantity`
========================

.. py:module:: dlite.quantity

.. autoapi-nested-parse::

   Define the singleton QuantityHelper to work with pint Quantity and dlite
   instance properties.



Module Contents
---------------

Classes
~~~~~~~

.. autoapisummary::

   dlite.quantity.QuantityHelper



Functions
~~~~~~~~~

.. autoapisummary::

   dlite.quantity.get_quantity_helper



Attributes
~~~~~~~~~~

.. autoapisummary::

   dlite.quantity.pint
   dlite.quantity.DLITE_QUANTITY_TYPES
   dlite.quantity.quantity_helper


.. py:data:: pint
   

   

.. py:data:: DLITE_QUANTITY_TYPES
   :annotation: = ['int', 'float', 'double', 'uint', 'int8', 'int16', 'int32', 'int64', 'uint8', 'uint16',...

   

.. py:data:: quantity_helper
   :annotation: = None

   

.. py:class:: QuantityHelper

   Singleton class for working with pint Quantity and dlite instance
   properties.

   .. py:property:: unit_registry
      :type: pint

      Returns the current pint UnitRegistry object

   .. py:method:: __getitem__(name)


   .. py:method:: __setitem__(name, value)


   .. py:method:: __getattr__(name)


   .. py:method:: __setattr__(name, value)


   .. py:method:: __call__(inst)


   .. py:method:: names()

      Return the names of each property which has a quantity type


   .. py:method:: units()

      Return the units of each property which has a quantity type


   .. py:method:: values()

      Return the quantity of each property which has a quantity type


   .. py:method:: items()

      Return a zip iterator (name, quantity)


   .. py:method:: get(*names)

      Return the quantity of each given property name


   .. py:method:: quantity(magnitude, unit)

      Return a pint.Quantity object


   .. py:method:: parse(value)

      Parse the given value and return a pint.Quantity object


   .. py:method:: parse_expression(value)

      Parse an expression (str) and return a pint.Quantity object


   .. py:method:: update(**kwargs)


   .. py:method:: to_dict(names=None, value_type='quantity', fmt='')



.. py:function:: get_quantity_helper(instance)


