import json
import sys
import warnings
from typing import Dict, List, Mapping, Optional, Sequence

# dataclasses is a rather new feature of Python, lets not require it...
try:
    from dataclasses import asdict, dataclass, is_dataclass
except:
    HAVE_DATACLASSES = False
else:
    HAVE_DATACLASSES = True

# pydantic is a third party library, lets not require it...
try:
    from pydantic import AnyUrl, BaseModel, Field
except:
    HAVE_PYDANTIC = False
else:
    HAVE_PYDANTIC = True

import numpy as np

import dlite


class MissingDependencyError(dlite.DLiteError):
    """The feature you request requires installing an external package.

    Install it with

        pip install <package>
    """

    exit_code = 44


class CannotInferDimensionError(dlite.DLiteError):
    """Cannot infer instance dimensions."""


class InvalidNumberOfDimensionsError(dlite.DLiteError):
    """Invalid number of instance dimensions."""


class MetadataNotDefinedError(dlite.DLiteError):
    """Metadata is not found in the internal instance store."""


def uncaught_exception_hook(exetype, value, trace):
    """A exception hook that allows exceptions to define an
    `exit_code` attribute that will make Python exit with that code if
    the exception is uncaught.
    """
    oldhook(exetype, value, trace)
    if hasattr(value, "exit_code"):
        sys.exit(value.exit_code)


sys.excepthook, oldhook = uncaught_exception_hook, sys.excepthook


def instance_from_dict(d, id=None, single=None, check_storages=True):
    """Returns a new DLite instance created from dict.

    Parameters
    ----------
    d: dict
        Dict to parse.  It should be of the same form as returned
        by the Instance.asdict() method.
    id: str
        Identity of the returned instance.

        If `d` is in single-entity form with no explicit 'uuid' or
        'uri', its identity will be assigned by `id`.  Otherwise
        `id` must be consistent with the 'uuid' and/or 'uri'
        fields of `d`.

        If `d` is in multi-entity form, `id` is used to select the
        instance to return.
    single: bool | None | "auto"
        Whether the dict is assumed to be in single-entity form
        If `single` is None or "auto", the form is inferred.
    check_storages: bool
        Whether to check if the instance already exists in storages
        specified in `dlite.storage_path`.
    """
    if single is None or single == "auto":
        single = True if "properties" in d else False

    if single:
        if not id and "uuid" not in d and "uri" not in d:
            if "namespace" in d and "version" in d and "name" in d:
                id = f"{d['namespace']}/{d['version']}/{d['name']}"
            else:
                raise ValueError(
                    "`id` required for dicts in single-entry "
                    "form with no explicit uuid or uri."
                )
    else:
        if not id:
            if len(d) == 1:
                (id,) = d.keys()
            else:
                raise ValueError(
                    "`id` required for dicts in multi-entry form."
                )
        if id in d:
            return instance_from_dict(
                d[id], id=id, single=True, check_storages=check_storages
            )
        else:
            uuid = dlite.get_uuid(id)
            if uuid in d:
                return instance_from_dict(
                    d[uuid], id=id, single=True, check_storages=check_storages
                )
            else:
                raise ValueError(f"no such id in dict: {id}")

    if "uri" in d or "uuid" in d:
        if "uri" in d and "uuid" in d:
            if dlite.get_uuid(d["uri"]) != d["uuid"]:
                raise dlite.DLiteError(
                    "uri and uuid in dict are not consistent"
                )
        uuid = dlite.get_uuid(str(d.get("uuid", d.get("uri"))))
        if id:
            if dlite.get_uuid(id) != uuid:
                raise ValueError(
                    f"`id` is not consistent with uri/uuid in dict"
                )

    meta = dlite.get_instance(d.get("meta", dlite.ENTITY_SCHEMA))

    if meta.is_metameta:
        if "uri" in d:
            uri = d["uri"]
        else:
            uri = dlite.join_meta_uri(d["name"], d["version"], d["namespace"])

        if check_storages:
            try:
                with dlite.silent:
                    return dlite.get_instance(uri)
            except dlite.DLiteError:
                pass

        if isinstance(d["dimensions"], Sequence):
            dimensions = [
                dlite.Dimension(d["name"], d.get("description"))
                for d in d["dimensions"]
            ]
        elif isinstance(d["dimensions"], Mapping):
            dimensions = [
                dlite.Dimension(k, v) for k, v in d["dimensions"].items()
            ]
        else:
            raise TypeError(
                "`dimensions` must be either a sequence or a mapping"
            )

        props = []
        if isinstance(d["properties"], Sequence):
            for p in d["properties"]:
                props.append(
                    dlite.Property(
                        name=p["name"],
                        type=p["type"],
                        dims=p.get("shape", p.get("dims")),
                        unit=p.get("unit"),
                        description=p.get("description"),
                    )
                )
        elif isinstance(d["properties"], Mapping):
            for k, v in d["properties"].items():
                props.append(
                    dlite.Property(
                        name=k,
                        type=v["type"],
                        dims=v.get("shape", v.get("dims")),
                        unit=v.get("unit"),
                        description=v.get("description"),
                    )
                )
        else:
            raise TypeError(
                "`properties` must be either a sequence or a mapping"
            )

        inst = dlite.Instance.create_metadata(
            uri, dimensions, props, d.get("description")
        )
    else:
        dims = [
            d["dimensions"][dim.name] for dim in meta.properties["dimensions"]
        ]
        inst_id = d.get("uri", d.get("uuid", id))
        inst = dlite.Instance.from_metaid(meta.uri, dims=dims, id=inst_id)
        for p in meta["properties"]:
            value = d["properties"][p.name]
            inst[p.name] = value

    return inst


def to_metadata(obj):
    """Converts `obj` to dlite Metadata."""
    if isinstance(obj, dlite.Instance):
        if obj.is_data:
            raise TypeError("data instances cannot be converted to metadata")
        return obj

    if isinstance(obj, dict):
        d = obj
    elif isinstance(obj, str):
        d = json.loads(obj)
    elif HAVE_DATACLASSES and is_dataclass(obj):
        if isinstance(obj, type):
            raise NotImplementedError(
                "only instances of dataclasses can be converted to metadata"
            )
        d = asdict(obj)
    elif HAVE_PYDANTIC and isinstance(obj, BaseModel):
        if isinstance(obj, type):
            raise NotImplementedError(
                "only pydandic model instances can be converted to metadata"
            )
        d = obj.dict()
    else:
        raise TypeError(
            "obj can be dict, json string, dataclasses instance "
            f"or pydantic instance.  Got {type(obj)}"
        )
    return instance_from_dict(d)


def get_dataclass_entity_schema():
    """Returns the datamodel for dataclasses in Python standard library."""

    if not HAVE_DATACLASSES:
        raise MissingDependencyError("dataclasses")

    @dataclass
    class Property:
        type: str
        # @ref: Optional[str]  # Should we rename this to "ref"? See issue #595
        shape: Optional[List[str]]
        unit: Optional[str]
        description: Optional[str]

    @dataclass
    class EntitySchema:
        uri: str
        description: Optional[str]
        dimensions: Dict[str, str]
        properties: Dict[str, Property]

    return EntitySchema


def pydantic_to_property(
    name: str,
    propdict: dict,
    dimensions: "Optional[dict]" = None,
    namespace: str = "http://onto-ns.com/meta",
    version: str = "0.1",
):
    """Return a dlite property from a name and a pydantic property dict.

    Arguments:
        name: Name of the property to create.
        propdict: Pydantic property dict.
        dimensions: If given, the dict will be updated with new dimensions
            from array properties.
        namespace: For a reference property use this as the namespace of
            the property to refer to.
        version:  For a reference property use this as the version of
            the property to refer to.

    Returns:
        New DLite property.
    """
    if not HAVE_PYDANTIC:
        raise MissingDependencyError("pydantic")

    # Map simple pydantic types to corresponding dlite types
    simple_types = dict(
        boolean="bool", integer="int64", number="float64", string="string"
    )

    if dimensions is None:
        dimensions = {}

    # Infer property type
    if "type" in propdict:
        ptype = propdict["type"]
    elif "anyOf" in propdict:
        # 'anyOf' was introduced in Pydantic 2
        typedicts = [d for d in propdict["anyOf"] if d["type"] != "null"]
        if not typedicts:
            raise dlite.DliteValueError(
                "no non-null type in `propdict`. "
                "Please add explicit type to `propdict`."
            )
        if len(typedicts) > 1:
            raise dlite.DliteValueError(
                f"more than one type in `propdict`: {typedicts}. "
                "Please add explicit type to `propdict`."
            )
        typedict = typedicts[0]
        if "type" not in typedict:
            raise dlite.DliteValueError(
                "missing type in field 'anyOf' of `propdict`"
            )
        ptype = typedict["type"]
    else:
        ptype = "ref"

    unit = propdict.get("unit")
    descr = propdict.get("description")

    if ptype in simple_types:
        return dlite.Property(
            name, simple_types[ptype], unit=unit, description=descr
        )

    if ptype == "array":
        if "type" in propdict:
            subprop = pydantic_to_property("tmp", propdict["items"])
        elif "anyOf" in propdict:
            subprop = pydantic_to_property("tmp", typedict["items"])
        else:
            raise dlite.DliteSystemError(
                f'`propdict` for arrays must have key "type" or "anyOf"'
            )
        shape = propdict.get("shape", [f"n{name}"])
        for dim in shape:
            dimensions.setdefault(dim, f"Number of {dim}.")
        return dlite.Property(
            name,
            subprop.type,
            ref=subprop.ref,
            dims=shape,
            unit=unit,
            description=descr,
        )

    if ptype == "ref":
        refname = propdict["$ref"].rsplit("/", 1)[-1]
        ref = f"{namespace}/{version}/{refname}"
        prop = dlite.Property(
            name, "ref", ref=ref, unit=unit, description=descr
        )
        return prop

    raise ValueError(f"unsupported pydantic type: {ptype}")


def pydantic_to_metadata(
    model,
    uri=None,
    default_namespace="http://onto-ns.com/meta",
    default_version="0.1",
    metaid=dlite.ENTITY_SCHEMA,
):
    """Create a new dlite metadata from a pydantic model.

    Arguments:
        model: A pydantic model or an instance of one to create the
            new metadata from.
        uri: URI of the created metadata.  If not given, it is
            inferred from `default_namespace`, `default_version` and
            the title of the model schema.
        default_namespace: Default namespace used if `uri` is None.
        default_version: Default version used if `uri` is None.
        metaid: Metadata for the created metadata.  Defaults to
            dlite.ENTITY_SCHEMA.
    """
    if not HAVE_PYDANTIC:
        raise MissingDependencyError("pydantic")

    d = model.schema()
    if not uri:
        uri = f"{default_namespace}/{default_version}/{d['title']}"

    dimensions = {}
    properties = []
    for name, descr in d["properties"].items():
        properties.append(
            pydantic_to_property(
                name, descr, dimensions, default_namespace, default_version
            )
        )
    dims = [dlite.Dimension(k, v) for k, v in dimensions.items()]
    return dlite.Instance.create_metadata(
        uri,
        dims,
        properties,
        d.get("description", ""),
    )


def pydantic_to_instance(meta, pydinst):
    """Return a new dlite instance from a pydantic instance `pydinst`."""
    if not HAVE_PYDANTIC:
        raise MissingDependencyError("pydantic")

    d = pydinst if isinstance(pydinst, dict) else pydinst.dict()
    meta = dlite.get_instance(meta)
    dimensions = infer_dimensions(meta, d)
    inst = meta(dimensions)

    def getval(p, v):
        if p.type == "ref":
            if dlite.has_instance(p.ref):
                submeta = dlite.get_instance(p.ref)
                return pydantic_to_instance(submeta, v)
            else:
                raise MetadataNotDefinedError(p.ref)
        else:
            return v

    for k, v in d.items():
        p = inst.get_property_descr(k)
        if p.ndims:
            inst[k] = [getval(p, w) for w in v]
        else:
            inst[k] = getval(p, v)

    return inst


def get_pydantic_entity_schema():
    """Returns the datamodel for dataclasses in Python standard library."""
    if not HAVE_PYDANTIC:
        raise MissingDependencyError("pydantic")

    class Property(BaseModel):
        type: str = Field(...)
        shape: Optional[Sequence[str]] = Field(None, alias="dims")
        ref: Optional[str] = Field(None, alias="$ref")
        unit: Optional[str] = Field(None)
        description: Optional[str] = Field(None)

    class EntitySchema(BaseModel):
        uri: AnyUrl = Field(...)
        description: Optional[str] = Field("")
        dimensions: Optional[Dict[str, str]] = Field({})
        properties: Dict[str, Property] = Field(...)

    return EntitySchema


def get_package_paths():
    """Returns a dict with all the DLite builtin path variables."""
    return {k: v for k, v in dlite.__dict__.items() if k.endswith("_path")}


def infer_dimensions(meta, values, strict=True):
    """Infer the dimensions if we should create an instance of `meta` with
    the given `values`.

    Arguments:
        meta: URI or metadata object.
        values: Dict mapping property names to values.  Not all property
            names needs to be mapped.
        strict: Whether to require that all keys in `values` correspond
            to a property name in `meta`.

    Returns:
        Dict mapping dimension names to dimension values.

    Raises:
        InvalidNumberOfDimensionsError: Inconsistent number of dimensions.
        CannotInferDimensionError: Cannot infer instance dimensions.
    """
    if isinstance(meta, str):
        meta = dlite.get_instance(meta)

    if strict:
        propnames = {propname for propname in values.keys()}
        extra_props = propnames.difference(
            {prop.name for prop in meta["properties"]}
        )
        if extra_props:
            raise CannotInferDimensionError(
                f"invalid property names in `values`: {extra_props}"
            )

    dims = {}
    for prop in meta["properties"]:
        if prop.name in values and prop.ndims:
            with warnings.catch_warnings():
                warnings.filterwarnings(
                    "ignore",
                    message="The unit of the quantity is stripped when "
                    "downcasting to ndarray.",
                )
                v = np.array(values[prop.name])
            if len(v.shape) != prop.ndims:
                raise InvalidNumberOfDimensionsError(
                    f"property {prop.name} has {prop.ndims} dimensions, but "
                    f"{len(v.shape)} was provided"
                )
            for i, dimname in enumerate(prop.shape):
                if dimname in dims and v.shape[i] != dims[dimname]:
                    raise CannotInferDimensionError(
                        f'inconsistent assignment of dimension "{dimname}" '
                        f'when checking property "{prop.name}"'
                    )
                dims[dimname] = v.shape[i]

    dimnames = {d.name for d in meta["dimensions"]}
    if len(dims) != len(meta["dimensions"]):
        missing_dims = dimnames.difference(dims.keys())
        raise CannotInferDimensionError(
            f"insufficient number of properties provided to infer dimensions: "
            f"{missing_dims}"
        )

    return dims
