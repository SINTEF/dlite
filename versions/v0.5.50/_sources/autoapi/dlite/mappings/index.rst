:py:mod:`dlite.mappings`
========================

.. py:module:: dlite.mappings

.. autoapi-nested-parse::

   Implements mappings between entities.

   Units are currently handled with pint.Quantity.  The benefit of this
   compared to explicit unit conversions, is that units will be handled
   transparently by mapping functions, without any need to specify units
   of input and output parameters.

   Shapes are automatically handled by expressing non-scalar quantities
   with numpy.



Module Contents
---------------


Functions
~~~~~~~~~

.. autoapisummary::

   dlite.mappings.instance_routes
   dlite.mappings.instantiate_from_routes
   dlite.mappings.instantiate
   dlite.mappings.instantiate_all



.. py:function:: instance_routes(meta, instances, triplestore, allow_incomplete = False, quantity = Quantity, **kwargs)

   Find all mapping routes for populating an instance of `meta`.

   :param meta: Metadata for the instance we will create.
   :param instances: sequence of instances that the new instance will be
                     populated from.
   :param triplestore: Triplestore containing the mappings.
   :param allow_incomplete: Whether to allow not populating all properties
                            of the returned instance.
   :param quantity: Class implementing quantities with units.  Defaults to
                    pint.Quantity.
   :param kwargs: Keyword arguments passed to mapping_routes().

   :returns: A dict mapping property names to a MappingStep instance.


.. py:function:: instantiate_from_routes(meta, routes, routedict = None, id = None, default = None, quantity = Quantity)

   Create a new instance of `meta` from selected mapping route returned
   by instance_routes().

   :param meta: Metadata to instantiate.
   :param routes: Dict returned by instance_routes().  It should map property
                  names to MappingStep instances.
   :param routedict: Dict mapping property names to route number to select for
                     the given property.  The default is to select the route with
                     lowest cost.
   :param id: URI of instance to create.
   :param default: A dlite instance with default values for unassigned
                   properties.
   :param quantity: Class implementing quantities with units.  Defaults to
                    pint.Quantity.

   :returns: New instance.


.. py:function:: instantiate(meta, instances, triplestore, routedict = None, id = None, allow_incomplete = False, default = None, quantity = Quantity, **kwargs)

   Create a new instance of `meta` populated with the selected mapping
   routes.

   This is a convenient function that combines instance_routes() and
   instantiate_from_routes().  If you want to investigate the possible
   routes, you will probably want to call instance_routes() and
   instantiate_from_routes() instead.

   :param meta: Metadata to instantiate.
   :param instances: Sequence of instances with source values.
   :param triplestore: Triplestore instance.
                       It is safe to pass a generator expression too.
   :param routedict: Dict mapping property names to route number to select for
                     the given property.  The default is to select the route with
                     lowest cost.
   :param id: URI of instance to create.
   :param allow_incomplete: Whether to allow not populating all properties
                            of the returned instance.
   :param default: A dlite instance with default values for unassigned
                   properties.  Implies `allow_incomplete=True`.
   :param quantity: Class implementing quantities with units.  Defaults to
                    pint.Quantity.

   :returns: New instance.


.. py:function:: instantiate_all(meta, instances, triplestore, routedict = None, allow_incomplete = False, default = None, quantity = Quantity, **kwargs)

   Like instantiate(), but returns a generator over all possible instances.

   The number of instances iterated over is the product of the number
   routes for each property and may potentially be very large.  Use
   `routedict` to reduct the number.

   :param meta: Metadata to instantiate.
   :param instances: Sequence of instances with source values.
   :param triplestore: Triplestore instance.
                       It is safe to pass a generator expression too.
   :param routedict: Dict mapping property names to route number to select for
                     the given property.  For property names not in `routedict`, the
                     default is to iterate over all possible routes.  Set the value to
                     None to only consider the route with lowest cost.
   :param allow_incomplete: Whether to allow not populating all properties
                            of the returned instance.
   :param default: A dlite instance with default values for unassigned
                   properties.  Implies `allow_incomplete=True`.
   :param quantity: Class implementing quantities with units.  Defaults to
                    pint.Quantity.

   :Yields: New instances.


