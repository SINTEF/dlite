import os
import sys
from pathlib import Path

import dlite


user = os.environ["USER"]
options = f"database=dlite;collection=entities;user={user};password={user}"
thisdir = Path(__file__).resolve().parent


try:
    with dlite.silent:
        Energy = dlite.Instance.from_location(
            "mongodb", "localhost:27017", options=f"{options};mode=r",
            id="http://onto-ns.com/meta/0.1/Energy"
        )
except dlite.DLiteError:
    pass
else:
    print("MongoDB already populated")
    sys.exit()


print("Populating MongoDB...")
entity_paths = [
    "examples/ex1/Chemistry-0.1.json",
    "examples/dehydrogenation/entities/Molecule.json",
    "examples/dehydrogenation/entities/Reaction.json",
    "examples/dehydrogenation/entities/Substance.json",
    "examples/storages/Person.json",
    "examples/mappings/entities/CalcResult.json",
    "examples/mappings/entities/Energy.json",
    "examples/mappings/entities/Forces.json",
]
with dlite.Storage("mongodb", "localhost:27017", f"{options};mode=w") as s:
    for entity_path in entity_paths:
        path = Path(thisdir / "../.." / entity_path)
        inst = dlite.Instance.from_location("json", path, "mode=r")
        s.save(inst)
