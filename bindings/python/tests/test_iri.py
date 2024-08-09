#!/usr/bin/env python
from pathlib import Path

import dlite


thisdir = Path(__file__).parent.absolute()
entitydir = thisdir / "entities"
dlite.storage_path.append(f"{entitydir}/*.json")

E = dlite.Instance.from_url(url=f"json://{entitydir}/MyEntity.json?mode=r")
E.iri = "http://emmo.info/emmo/EMMO_Physical"
E.iri = None
E.iri = "http://emmo.info/emmo/EMMO_Physical"

e = E([3, 4])
e.iri = "abc"

p = E["properties"][3]
p.iri = "http://emmo.info/emmo/EMMO_Length"
