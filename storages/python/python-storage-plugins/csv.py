"""Storage plugin that reading/writing CSV files."""
from __future__ import annotations

import warnings
import hashlib
import ast
from pathlib import Path
from typing import TYPE_CHECKING

import pandas as pd

import dlite
from dlite.options import Options

if TYPE_CHECKING:  # pragma: no cover
    from typing import Any, Optional


class csv(dlite.DLiteStorageBase):
    """DLite storage plugin for CSV files."""

    def open(self, uri: str, options: "Optional[str]" = None) -> None:
        """Opens `uri`, which should be a valid path to a local file.

        Parameters:
            uri: A fully resolved URI to the CSV.
            options: Supported options:

                - mode: Whether to read or write data. Required.
                  Valid values are:

                  - `r`: Read data.
                  - `w`: Write data.

                - meta: URI to metadata describing the table to read.
                  Required if `infer` is ``False``.
                - infer: Whether to infer metadata from data source.
                  Defaults to ``True``.
                - path: Additional search directories to add to the search path for
                  `meta`. Optional.
                - pandas_opts: Comma-separated string of "key"=value options sent to
                  pandas read_<format> or save_<format> function. String values should
                  be quoted.
                - format: Any format supported by pandas. The default is inferred from
                  the extension of `uri`.
                - id: Explicit id of returned instance if reading. Optional.

        """
        self.options = Options(options, defaults="mode=r")
        self.mode = {"r": "rt", "w": "wt"}[self.options.mode]
        self.readable = "rt" in self.mode
        self.writable = "rt" != self.mode
        self.generic = False
        self.uri = uri
        self.format = get_pandas_format_name(uri, self.options.get("format"))

    def close(self) -> None:
        """Closes this storage."""

    def load(self, id: "Optional[str]" = None) -> dlite.Instance:
        """Loads `id` from current storage and return it as a new instance.

        Parameters:
            id: A UUID representing a DLite Instance to return from the CSV storage.

        Returns:
            A DLite Instance corresponding to the given `id` (UUID).

        """
        # This will break recursive search for metadata using this plugin
        if id:
            raise dlite.DLiteError(
                "csv plugin does support loading an instance with a given id"
            )

        reader = getattr(pd, f"read_{self.format}")
        pdopts = optstring2keywords(self.options.get("pandas_opts", ""))
        metaid = self.options.meta if "meta" in self.options else None
        data = reader(self.uri, **pdopts)
        rows, _ = data.shape

        if "infer" not in self.options or dlite.asbool(self.options.infer):
            Meta = infer_meta(data, metaid, self.uri)
        elif metaid:
            Meta = dlite.get_instance(metaid)
        else:
            raise ValueError(
                "csv option `meta` must be provided if `infer` if false"
            )

        inst = Meta(dimensions=(rows,), id=self.options.get("id"))
        for i in range(len(inst.properties)):
            inst[i] = data.iloc[:, i]

        return inst

    def save(self, inst: dlite.Instance) -> None:
        """Stores `inst` in current storage.

        Parameters:
            inst: A DLite Instance to store in the CSV storage.

        """
        inst_as_dict = inst.asdict()
        data = pd.DataFrame(inst_as_dict["properties"])

        writer = getattr(data, f"to_{self.format}")
        pdopts = optstring2keywords(self.options.get("pandas_opts", ""))
        writer(self.uri, **pdopts)


def get_pandas_format_name(uri: str, format: str) -> str:
    """Return Pandas format name corresponding to `format`.

    If `format` is ``None``, the name is inferred from the extension of `uri`.

    Parameters:
            uri: A fully resolved URI to the CSV.
            format: A format to be mapped to Pandas format.

    Returns:
        A Pandas format matching `format`.

    """
    fmt = format.lower() if format else Path(uri).suffix.lstrip(".").lower()
    return {
        "xls": "excel",
        "xlsx": "excel",
        "h5": "hdf",
        "hdf5": "hdf",
    }.get(fmt, fmt)


def infer_prop_name(name: str) -> str:
    """Return inferred property name from pandas column name.

    Parameters:
        name: Pandas column name.

    Returns:
        Inferred property name.

    """
    return (
        name.strip(' "')
        .rsplit("(", 1)[0]
        .rsplit("[", 1)[0]
        .strip()
        .replace(" ", "_")
    )


def infer_prop_unit(name: str) -> "Optional[str]":
    """Return inferred property unit from pandas column name.

    Parameters:
        name: Pandas column name.

    Returns:
        Inferred property unit.

    """
    if "(" in name:
        return name.strip(' "').rsplit("(", 1)[1].strip().rstrip(")").rstrip()

    if "[" in name:
        return name.strip(' "').rsplit("[", 1)[1].strip().rstrip("]").rstrip()

    return None


def infer_meta(data: pd.DataFrame, metauri: str, uri: str) -> dlite.Metadata:
    """Infer dlite metadata from Pandas dataframe `data`.

    Parameters:
        data: A Pandas DataFrame.
        metauri: A namespace/version/name URI for the metadata.
        uri: The location of the input storage.

    Returns:
        A DLite Metadata based on the Pandas DataFrame.

    """
    if not metauri:
        ext = Path(uri).suffix.lstrip(".")
        fmt = ext if ext else "csv"

        with open(uri, "rb") as handle:
            hash = hashlib.sha256(handle.read()).hexdigest()

        metauri = f"http://onto-ns.com/meta/1.0/generated_from_{fmt}_{hash}"
    elif dlite.has_instance(metauri):
        warnings.warn(
            f"csv option infer is true, but explicit instance id {metauri!r} already "
            "exists"
        )

    dims = [dlite.Dimension("rows", "Number of rows.")]
    props = []
    for i, col in enumerate(data.columns):
        name = infer_prop_name(col)
        type = data.dtypes[i].name
        if type == "object":
            type = "string"
        col_shape = ["rows"]
        unit = infer_prop_unit(col)
        props.append(
            dlite.Property(
                name=name,
                type=type,
                shape=col_shape,
                unit=unit,
                description=None,
            )
        )

    return dlite.Instance.create_metadata(
        metauri, dims, props, f"Inferred metadata for {uri}"
    )


def optstring2keywords(optstring: str) -> "dict[str, Any]":
    """Converts comma-separated ``"key":value`` options string `optstring`
    to a keyword dict and returns it.

    The values should be valid Python expressions. They are parsed with
    ast.literal_eval().

    Parameters:
        optstring: Commma-separated options string.

    Returns:
        Parsed options as a dictionary.

    """
    try:
        return ast.literal_eval("{%s}" % (optstring,))
    except Exception as exc:
        raise ValueError(
            f"invalid in option string ({exc.__name__}): {optstring!r}"
        ) from exc
