"""Try to use the http storage plugin to access metadata in MongoDB.

Currently it doesn't work...
"""
import dlite



# Manual testing...
import json
import requests

r = requests.get("http://onto-ns.com/meta/0.1/Molecule")
content = json.loads(r.content)
print("*** PAYLOAD:", content)





# We would like this to work, then we could hide it away by appending
# "http://service" to `dlite.storage_path`
Energy = dlite.Instance.from_location(
    "http", "service", options="", id="http://onto-ns.com/meta/0.1/Energy",
)
