"""Example where we add the mongodb database to `dlite.storage_path`.
After that we can access all stored instances with `dlite.get_instance()`.
"""
import os

import dlite

user = os.environ["USER"]
options = f"database=dlite;collection=entities;user={user};password={user}"
ns = "http://onto-ns.com/meta"

dlite.storage_path.append(f"mongodb://localhost:27017?{options};mode=r")

Molecule = dlite.get_instance(f"{ns}/0.1/Molecule")
Forces = dlite.get_instance(f"{ns}/0.1/Forces")
Substance = dlite.get_instance(f"{ns}/0.1/Substance")
print(f"Forces UUID: {Forces.uuid}")
