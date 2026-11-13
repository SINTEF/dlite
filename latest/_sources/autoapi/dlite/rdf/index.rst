:py:mod:`dlite.rdf`
===================

.. py:module:: dlite.rdf

.. autoapi-nested-parse::

   Module for representing DLite entities and instances with rdflib.

   It uses the datamodel ontology: https://github.com/emmo-repo/datamodel-ontology



Module Contents
---------------


Functions
~~~~~~~~~

.. autoapisummary::

   dlite.rdf.to_graph
   dlite.rdf.to_rdf
   dlite.rdf.from_graph
   dlite.rdf.from_rdf



Attributes
~~~~~~~~~~

.. autoapisummary::

   dlite.rdf.PUBLIC_ID
   dlite.rdf.DM


.. py:data:: PUBLIC_ID
   :annotation: = 'http://onto-ns.com/data/'

   

.. py:data:: DM
   

   

.. py:function:: to_graph(inst, graph=None, base_uri='', base_prefix=None, include_meta=None)

   Serialise DLite instance to a rdflib Graph object.

   :param inst: Instance to serialise.
   :param graph: A rdflib Graph object that rdf triples for `inst` are added to.
                 If None, a new Graph object is created.
   :param base_uri: Base URI that is prepended to the instance UUID or URI
                    (if it is not already a valid URI).
   :param base_prefix: Optional namespace prefix to use for `base_uri`.
   :param include_meta: Whether to also serialise metadata.  The default
                        is to only include metadata if `inst` is a data object.

   :returns: A rdflib Graph object.


.. py:function:: to_rdf(inst, destination=None, format='turtle', base_uri='', base_prefix=None, include_meta=None, decode=True, **kwargs)

   Serialise DLite instance to string.

   :param inst: Instance to serialise.
   :param destination: File name or file object to serialise `inst` to.
                       If None, the serialisation of `inst` is returned.
   :param format: Format to serialise to.  Build formats: "xml", "n3", "turtle",
                  "nt", "pretty-xml", "trix", "trig", "nquads".
   :param base_uri: Base URI that is prepended to the instance UUID or URI
                    (if it is not already a valid URI).
   :param base_prefix: Optional namespace prefix to use for `base_uri`.
   :param include_meta: Whether to also serialise metadata.  The default
                        is to only include metadata if `inst` is a data object.
   :param decode: Whether to decode the resulting bytes object into a string.
                  For rdflib 6.0 or newer, use the `encoding` option instead.
   :param kwargs: Additional arguments to pass to rdflib.Graph.serialize().

   :returns: The serialised instance if `destination is not None.
             If `decode` is true, a string is returned, otherwise a bytes object.


.. py:function:: from_graph(graph, id=None)

   Instantiate DLite instance with given id from rdflib Graph object.

   If `id` is None and `graph` only contain one instance, that instance
   is returned.  It is an error if `id` is None and `graph` contains more
   than one instances.

   Returns new DLite instance.


.. py:function:: from_rdf(source=None, location=None, file=None, data=None, format=None, id=None, publicID=PUBLIC_ID, **kwargs)

   Instantiate DLite instance from RDF.

   The source is specified using one of `source`, `location`, `file` or `data`.

   :param source: An InputSource, file-like object, or string. In the case
                  of a string the string is the location of the source.
   :param location: A string indicating the relative or absolute URL of the
                    source. Graph's absolutize method is used if a relative location
                    is specified.
   :param file: A file-like object.
   :param data: A string containing the data to be parsed.
   :param format: Used if format can not be determined from source, e.g. file
                  extension or Media Type.  By default, the following formats are
                  supported: "xml", "n3", "turtle", "nt" and "trix".
   :param id: Id of the instance to return.  May be None if the source only
              contain one instance.
   :param publicID: Logical URI to use as document base.
   :param kwargs: Additional keyword arguments passed to rdflib.Graph.parse().

   Returns
       New DLite instance.


