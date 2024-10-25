from pathlib import Path

import dlite
from dlite.testutils import importcheck

yaml = importcheck("yaml")


thisdir = Path(__file__).resolve().parent
indir = thisdir / "input"

dlite.storage_path.append(indir)
dlite.storage_path.append(indir/"test_ref_type_middle.yaml")

# If yaml is available, we read Middle v0.2, which is defined in
# `test_ref_type_middle.yaml`.  Otherwise, we read Middle v0.1, which
# is defined together with the other datamodels in `test_ref_type.json`.
version = "0.2" if yaml else "0.1"

Top = dlite.get_instance("http://onto-ns.com/meta/0.1/Top")
Middle = dlite.get_instance(f"http://onto-ns.com/meta/{version}/Middle")
Leaf = dlite.get_instance("http://onto-ns.com/meta/0.1/Leaf")
Linked = dlite.get_instance("http://onto-ns.com/meta/0.1/Linked")
Tree = dlite.get_instance("http://onto-ns.com/meta/0.1/Tree")

leaf1 = Leaf(properties={"a": 3, "b": True})
leaf2 = Leaf(properties={"a": 5, "b": False})

middle1 = Middle(properties={"name": "leaf1", "leaf": leaf1})
middle2 = Middle(properties={"name": "leaf2", "leaf": leaf2})

top = Top(dimensions=[2], properties={"middles": [middle1, middle2]})

assert top.middles[0].name == "leaf1"
assert top.middles[0].leaf.a == 3
assert top.middles[0].leaf.b == True
assert top.middles[1].name == "leaf2"
assert top.middles[1].leaf.a == 5
assert top.middles[1].leaf.b == False

# Test unseting ref
middle1.leaf = None
assert middle1.leaf is None

# Test assigning from uuid
middle1.leaf = leaf1.uuid
assert middle1.leaf == leaf1
assert middle1.leaf.a == 3

# Create a linked list
item0 = Linked()
item1 = Linked(properties={"next": item0})
item2 = Linked(properties={"next": item1})
assert item2.next == item1
assert item2.next.next == item0

# Convert to a cyclic list
item0.next = item2
assert item1.next.next.next == item1  # one cycle

# Create from json
inst = dlite.Instance.from_json(
    f"""
{{
  "meta": "http://onto-ns.com/meta/0.1/Linked",
  "dimensions": {{}},
  "properties": {{
    "next": "{item0.uuid}"
  }}
}}
"""
)
assert inst.next == item0
assert inst.next.next.next.next == item0

# Create a tree structure
tree0 = Tree(dimensions=[0])
tree1 = Tree(dimensions=[2], properties={"subtree": [tree0, tree0]})
tree2 = Tree(dimensions=[1], properties={"subtree": [tree1]})
assert tree2.subtree[0].subtree[0] == tree0
assert tree2.subtree[0].subtree[1] == tree0

# Create a cyclic tree structure
cyclic = Tree(dimensions=[1])
cyclic.subtree = [cyclic]
assert cyclic.subtree[0] == cyclic
assert cyclic.subtree[0].subtree[0] == cyclic
assert cyclic.subtree[0].subtree[0].subtree[0] == cyclic

# For isue #982: ref-type in yaml
assert Middle.getprop("leaf").ref == "http://onto-ns.com/meta/0.1/Leaf"

# For issue #515: Instantiate nested from dict
#middle = Middle(properties={"name": "nested", "leaf": {"a": 1, "b": True}})
