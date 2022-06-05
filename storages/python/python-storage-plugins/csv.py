"""Storage plugin that reading/writing CSV files."""
import sys
import re
import warnings
import hashlib
import ast
from pathlib import Path

import pandas as pd

import dlite
from dlite.options import Options


class csv(dlite.DLiteStorageBase):  # noqa: F821
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
            Comma-separated string of "key"=value options sent to pandas
            read_<format> or save_<format> function.  String values should
            be quoted.
        format: "csv" | "excel" | "json" | "clipboard", ...
            Any format supported by pandas.  The default is inferred from
            the extension of `uri`.
        id: string
            Explicit id of returned instance if reading.  Optional
        """
        self.options = Options(options, defaults='mode=r')
        self.mode = dict(r='rt', w='wt')[self.options.mode]
        self.readable = True  if 'rt' in self.mode else False
        self.writable = False if 'rt' == self.mode else True
        self.generic = False
        self.uri = uri
        self.format = get_pandas_format_name(uri, self.options.get('format'))

    def close(self):
        """Closes this storage."""
        pass

    def load(self, id=None):
        """Loads `id` from current storage and return it as a new
        instance."""
        # This will break recursive search for metadata using this plugin
        if id:
            raise dlite.DLiteError('csv plugin does support loading an '
                                   'instance with a given id')

        reader = getattr(pd, 'read_' + self.format)
        pdopts = optstring2keywords(self.options.get('pandas_opts', ''))
        metaid = self.options.meta if 'meta' in self.options else None
        data = reader(self.uri, **pdopts)
        rows, columns = data.shape

        if 'infer' not in self.options or dlite.asbool(self.options.infer):
            Meta = infer_meta(data, metaid, self.uri)
        elif metaid:
            Meta = dlite.get_instance(metaid)
        else:
            raise ValueError(
                'csv option `meta` must be provided if `infer` if false')

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
    if not metauri:
        ext = Path(uri).suffix.lstrip('.')
        fmt = ext if ext else 'csv'
        with open(uri, 'rb') as f:
            hash = hashlib.sha256(f.read()).hexdigest()
        metauri = f'onto-ns.com/meta/1.0/generated_from_{fmt}_{hash}'
    elif dlite.has_instance(metauri):
        warnings.warn(f'csv option infer is true, but explicit instance id '
                      f'"{metauri}" already exists')

    dims_ = [dlite.Dimension('rows', 'Number of rows.')]
    props = []
    for i, col in enumerate(data.columns):
        name = infer_prop_name(col)
        type = data.dtypes[i].name
        dims = ['rows']
        unit = infer_prop_unit(col)
        props.append(dlite.Property(name=name, type=type, dims=dims,
                                    unit=unit, description=None))
    descr = f'Inferred metadata for {uri}'
    return dlite.Instance.create_metadata(metauri, dims_, props, descr)


def optstring2keywords(optstring):
    """Converts comma-separated ``"key":value`` option string `optstring`
    to a keyword dict and return it.

    The values should be valid Python expressions.  They are parsed with
    ast.literal_eval().
    """
    s = '{%s}' % (optstring, )
    try:
        return ast.literal_eval(s)
    except Exception as e:
        raise ValueError(
            f'invalid in option string ({e.__name__}): {optstring!r}')
