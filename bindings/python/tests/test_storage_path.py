#!/usr/bin/env python
from pathlib import Path

import dlite


# Configure paths
thisdir = Path(__file__).parent.absolute()

dlite.storage_path.append(thisdir / '*.txt')
dlite.storage_path.append(thisdir / '*.yaml')
dlite.storage_path.append(thisdir / '*.json')
Person = dlite.get_instance('http://onto-ns.com/meta/0.1/Person')
print(Person)
