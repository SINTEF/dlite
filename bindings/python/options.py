"""A module for parsing standard options intended to be used by Python
storage plugins."""


class Options(dict):
    """A dict representation of the options string `options`.

    Options is a string of the form

        opt1=value1;opt2=value2...

    where semicolon (;) may be replaced with an ampersand (&).

    Default values may be provided via the `defaults` argument.  It
    should either be a dict or a string of the same form as `options`.

    Options may also be accessed as attributes.
    """

    def __init__(self, options, defaults=None):
        dict.__init__(self)
        if options is None:
            options = ""
        options = options.split("#")[0]  # strip hash and everything following
        if isinstance(defaults, str):
            defaults = Options(defaults)
        if defaults:
            self.update(defaults)

        if ";" in options:
            tokens = options.split(";")
        elif "&" in options:
            tokens = options.split("&")
        else:
            tokens = [options]

        if tokens and tokens != [""]:
            self.update([t.split("=", 1) for t in tokens])

    def __getattr__(self, name):
        if name in self:
            return self[name]
        else:
            raise KeyError(name)

    def __setattr__(self, name, value):
        self[name] = value
