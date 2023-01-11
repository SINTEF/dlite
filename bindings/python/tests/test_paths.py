#!/usr/bin/env python
from pathlib import Path

import dlite

print("dlite storage paths:")
for path in dlite.storage_path:
    print("- " + path)


print()
print("append path with glob pattern:")
thisdir = Path(__file__).parent.absolute()
dlite.storage_path.append(f"{thisdir}/*.json")
for path in dlite.storage_path:
    print("- " + path)

print()
print("delete second last path:")
del dlite.storage_path[-2]
for path in dlite.storage_path:
    print("- " + path)

print()
print("Predefined paths:")
for (k, v) in dlite.__dict__.items():
    if k.endswith("path"):
        print(f"dlite.{k}='{v}'")
