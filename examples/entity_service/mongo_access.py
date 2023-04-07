"""Load instances directly from the mongo database."""
import os

import dlite


user = os.environ["USER"]
options = f"database=dlite;collection=entities;user={user};password={user}"


# Ex1: Access from storage
with dlite.Storage("mongodb", "localhost:27017", f"{options};mode=r") as s:
    Energy = s.load("http://onto-ns.com/meta/0.1/Energy")
assert Energy
#print(Energy)


# Ex2: Access from location
Forces = dlite.Instance.from_location(
    driver="mongodb",
    location="localhost:27017",
    options=f"{options};mode=r",
    id="http://onto-ns.com/meta/0.1/Forces",
    #id="http://onto-ns.com/meta/0.1/Reaction",
)
assert Forces
#print(Forces)


# Ex3: Access from url
Reaction = dlite.Instance.from_url(
    f"mongodb://localhost:27017"
    f"?user={user};password={user};collection=entities"
    f"#http://onto-ns.com/meta/0.1/Reaction",
)
assert Reaction
#print(Reaction)


# Ex4: Appending to dlite.storage_path
#dlite.storage_path.append(
#    f"mongodb://localhost:27017"
#    f"?user={user};password={user};collection=entities"
#    f"#*"
#)
print()
print(dlite.storage_path)
print()
CalcResult= dlite.get_instance("http://onto-ns.com/meta/0.1/CalcResult")
print(CalcResult)


#from pymongo import MongoClient
#client = MongoClient("localhost:27017", username=user, password=user)
#db = client["dlite"]
#collection = db["entities"]



#http://onto-ns.com/meta/0.1/Reaction
#http://onto-ns.com/meta/0.1/CalcResult
#http://onto-ns.com/meta/0.1/Person
#http://onto-ns.com/meta/0.1/Substance
