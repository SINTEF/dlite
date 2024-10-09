"""A module for handling options passed to storage plugins,
converting between valid percent-encoded URL query strings
and dicts.

A URL query string is a string of key-value pairs separated by either semicolon (;) or ampersand (&).
For example

    key1=value1;key2=value2...

or

    key1=value1&key2=value2...

where the keys and and values are percent-encoded.

Percent-encoding means that all characters that are digits, letters or
one of "~._-" are encoded as-is, while all other are encoded as their
unicode byte number in hex with each byte preceeded "%". For example
"a" would be encoded as "a", "+" would be encoded as "%2B" and "å" as
"%C3%A5".

In DLite, a value can also start with "%%", which means that the rest of the value is assumed to be a percent-encoded json string.
This addition makes it possible to pass any kind of json-serialisable data structures as option values.
"""

import json
import re

import dlite


class Options(dict):
    """A dict representation of the options string `options` with
    attribute access.

    Arguments:
        options: Percent-encoded URL query string or dict.
            The options to represent.
        defaults: Percent-encoded URL query string or dict.
            Default values for options.

    """

    def __init__(self, options, defaults=None):
        super().__init__()
        if defaults:
            self.update(
                parse_query(defaults) if isinstance(defaults, str) else defaults
            )
        if options:
            self.update(
                parse_query(options) if isinstance(options, str) else options
            )

    def __getattr__(self, name):
        if name in self:
            return self[name]
        else:
            raise KeyError(name)

    def __setattr__(self, name, value):
        self[name] = value

    def __str__(self):
        return make_query(self)


def parse_query(query):
    """Parse URL query string `query` and return a dict."""
    d = {}
    for token in re.split("[;&]", query):
        k, v = token.split("=", 1) if "=" in token else (token, None)
        key = dlite.uridecode(k)
        if v.startswith("%%"):
            val = json.loads(dlite.uridecode(v[2:]))
        else:
            val = dlite.uridecode(v)
        d[key] = val
    return d


def make_query(d, sep=";"):
    """Returns an URL query string from dict `d`."""
    lst = []
    for k, v in d.items():
        if isinstance(v, str):
            val = dlite.uriencode(v)
        else:
            val = "%%" + dlite.uriencode(json.dumps(v))
        lst.append(f"{dlite.uriencode(k)}={val}")
    return sep.join(lst)
