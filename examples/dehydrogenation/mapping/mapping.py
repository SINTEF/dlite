#!/usr/bin/env python3
from rdflib import Graph, Namespace, URIRef, BNode, Literal
from rdflib.namespace import RDF, FOAF, XSD

import dlite


map = Namespace('http://onto-ns.com/ontology/dehydrogenation/mapping#')
cr = Namespace('http://onto-ns.com/ontology/dehydrogenation/chemical-reaction#')
emmo = Namespace('http://emmo.info/emmo#')
mo = Namespace('http://emmo.info/domain-mappings#')

g = Graph()
g.bind('map', map)
g.bind('cr', cr)
g.bind('emmo', emmo)
g.bind('mo', mo)

g.add((URIRef('http://onto-ns.com/meta/0.1/Molecule'), mo.MapsTo, emmo.Molecule))

prefixes = {
    'emmo': 'http://emmo.info/emmo#',
    'mo': 'http://emmo.info/domain-mappings#',
    'cr': 'http://onto-ns.com/ontology/dehydrogenation/chemical-reaction#',
}

mappings = [
    # Mapping metadata
    ('http://onto-ns.com/meta/0.1/Molecule', 'mo:MapsTo', 'emmo:Molecule'),
    ('http://onto-ns.com/meta/0.1/Reaction', 'mo:MapsTo', 'cr:ChemicalReaction'),
    # Mapping properties
    ('http://onto-ns.com/meta/0.1/Molecule#energy', 'mo:MapsTo',
     'emmo:PotentialEnergy'),
    ('http://onto-ns.com/meta/0.1/Reaction#reactants', 'mo:MapsTo',
     'cr:ChemicalReactant'),
    ('http://onto-ns.com/meta/0.1/Reaction#products', 'mo:MapsTo',
     'cr:ChemicalProduct'),
    ('http://onto-ns.com/meta/0.1/Reaction#energy', 'mo:MapsTo',
     'cr:ReactionEnergy'),
    # Mapping instances
    ('b1108f07-e09e-57e0-920b-7701ebaaa077', 'mo:MapsTo', 'cr:ethan'),
    ('de5fd264-b03d-54ca-8156-81f4489e2fe9', 'mo:MapsTo', 'cr:ethen'),
    ('8a986375-22d2-5c2e-aa55-1f07567f4524', 'mo:MapsTo', 'cr:hydrogen'),
]
