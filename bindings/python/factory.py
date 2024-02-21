# -*- coding: utf-8 -*-
"""A module that makes it easy to add dlite functionality to existing classes.

Class customisations
--------------------
In order to handle special cases, the following methods may be
defined/overridden in the class that is to be extended:

```python
_dlite_get_<name>(self, name)
_dlite_set_<name>(self, name, value)
_dlite__new__(cls, inst)
```

"""
from __future__ import annotations

import copy
from typing import TYPE_CHECKING

import numpy as np

from .dlite import Instance, Storage

if TYPE_CHECKING:  # pragma: no cover
    from typing import Any, Callable, List, Optional, Tuple, Union

    from dlite import Metadata


class FactoryError(Exception):
    """Base exception for factory errors."""


class IncompatibleClassError(FactoryError):
    """Raised if an extended class is not compatible with its dlite
    metadata."""


class MetaExtension(type):
    """Metaclass for BaseExtension."""

    def __init__(cls, name, bases, attr) -> None:
        super().__init__(name, bases, attr)


class BaseExtension(metaclass=MetaExtension):
    """Base class for extension.

    Except for `dlite_id`, all arguments are passed further to the `__init__()`
    function of the class we are inheriting from.

    If `instanceid` is given, the id of the underlying dlite instance
    will be set to it.
    """

    def __init__(self, *args, instanceid: str = None, **kwargs) -> None:
        self._theclass.__init__(self, *args, **kwargs)
        self._dlite_init(instanceid=instanceid)

    def _dlite_init(self, instanceid: str = None) -> None:
        """Initialise the underlying DLite Instance.

        If `id` is given, the id of the underlying DLite Instance will be
        set to it.

        Arguments:
            instanceid: A DLite ID (UUID) to use for the underlying DLite
                Instance.
        """
        dims = self._dlite_infer_dimensions()
        self.dlite_inst = Instance.from_metaid(
            self.dlite_meta.uri, dims, instanceid
        )
        self._dlite_assign_properties()

    def _dlite_get(self, name: str) -> "Any":
        """Returns the value of property `name` from the wrapped object.

        Arguments:
            name: The property to retrieve.

        Returns:
            The property value.
        """
        if hasattr(self, f"_dlite_get_{name}"):
            return getattr(self, f"_dlite_get_{name}")()

        if hasattr(self, f"get_{name}"):
            return getattr(self, f"get_{name}")()

        return getattr(self, name)

    def _dlite_set(self, name: str, value: "Any") -> None:
        """Sets value of property ``name`` in the wrapped object.

        Arguments:
            name: The property to set a value for.
            value: The value to set for the property.
        """
        if hasattr(self, f"_dlite_set_{name}"):
            getattr(self, f"_dlite_set_{name}")(value)
        elif hasattr(self, f"set_{name}"):
            getattr(self, f"set_{name}")(value)
        else:
            setattr(self, name, value)

    def _dlite_infer_dimensions(
        self,
        meta: "Optional[Metadata]" = None,
        getter: "Optional[Callable[[str], Any]]" = None,
    ) -> "List[int]":
        """Returns inferred property dimensions from __dict__."""
        meta = meta if meta is not None else self.dlite_meta
        getter = getter if getter is not None else self._dlite_get

        dims = [-1] * len(meta.properties["dimensions"])
        dimnames = [dim.name for dim in meta.properties["dimensions"]]
        for prop in meta.properties["properties"]:
            if prop.ndims:
                value = getter(prop.name)
                array = (
                    np.array(value, copy=False)
                    if value is not None
                    else np.zeros([0] * prop.ndims)
                )
                if array.ndim < prop.ndims:
                    raise ValueError(
                        f"Expected {prop.ndims} dimensions for array property "
                        f"{prop.name!r}; got {array.ndim}"
                    )
                for i, pdim in enumerate(prop.shape):
                    if pdim in dimnames:
                        n = dimnames.index(pdim)
                        if dims[n] == -1:
                            dims[n] = array.shape[i]
                        elif array.shape[i] and dims[n] != array.shape[i]:
                            raise ValueError(
                                "Inconsistent length of dimension "
                                f"{meta.properties['dimensions'][n].name!r}; was "
                                f"{dims[n]} but got {array.shape[i]} for property "
                                f"{prop.name!r}"
                            )
        if min(dims) < 0:
            raise ValueError("Cannot infer all dimensions")
        return dims

    def _dlite_assign_properties(self) -> None:
        """Assigns all dlite properties from extended object."""
        for prop in self.dlite_meta.properties["properties"]:
            name = prop.name
            self.dlite_inst[name] = self._dlite_get(name)

    @classmethod
    def _dlite__new__(cls, inst=None):
        """Class method returning a new uninitialised instance of the class
        that is extended.

        This method simply returns `cls.__new__(cls)`.

        Override this method if the extended class already defines a
        `__new__()` method.
        """
        return cls.__new__(cls)

    def dlite_assign(self, inst: Instance) -> None:
        """Assigns self from dlite instance `inst`."""
        if self.dlite_meta.uri != inst.meta.uri:
            raise TypeError(
                f"Expected instance of metadata {self.dlite_meta.uri!r}, got "
                f"{inst.meta.uri!r}"
            )
        for prop in self.dlite_meta.properties["properties"]:
            name = prop.name
            self._dlite_set(name, inst[name])

    def dlite_load(self, *args) -> None:
        """Loads dlite instance from storage and assign self from it.
        The arguments `args` are passed to dlite.Instance.from_storage()."""
        inst = Instance.from_storage(*args)
        self._dlite_assign(inst)


def instancefactory(theclass: type, inst: Instance) -> "Any":
    """Returns an extended instance of `theclass` initiated from dlite
    instance ``inst``.

    Arguments:
        theclass: The class to instantiate an object from using `inst`.
        inst: A DLite Instance to use as source for a `theclass` instance
            object.

    Returns:
        A `theclass` instance object based on the DLite Instance `inst`.

    """
    cls = classfactory(theclass, meta=inst.meta)
    obj = cls._dlite__new__(inst)
    obj.dlite_assign(inst)
    obj.dlite_meta = inst.meta
    obj.dlite_inst = inst
    return obj


def objectfactory(
    obj: "Any",
    meta: "Optional[Metadata]" = None,
    deepcopy: bool = False,
    cls: "Optional[type]" = None,
    url: "Optional[str]" = None,
    storage: "Optional[Union[Storage, Tuple[str, str, str]]]" = None,
    id: "Optional[str]" = None,
    instanceid: "Optional[str]" = None,
) -> "Any":
    """Returns an extended copy of `obj`.

    The `url`, `storage`, and `id` arguments are passed to `classfactory()`.

    Arguments:
        obj: A Python object.
        meta: A DLite Metadata Instance.
        deepcopy: Whether to perform a deep copy.  Otherwise a shallow copy
            is performed.
        cls: A class to use for the new object. If this is not supplied,
            the new object will be of the same class as the original.
        url: If given and `meta` is not given, load metadata from this URL.
            It should be of the form `driver://location?options#id`.
        storage: If given and `meta` and `url` are not given, load metadata
            from this storage.
        id: A unique ID referring to the metadata if `storage` is provided.
        instanceid: A DLite ID (UUID) to use for the underlying DLite Instance.

    Returns:
        A new, extended copy of the Python object `obj`.

    """
    cls = (
        cls
        if cls is not None
        else classfactory(
            obj.__class__,
            meta=meta,
            url=url,
            storage=storage,
            id=id,
        )
    )
    new = copy.deepcopy(obj) if deepcopy else copy.copy(obj)
    new.__class__ = cls
    new._dlite_init(instanceid=instanceid)
    return new


def classfactory(
    theclass: type,
    meta: "Optional[Metadata]" = None,
    url: "Optional[str]" = None,
    storage: "Optional[Union[Storage, Tuple[str, str, str]]]" = None,
    id: "Optional[str]" = None,
) -> type:
    """Factory function that returns a new class that inherits from both
    `theclass` and `BaseExtension`.

    Arguments:
        theclass: The class to extend.
        meta: Metadata instance.
        url: If given and `meta` is not given, load metadata from this URL.
            It should be of the form `driver://location?options#id`.
        storage: If given and `meta` and `url` are not given, load metadata
            from this storage.
        id: A unique ID referring to the metadata if `storage` is provided.

    Returns:
        A new class based on `theclass` and `BaseExtensions`.

    """
    if meta is None:
        if url is not None:
            meta = Instance.from_url(url)
        elif storage is not None:
            if isinstance(storage, Storage):
                meta = Instance.from_storage(storage, id)
            else:
                meta = Instance.from_driver(*storage, id=id)
        else:
            raise TypeError("`meta`, `url`, or `storage` must be provided.")

    if not meta.is_meta:
        raise TypeError("`meta` must refer to metadata.")

    attr = {
        "dlite_meta": meta,
        "_theclass": theclass,
        "__init__": BaseExtension.__init__,
    }

    return type(meta.name, (theclass, BaseExtension), attr)
