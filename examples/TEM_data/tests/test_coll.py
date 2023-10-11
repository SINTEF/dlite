from paths import outdir

import dlite


coll = dlite.Collection()
coll.add_relation("a", "b", None);
coll.add_relation("e", "e", "\"f\"");
coll.save("json", outdir / "xxx.json", "mode=w");

del coll
coll = dlite.Instance.from_location("json", outdir / "xxx.json")
print(coll)
print("***", coll.relations[1].o[0]);
