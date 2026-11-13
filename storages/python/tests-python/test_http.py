import os
import subprocess
import sys

import dlite
from dlite.testutils import importskip

importskip("requests")  # skip this test if requests is not available

import requests


current_branch = 'master'
url = f"https://raw.githubusercontent.com/SINTEF/dlite/{current_branch}/storages/python/tests-python/input/test_meta.json"

meta = dlite.Instance.from_location("http", url)


assert str(meta) == """
{
  "uri": "http://onto-ns.com/meta/0.1/TestEntity",
  "description": "test entity with explicit meta",
  "dimensions": {
    "L": "first dim",
    "M": "second dim",
    "N": "third dim"
  },
  "properties": {
    "myblob": {
      "type": "blob3"
    },
    "mydouble": {
      "type": "float64",
      "unit": "m",
      "description": "A double following a single character..."
    },
    "myfixstring": {
      "type": "string3",
      "description": "A fix string."
    },
    "mystring": {
      "type": "string",
      "description": "A string pointer."
    },
    "myshort": {
      "type": "uint16",
      "description": "An unsigned short integer."
    },
    "myarray": {
      "type": "int32",
      "shape": ["L", "M", "N"],
      "description": "An array string pointer."
    }
  }
}
""".strip()


# Test fetching datamodel from http://onto-ns.com/
#
# TODO: Replace the entities below with a dedicated test entity that
#       will not be cleaned up.
uri = "http://onto-ns.com/meta/0.4/HallPetch"

# Skip the rest of this test if the entity service is unavailable
try:
    requests.get(uri, timeout=3).raise_for_status()
except requests.RequestException as exc:
    print(f"Entity service unavailable: {exc}: skipping test", file=sys.stderr)
    sys.exit(44)

HallPetch = dlite.get_instance(uri)
assert HallPetch.uri == uri

uri = "http://onto-ns.com/meta/0.3/Chemistry"
Chemistry = dlite.get_instance(uri)
assert Chemistry.uri == uri
