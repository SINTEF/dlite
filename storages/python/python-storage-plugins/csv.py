"""Storage plugin that reading/writing CSV files."""
import re
from pathlib import Path

import pandas as pd

import dlite
from dlite.options import Options


class csv(DLiteStorageBase):  # noqa: F821
    """DLite storage plugin for CSV files."""

    def open(self, uri, options=None):
        """Opens `uri`, which should be a valid path to a local file.

        Options
        -------
        mode: "r" | "w"
            Whether to read or write data.  Required
        meta: URI
            URI to metadata describing the table to read.  Required if `mode`
            is "r"
        infer: bool
            Whether to infer metadata from data source.  Defaults to True
        path: directories
            Additional search directories to add to the search path for
            `meta`.  Optional
        pandas_opts: string
            Comma-separated string of key=value options sent to pandas
            read_<format> or save_<format> function.
            Values containing commas may be quoted with either single
            or double quotes.
        format: "csv" | "excel" | "json" | "clipboard", ...
            Any format supported by pandas.  The default is inferred from
            the extension of `uri`.
        id: string
            Explicit id of returned instance if reading.  Optional
        """
        self.options = Options(options, defaults='mode=r')
        self.mode = dict(r='rt', w='wt')[self.options.mode]
        self.writable = False if 'r' in self.mode else True
        self.uri = uri
        self.format = get_pandas_format_name(uri, self.options.get('format'))

    def close(self):
        """Closes this storage."""
        pass

    def load(self, uuid=None):
        """Loads `uuid` from current storage and return it as a new
        instance."""
        reader = getattr(pd, 'read_' + self.format)
        pdopts = optstring2keywords(self.options.get('pandas_opts', ''))
        data = reader(self.uri, **pdopts)
        rows, columns = data.shape

        if 'infer' not in self.options or self.options.infer:
            Meta = infer_meta(data, self.options.meta, self.uri)
        else:
            Meta = dlite.get_instance(self.options.meta)

        inst = Meta(dims=(rows, ), id=self.options.get('id'))
        for i, name in enumerate(inst.properties):
            inst[i] = data.iloc[:, i]

        return inst

    def save(self, inst):
        """Stores `inst` in current storage."""
        d = inst.asdict()
        data = pd.DataFrame(d['properties'])

        writer = getattr(data, 'to_' + self.format)
        pdopts = optstring2keywords(self.options.get('pandas_opts', ''))
        writer(self.uri, **pdopts)


def get_pandas_format_name(uri, format):
    """Return Pandas format name corresponding to `format`.  If `format`
     is None, the name is inferred from the extension of `uri`.
    """
    fmt = format.lower() if format else Path(uri).suffix.lstrip('.').lower()
    d = {
        'xls': 'excel',
        'xlsx': 'excel',
        'h5': 'hdf',
        'hdf5': 'hdf',
    }
    return d.get(fmt, fmt)


def infer_prop_name(name):
    """Return inferred property name from pandas column name."""
    return name.strip(' "').rsplit('(', 1)[0].rsplit(
        '[', 1)[0].strip().replace(' ', '_')


def infer_prop_unit(name):
    """Return inferred property unit from pandas column name."""
    if '(' in name:
        return name.strip(' "').rsplit('(', 1)[1].strip().rstrip(')').rstrip()
    elif '[' in name:
        return name.strip(' "').rsplit('[', 1)[1].strip().rstrip(']').rstrip()
    else:
        return None


def infer_meta(data, metauri, uri):
    """Infer dlite metadata from Pandas dataframe `data`.
    `metauri` is a namespace/version/name URI for the metadata.
    `uri` is the location of the input storage.
    """
    dims_ = [dlite.Dimension('rows', 'Number of rows.')]
    props = []
    for i, col in enumerate(data.columns):
        name = infer_prop_name(col)
        type = data.dtypes[i].name
        dims = ['rows']
        unit = infer_prop_unit(col)
        props.append(dlite.Property(name, type, dims, unit, None, None))
    descr = f'Inferred metadata for {uri}'
    return dlite.Instance(metauri, dims_, props, None, descr)


def optstring2keywords(optstring):
    """Converts comma-separated ``key=value`` option string `optstring` to
    a keyword dict and return it.  Values containing commas may be
    quoted with either single or double quotes.
    """
    d = {}
    for key, a, b, c in re.findall(
            '([^=]+)=([^"\',]+|"([^"]*)"|\'([^\']*)\'),?', optstring):
        d[key] = c if c else b if b else a
    return d
