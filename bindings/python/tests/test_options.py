"""Test options module."""

import dlite
from dlite.options import Options, make_query, parse_query
from dlite.testutils import raises


d = {"a": "A", "b": "B", "c": "C"}
query = make_query(d)
opts = parse_query(query)
assert opts == d

d2 = {"number": 1, "color": "red", "value": True}
query2 = make_query(d2)
opts2 = parse_query(query2)
assert opts2 == d2


opts = Options("number=1&color=red&value=true", defaults="x=false")
assert opts.number == "1"
assert opts.color == "red"
assert dlite.asbool(opts.value) is True
assert dlite.asbool(opts.x) is False
assert "number" in opts
assert "name" not in opts

with raises(KeyError):
    opts.y


opts = Options(
    options={"number": 1, "color": "red", "value": True},
    defaults={"name": "<unknown>", "color": "blue"},
)
