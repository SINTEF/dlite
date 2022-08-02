from dlite.triplestore import en, Literal, Triplestore, RDF, RDFS, XSD, OWL
from dlite.triplestore.triplestore import function_id


# Test namespaces
# ---------------
assert str(RDF) == "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
assert RDF.type == "http://www.w3.org/1999/02/22-rdf-syntax-ns#type"


# Test RDF literals
# -----------------
l1 = Literal("Hello world!")
assert l1 == "Hello world!"
assert isinstance(l1, str)
assert l1.lang is None
assert l1.datatype is None
assert l1.to_python() == "Hello world!"
assert l1.value == "Hello world!"
assert l1.n3() == '"Hello world!"'

l2 = Literal("Hello world!", lang="en")
assert l2.lang == "en"
assert l2.datatype == None
assert l2.value == "Hello world!"
assert l2.n3() == '"Hello world!"@en'

l3 = en("Hello world!")
assert l3.n3() == '"Hello world!"@en'

l4 = Literal(42)
assert l4.lang == None
assert l4.datatype == XSD.integer
assert l4.value == 42
assert l4.n3() == f'"42"^^{XSD.integer}'

l5 = Literal(42, datatype=float)
assert l5.lang == None
assert l5.datatype == XSD.double
assert l5.value == 42.0
assert l5.n3() == f'"42"^^{XSD.double}'


# Test rdflib triplestore backend
# -------------------------------
ts = Triplestore("rdflib")
assert ts.expand_iri("xsd:integer") == XSD.integer
assert ts.prefix_iri(RDF.type) == 'rdf:type'
EX = ts.bind("ex", "http://example.com/onto#")
assert str(EX) == "http://example.com/onto#"
ts.add_mapsTo(EX.MyConcept, "http://onto-ns.com/meta/0.1/MyEntity", "myprop")
ts.add((EX.MyConcept, RDFS.subClassOf, OWL.Thing))
ts.add((EX.AnotherConcept, RDFS.subClassOf, OWL.Thing))
ts.add((EX.Sum, RDFS.subClassOf, OWL.Thing))

def sum(a, b):
    """Summarise `a` and `b`."""
    return a + b

ts.add_function(sum, expects=(EX.MyConcept, EX.AnotherConcept), returns=EX.Sum,
                base_iri=EX)

s = ts.serialize(format="turtle")
fid = function_id(sum)
assert s == f"""\
@prefix dcterms: <http://purl.org/dc/terms/> .
@prefix ex: <http://example.com/onto#> .
@prefix fno: <https://w3id.org/function/ontology#> .
@prefix map: <http://emmo.info/domain-mappings#> .
@prefix owl: <http://www.w3.org/2002/07/owl#> .
@prefix rdf: <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .
@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .

ex:sum_{fid} a fno:Function ;
    dcterms:description "Summarise `a` and `b`."@en ;
    fno:expects ( ex:sum_{fid}_parameter1_a ex:sum_{fid}_parameter2_b ) ;
    fno:returns ( ex:sum_{fid}_output1 ) .

<http://onto-ns.com/meta/0.1/MyEntity#myprop> map:mapsTo ex:MyConcept .

ex:AnotherConcept rdfs:subClassOf owl:Thing .

ex:Sum rdfs:subClassOf owl:Thing .

ex:sum_{fid}_output1 a fno:Output ;
    map:mapsTo ex:Sum .

ex:sum_{fid}_parameter1_a a fno:Parameter ;
    rdfs:label "a"@en ;
    map:mapsTo ex:MyConcept .

ex:sum_{fid}_parameter2_b a fno:Parameter ;
    rdfs:label "b"@en ;
    map:mapsTo ex:AnotherConcept .

ex:MyConcept rdfs:subClassOf owl:Thing .

"""

ts2 = Triplestore("rdflib")
ts2.parse(format="turtle", data=s)
assert ts2.serialize(format="turtle") == s
ts2.set((EX.AnotherConcept, RDFS.subClassOf, EX.MyConcept))
# print(ts2.serialize(format="turtle"))
