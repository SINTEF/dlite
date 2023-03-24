from pathlib import Path

import dlite


thisdir = Path(__file__).resolve().parent

dlite.storage_path.append(thisdir / "test_ref_type.json")

Top = dlite.get_instance("http://onto-ns.com/meta/0.1/Top")
Middle = dlite.get_instance("http://onto-ns.com/meta/0.1/Middle")
Leaf = dlite.get_instance("http://onto-ns.com/meta/0.1/Leaf")
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



tree0 = Tree(dimensions=[0])
tree1 = Tree(dimensions=[2], properties={"subtree": [tree0, tree0]})
tree2 = Tree(dimensions=[1], properties={"subtree": [tree1]})

cyclic = Tree(dimensions=[2])
cyclic.subtree = [tree0, cyclic]
