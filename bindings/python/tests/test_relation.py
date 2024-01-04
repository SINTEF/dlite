import dlite
from dlite.testutils import raises


rel1 = dlite.Relation("s1", "p1", "o1")
assert (rel1.s, rel1.p, rel1.o) == ("s1", "p1", "o1")
assert rel1.d == None

rel2 = dlite.Relation("s2", "p2", "o2", "d2")
assert (rel2.s, rel2.p, rel2.o) == ("s2", "p2", "o2")
assert rel2.d == "d2"

rel = dlite.Relation("s", "p", "3")
with raises(TypeError):
    rel = dlite.Relation("s", "p", 3)

with raises(TypeError):
    rel = dlite.Relation("s", "p", "o", "d", "id", "non-expected-argument")
