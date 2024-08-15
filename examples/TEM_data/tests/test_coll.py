from paths import outdir

import dlite


coll = dlite.Collection()
coll.add_relation("a", "b", "c");
coll.add_relation("d", "e", "\"f\"");
coll.save("json", outdir / "xxx.json", "mode=w");

del coll
coll = dlite.Instance.from_location("json", outdir / "xxx.json")

print(coll)
assert coll.relations[1].o == '"f"'
