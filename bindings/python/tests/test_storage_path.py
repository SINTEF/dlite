#!/usr/bin/env python
from pathlib import Path

# Configure paths
thisdir = Path(__file__).parent.absolute()

dlite.storage_path.append(entitydir / '*.txt')
dlite.storage_path.append(entitydir / '*.json')
Person = dlite.get_instance('http://onto-ns.com/meta/0.1/Person')
