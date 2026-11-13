:py:mod:`dlite.options`
=======================

.. py:module:: dlite.options

.. autoapi-nested-parse::

   A module for handling options passed to storage plugins,
   converting between valid percent-encoded URL query strings
   and dicts.

   A URL query string is a string of key-value pairs separated by either semicolon (;) or ampersand (&).
   For example

       key1=value1;key2=value2...

   or

       key1=value1&key2=value2...

   where the keys and and values are percent-encoded.

   Percent-encoding means that all characters that are digits, letters or
   one of "~._-" are encoded as-is, while all other are encoded as their
   unicode byte number in hex with each byte preceeded "%". For example
   "a" would be encoded as "a", "+" would be encoded as "%2B" and "å" as
   "%C3%A5".

   In DLite, a value can also start with "%%", which means that the rest of the value is assumed to be a percent-encoded json string.
   This addition makes it possible to pass any kind of json-serialisable data structures as option values.



Module Contents
---------------

Classes
~~~~~~~

.. autoapisummary::

   dlite.options.Options



Functions
~~~~~~~~~

.. autoapisummary::

   dlite.options.parse_query
   dlite.options.make_query



.. py:class:: Options(options, defaults=None)

   Bases: :py:obj:`dict`

   A dict representation of the options string `options` with
   attribute access.

   :param options: Percent-encoded URL query string or dict.
                   The options to represent.
   :param defaults: Percent-encoded URL query string or dict.
                    Default values for options.

   .. py:method:: __getattr__(name)


   .. py:method:: __setattr__(name, value)

      Implement setattr(self, name, value).


   .. py:method:: __str__()

      Return str(self).



.. py:function:: parse_query(query)

   Parse URL query string `query` and return a dict.


.. py:function:: make_query(d, sep=';')

   Returns an URL query string from dict `d`.


