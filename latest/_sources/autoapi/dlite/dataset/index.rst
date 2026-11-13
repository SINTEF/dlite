:py:mod:`dlite.dataset`
=======================

.. py:module:: dlite.dataset

.. autoapi-nested-parse::

   Module for representing DLite data models and instances with rdflib.

   DLite data models are represented as EMMO datasets.

   NOTE: This module depends on Tripper.



Module Contents
---------------

Classes
~~~~~~~

.. autoapisummary::

   dlite.dataset.MissingUnitError



Functions
~~~~~~~~~

.. autoapisummary::

   dlite.dataset.title
   dlite.dataset.dlite2emmotype
   dlite.dataset.emmo2dlitetype
   dlite.dataset.get_shape
   dlite.dataset.dimensional_string
   dlite.dataset.get_unit_symbol
   dlite.dataset.is_valid_url
   dlite.dataset.get_unit_iri
   dlite.dataset.metadata_to_rdf
   dlite.dataset.add_dataset
   dlite.dataset.get_dataset
   dlite.dataset.add_data
   dlite.dataset.get_data



Attributes
~~~~~~~~~~

.. autoapisummary::

   dlite.dataset.Triple
   dlite.dataset.TS_EMMO
   dlite.dataset.EMMO_VERSIONIRI
   dlite.dataset.EMMO
   dlite.dataset.Dimension
   dlite.dataset.hasDimension
   dlite.dataset.hasScalarData
   dlite.dataset.EMMO_TYPES
   dlite.dataset.unit_cache


.. py:data:: Triple
   

   

.. py:data:: TS_EMMO
   

   

.. py:data:: EMMO_VERSIONIRI
   

   

.. py:data:: EMMO
   

   

.. py:data:: Dimension
   :annotation: = 'https://w3id.org/emmo#EMMO_b4c97fa0_d82c_406a_bda7_597d6e190654'

   

.. py:data:: hasDimension
   :annotation: = 'https://w3id.org/emmo#EMMO_0a9ae0cb_526d_4377_9a11_63d1ce5b3499'

   

.. py:data:: hasScalarData
   :annotation: = 'https://w3id.org/emmo#EMMO_e5a34647_a955_40bc_8d81_9b784f0ac527'

   

.. py:data:: EMMO_TYPES
   

   

.. py:data:: unit_cache
   

   

.. py:class:: MissingUnitError

   Bases: :py:obj:`dlite.DLiteValueError`

   Unit not found in ontology.


.. py:exception:: UnknownUnitWarning

   Bases: :py:obj:`Warning`

   Unit not found in ontology.


.. py:exception:: UnsupportedTypeError

   Bases: :py:obj:`TypeError`, :py:obj:`NotImplementedError`

   The given type is not supported.


.. py:exception:: KBError

   Bases: :py:obj:`ValueError`

   Missing or inconsistent data in knowledge base.


.. py:function:: title(s)

   Capitalise first letter in `s`.


.. py:function:: dlite2emmotype(dlitetype)

   Convert a DLite type string to corresponding EMMO class label.


.. py:function:: emmo2dlitetype(emmotype, size=None)

   Convert EMMO type and size to dlite type.


.. py:function:: get_shape(ts, dimiri, dimensions=None, mappings=None, uri=None)

   Returns a shape list for a datum who's first dimension is `dimiri`.

   If `dimensions` is given, it should be a list that will be updated
   with new dimensions.

   If `mappings` and `uri` are given, then `mappings` should be a
   list that will be updated with new mappings. `uri` should be the
   URI of the data model.


.. py:function:: dimensional_string(unit_iri)

   Return the inferred dimensional string of the given unit IRI.  Returns
   None if no dimensional string can be inferred.


.. py:function:: get_unit_symbol(iri)

   Return the unit symbol for .


.. py:function:: is_valid_url(url)

   Return whether `url` is a valid URL.


.. py:function:: get_unit_iri(unit)

   Returns the IRI for the given unit.


.. py:function:: metadata_to_rdf(meta, iri = None, mappings = ())

   Serialise DLite metadata to RDF.

   :param meta: Metadata to serialise.
   :param iri: IRI of the dataset in the triplestore. Defaults to `meta.uri`.
   :param mappings: Sequence of mappings of properties to ontological concepts.

   :returns: A list of RDF triples.  Literal objects are encoded in n3 notation.


.. py:function:: add_dataset(ts, meta, iri = None, mappings = ())

   Save DLite metadata as an EMMO dataset to a triplestore.

   :param ts: Triplestore to save to.
   :param meta: DLite metadata to save.
   :param iri: IRI of the dataset in the triplestore. Defaults to `meta.uri`.
   :param mappings: Sequence of mappings of properties to ontological concepts.

   :returns: IRI of the saved dataset.


.. py:function:: get_dataset(ts, iri, uri = None)

   Load dataset from triplestore.

   :param ts: Triplestore to load from.
   :param iri: IRI of the dataset to load.
   :param uri: URI of the DLite datamodel to load. The defaults is inferred
               from `iri`.

   :returns: A `(meta, mappings)` tuple, where `meta` is a DLite metadata and
             `mappings` is a list of triples.


.. py:function:: add_data(ts, inst, iri = None, mappings = ())

   Save DLite instance as an EMMO dataset to a triplestore.

   Data instances are represented as individuals of the corresponding
   EMMO Dataset. The corresponding metadata is also stored if it not
   already exists in the triplestore.

   :param ts: Triplestore to save to.
   :param inst: DLite instance to save.
   :param iri: IRI of the dataset in the triplestore. The default is the
               metadata IRI prepended with a slash and the UUID.
   :param mappings: Sequence of mappings of properties to ontological concepts.

   :returns: IRI of the saved dataset.


.. py:function:: get_data(ts, iri)

   Load dataset from triplestore.

   :param ts: Triplestore to load from.
   :param iri: IRI of the dataset to load.

   :returns: A `(meta, mappings)` tuple, where `meta` is a DLite metadata and
             `mappings` is a list of triples.


