try:
    import requests
except ModuleNotFoundError:
    print("requests not installed, skipping test")
    raise SystemExit(44)  # skip test

import dlite


url = "https://raw.githubusercontent.com/SINTEF/dlite/master/storages/python/tests-python/input/test_meta.json"

meta = dlite.Instance.from_location("http", url)
print(meta)
