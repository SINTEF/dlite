import dlite


# Create two entities with ref properties that cyclic refer to each other...
prop_a = dlite.Property("a", "ref", "http://onto-ns.com/meta/0.1/A")
B = dlite.Metadata(
    "http://onto-ns.com/meta/0.1/B", dimensions=[], properties=[prop_a],
)

prop_v = dlite.Property("v", "int")
prop_b = dlite.Property("b", "ref", "http://onto-ns.com/meta/0.1/B")
A = dlite.Metadata(
    "http://onto-ns.com/meta/0.1/A", dimensions=[], properties=[prop_v, prop_b],
)

# Create instances
b = B()
a = A(properties={"v": 3, "b": b})
b.a = a


assert a.v == 3
assert a.b == b
assert b.a == a
