import dlite


# Create two entities (with ref properties) that cyclically refer to each other.
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
inst_b = B()
inst_a = A(properties={"v": 3, "b": inst_b})
inst_b.a = inst_a


assert inst_a.v == 3
assert inst_a.b == inst_b
assert inst_b.a == inst_a
