"""A module for converting option to storage plugins between
valid percent-encoded URL query strings and and dicts.

Options is a string of the form

    opt1=value1;opt2=value2...

where semicolon (;) may be replaced with an ampersand (&).



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
        dict.__init__(self)
        if isinstance(defaults, str):
            defaults = Options(defaults)
        if defaults:
            self.update(defaults)

        if isinstance(options, str):
            # URI-decode the options string
            options = dlite.uridecode(options)

            if options.startswith("{"):
                self.update(json.loads(options))
            else:
                # strip hash and everything following
                options = options.split("#")[0]
                if ";" in options:
                    tokens = options.split(";")
                elif "&" in options:
                    tokens = options.split("&")
                else:
                    tokens = [options]
                if tokens and tokens != [""]:
                    self.update([t.split("=", 1) for t in tokens])
        elif isinstance(options, dict):
            self.update(options)
        elif options is not None:
            raise TypeError(
                "`options` should be either a %-encoded string or a dict: "
                f"{options!r}"
            )

    def __getattr__(self, name):
        if name in self:
            return self[name]
        else:
            raise KeyError(name)

    def __setattr__(self, name, value):
        self[name] = value

    def __str__(self):
        encode = False
        for value in self.values():
            if isinstance(value, (bool, int, float)):
                encode = True
                break
            elif isinstance(value, str):
                if ("&" in value) | (";" in value):
                    encode = True
                    break
        if encode:
            return json.dumps(self, separators=(",", ":"))
        else:
            return ";".join([f"{k}={v}" for k, v in self.items()])


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
