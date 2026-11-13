:py:mod:`dlite.table`
=====================

.. py:module:: dlite.table

.. autoapi-nested-parse::

   Utilities for reading and writing DLite instances from and to tables.



Module Contents
---------------

Classes
~~~~~~~

.. autoapisummary::

   dlite.table.DMTable



Functions
~~~~~~~~~

.. autoapisummary::

   dlite.table.csvsniff



Attributes
~~~~~~~~~~

.. autoapisummary::

   dlite.table.HAVE_TRIPPER
   dlite.table.DEFAULT_DATAMODEL_MAPPINGS
   dlite.table.DEFAULT_PROPERTY_MAPPINGS


.. py:data:: HAVE_TRIPPER
   :annotation: = False

   

.. py:data:: DEFAULT_DATAMODEL_MAPPINGS
   

   

.. py:data:: DEFAULT_PROPERTY_MAPPINGS
   

   

.. py:class:: DMTable(table, datamodel_mappings = DEFAULT_DATAMODEL_MAPPINGS, property_mappings = DEFAULT_PROPERTY_MAPPINGS, baseuri = None, unit_handling = 'raise')

   A class for loading DLite data models form tables.

   A table with datamodels should contain some columns with pre-defined
   header labels. For details, see the [online documentation].

   References:
   [online documentation]: https://sintef.github.io/dlite/user_guide/datamodels.html#defining-multiple-datamodels-in-a-table

   .. py:attribute:: dmdicts
      

      

   .. py:attribute:: datamodel_mappings
      

      

   .. py:attribute:: property_mappings
      

      

   .. py:method:: get_datamodels()

      Return a list with all datamodels parsed from the table.


   .. py:method:: from_csv(csvfile, encoding = 'utf-8', dialect = None, datamodel_mappings = DEFAULT_DATAMODEL_MAPPINGS, property_mappings = DEFAULT_PROPERTY_MAPPINGS, baseuri = None, unit_handling = 'raise', **kwargs)
      :staticmethod:

      Parse a csv file using the standard library csv module.

      :param csvfile: Name of CSV file to parse or an iterable of strings.
      :param encoding: The encoding of the csv file.  Note that Excel may
                       encode as "ISO-8859" (which was commonly used in the 1990s).
      :param dialect: A subclass of csv.Dialect, or the name of the dialect,
                      specifying how the `csvfile` is formatted.  For more details,
                      see [Dialects and Formatting Parameters].
      :param datamodel_mappings: Mapping of DLite datamodel fields (uri,
                                 dimensions, description) to table header names. 'dimensions'
                                 is normally not provided, in which case it will be inferred
                                 from `property_mappings`.
      :param property_mappings: Mapping of DLite property fields (name, type,
                                ref, unit, shape, description) to table header names.
      :param baseuri: Base URI to use if the identifier has no namespace.
                      See the corresponding argument of the __init__() method for
                      details.
      :param unit_handling: How to handle units that are not found in the EMMO
                            ontology.  Must be one of:
                            - 'raise': Raise a MissingUnit exception (default)
                            - 'ignore': Don't add the unit to the property.  Will issue
                              a warning.
                            - 'force': Add the unit to the property. Will issue a warning.
      :param kwargs: Additional keyword arguments overriding individual
                     formatting parameters.  For more details, see
                     [Dialects and Formatting Parameters].

      :returns: New DMTable instance.

      References:
      [Dialects and Formatting Parameters]: https://docs.python.org/3/library/csv.html#dialects-and-formatting-parameters


   .. py:method:: from_excel(excelfile, sheet = 0, cellrange = None, datamodel_mappings = DEFAULT_DATAMODEL_MAPPINGS, property_mappings = DEFAULT_PROPERTY_MAPPINGS, baseuri = None, unit_handling = 'raise')
      :staticmethod:

      Parse a csv file using the standard library csv module.

      :param excelfile: Name of excel file to parse.
      :param sheet: Sheet name or number to load.
      :param cellrange: Cell range to load. Examples: "A1:C4", "A:C", "1:4".
                        The default is to read all cells.
      :param dialect: A subclass of csv.Dialect, or the name of the dialect,
                      specifying how the `csvfile` is formatted.  For more details,
                      see [Dialects and Formatting Parameters].
      :param datamodel_mappings: Mapping of DLite datamodel fields (uri,
                                 dimensions, description) to table header names. 'dimensions'
                                 is normally not provided, in which case it will be inferred
                                 from `property_mappings`.
      :param property_mappings: Mapping of DLite property fields (name, type,
                                ref, unit, shape, description) to table header names.
      :param baseuri: Base URI to use if the identifier has no namespace.
                      See the corresponding argument of the __init__() method for
                      details.
      :param unit_handling: How to handle units that are not found in the EMMO
                            ontology.  Must be one of:
                            - 'raise': Raise a MissingUnit exception (default)
                            - 'ignore': Don't add the unit to the property.  Will issue
                              a warning.
                            - 'force': Add the unit to the property. Will issue a warning.

      :returns: New DMTable instance.


   .. py:method:: to_triplestore(ts)

      Save all datamodels to Tripper triplestore `ts`.



.. py:function:: csvsniff(sample)

   Custom csv sniffer.

   Analyse csv sample and returns a csv.Dialect instance.


