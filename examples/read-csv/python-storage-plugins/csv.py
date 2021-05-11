"""Storage plugin that reading/writing CSV files."""
import os
import sys
import re
from pathlib import Path

import numpy as np
import pandas as pd

import dlite
from dlite.options import Options
from dlite.utils import instance_from_dict



class csv(DLiteStorageBase):
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
        pandas: string
            Comma-separated string of key=value options sent to pandas.
        format: "csv" | "excel" | "json" | "clipboard", ...
            Any format supported by pandas.  Defaults to 'csv'
        id: string
            Explicit id of returned instance if reading.  Optional
        """
        self.options = Options(options, defaults='mode=r')
        self.mode = dict(r='rt', w='wt')[self.options.mode]
        self.writable = False if 'r' in self.mode else True
        self.uri = uri
        self.f = open(uri, self.mode)

    def close(self):
        """Closes this storage."""
        self.f.close()

    def load(self, uuid=None):
        """Loads `uuid` from current storage and return it as a new instance."""
        format = self.options.get('format', 'csv')
        reader = getattr(pd, 'read_' + format)
        data = reader(self.uri)  # FIXME - pass options to pandas
        rows, columns = data.shape

        if 'infer' not in self.options or self.options.infer:
            dims_ = [dlite.Dimension('rows', 'Number of rows.')]
            props = []
            for i, col in enumerate(data.columns):
                name = infer_prop_name(col)
                type = data.dtypes[i].name
                dims = ['rows']
                unit = infer_prop_unit(col)
                props.append(dlite.Property(name, type, dims, unit, None, None))
            descr = f'Inferred metadata for {self.uri}'
            Meta = dlite.Instance(self.options.meta, dims_, props, descr)
        else:
            Meta = dlite.get_instance(self.options.meta)

        inst = Meta(dims=(rows, ))
        for i, name in enumerate(inst.properties):
            inst[i] = data.iloc[:, i]

        return inst

    def save(self, inst):
        """Stores `inst` in current storage."""
        pass  # XXX

    def queue(self, pattern=None):
        """Generator method that iterates over all UUIDs in the storage
        who's metadata URI matches glob pattern `pattern`."""
        pass  # XXX



def infer_prop_name(name):
    """Infer property name from pandas column name."""
    return name.strip(' "').rsplit('(', 1)[0].rsplit(
        '[', 1)[0].strip().replace(' ', '_')

def infer_prop_unit(name):
    """Infer property unit from pandas column name."""
    if '(' in name:
        return name.strip(' "').rsplit('(', 1)[1].strip().rstrip(')').rstrip()
    elif '[' in name:
        return name.strip(' "').rsplit('[', 1)[1].strip().rstrip(']').rstrip()
    else:
        return None
