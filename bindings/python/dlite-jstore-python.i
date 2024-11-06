/* -*- Python -*-  (not really, but good for syntax highlighting) */

/* Python-spesific extensions to dlite-storage.i */

%pythoncode %{
import json
from typing import Mapping, Sequence


def format_dict(
    d, id=None, soft7=True, single=None, with_uuid=None, with_meta=False,
    with_parent=True, urikey=False,
):
    """Return a copy of `d` formatted according to the given options.

    Arguments:
        d: Input dict.
        id: If given, return dict-representation of this id.
            Otherwise, return dict-representation of the store.
        soft7: Whether to use soft7 formatting.
        single: Whether to return in single-instance format.
            If None, single-instance format is used for metadata and
            multi-instance format for data instances.
        with_uuid: Whether to include UUID in the dict.  The default
            is true if `single=True` and URI is None, otherwise it
            is false.
        with_meta: Whether to always include "meta" (even for metadata)
        with_parent: Whether to include parent info for transactions.
        urikey: Whether the URI is the preferred keys in multi-instance
            format.

    Notes:
        This method works with the dict-representation and does not
        access instances.  The only exception is when `d` corresponds to
        a data instance who's dimensions is a list of dimension lengths.
        In this case is the metadata needed to get dimension names.

    """
    if not id and single and "properties" not in d and len(d) != 1:
        raise _dlite.DLiteLookupError(
            "`id` must be given for `single=True` unless there is only one item"
        )

    if id and "properties" not in d:
        uuid = _dlite.get_uuid(id)
        key = id if id in d else uuid if uuid in d else None
        if not key:
            raise _dlite.DLiteLookupError(f"no such key in store: {id}")
        return format_dict(
            d[key], id=id, soft7=soft7, single=single, with_uuid=with_uuid,
            with_meta=with_meta, with_parent=with_parent, urikey=urikey)

    dct = {}

    if "properties" not in d:
        if single:
            if len(d) != 1:
                raise dlite.DLiteValueError(
                    "Not possible to return single-instance format, use `id`"
                )
        else:
            for k, v in d.items():
                vid  = v.get("uri", v.get("identity", k))
                key = vid if urikey else v.get("uuid", _dlite.get_uuid(vid))
                dct[key] = format_dict(
                    v, id=k, soft7=soft7, single=True, with_uuid=with_uuid,
                    with_meta=with_meta, with_parent=with_parent
                )
            return dct

    uri = d.get("uri", d.get("identity"))
    uuid = d.get("uuid", _dlite.get_uuid(uri) if uri else None)
    if id and not uuid:
        if _dlite.get_uuid_version(id) == 5:
            uri = id
        uuid = _dlite.get_uuid(id)
    metaid = d.get("meta", _dlite.ENTITY_SCHEMA)
    ismeta = (
        "meta" not in d
        or d["meta"] in (_dlite.ENTITY_SCHEMA, _dlite.BASIC_METADATA_SCHEMA)
        or "properties" in d["properties"]
    )

    if single is None:
        single = ismeta
    if with_uuid is None:
       with_uuid = single and not ("uri" in d or "identity" in d)

    if not uuid and (with_uuid or not single):
        raise _dlite.DLiteTypeError("cannot infer UUID from dict")

    if with_uuid:
        dct["uuid"] = uuid
    if uri:
        dct["uri"] = uri
    if with_meta or metaid != _dlite.ENTITY_SCHEMA:
        dct["meta"] = metaid
    if ismeta and "description" in d:
        dct["description"] = d["description"]
    if with_parent and "parent" in d:
        dct["parent"] = d["parent"].copy()

    dct["dimensions"] = {} if soft7 or not ismeta else []
    if "dimensions" in d:
        if isinstance(d["dimensions"], Mapping):
            if soft7 or not ismeta:
                dct["dimensions"].update(d["dimensions"])
            else:
                for k, v in d["dimensions"].items():
                    dct["dimensions"].append({"name": k, "description": v})
        elif isinstance(d["dimensions"], Sequence):
            if soft7 and ismeta:
                for dim in d["dimensions"]:
                    dct["dimensions"][dim["name"]] = dim.get("description", "")
            elif ismeta:
                dct["dimensions"].extend(d["dimensions"])
            else:
                meta = get_instance(metaid)
                for name, value in zip(meta.dimnames(), d["dimensions"]):
                    dct["dimensions"][name] = value
        else:
            raise dlite.DLiteValueError(
                "'dimensions' must be a mapping or sequence, got: "
                f"{type(d['dimensions'])}"
            )

    dct["properties"] = {} if soft7 or not ismeta else []
    if isinstance(d["properties"], Mapping):
        if soft7 or not ismeta:
            dct["properties"].update(d["properties"])
        else:
            for k, v in d["properties"].items():
                prop = {"name": k}
                prop.update(v)
                dct["properties"].append(prop)
    elif isinstance(d["properties"], Sequence):
        if not ismeta:
            raise dlite.DLiteValueError(
                "only metadata can have a sequence of properties"
            )
        if soft7:
            for prop in d["properties"]:
                p = prop.copy()
                name = p.pop("name")
                dct["properties"][name] = p
        else:
            dct["properties"].extend(d["properties"])
    else:
        raise dlite.DLiteValueError(
            "'properties' must be a mapping or sequence, got: "
            f"{type(d['properties'])}"
        )

    if "relations" in d:
        dct["relations"] = d["repations"].copy()

    if single:
        return dct
    return {uri if uri and urikey else uuid: dct}


class _JSONEncoder(json.JSONEncoder):
    """JSON encoder that also handle bytes object."""
    def default(self, o):
        if isinstance(o, bytes):
            return "".join(f"{c:x}" for c in o)
        elif (isinstance(o, (str, bool, int, float, list, dict)) or o is None):
            return super().default(o)
        else:
            return str(o)


%}


%extend _DLiteJStoreIter {
  %pythoncode %{
      def __next__(self):
          id = self.next()
          if id:
              return id
          raise StopIteration()

      def __iter__(self):
          return self
  %}
}


%extend _JStore {
  %pythoncode %{
      def get(self, id=None):
          """Return instance with given `id` from store.

          If `id` is None and there is exactly one instance in the store,
          return the instance.  Otherwise raise an DLiteLookupError.
          """
          inst = self._get(id=id)
          return instance_cast(inst)

      def load_dict(self, d, id=None):
          """Load dict representation of instance to the store."""
          if "properties" not in d:
              if id:
                  self.load_dict(d[id], id=id)
              else:
                  for id, val in d.items():
                      self.load_dict(val, id=id)
              return

          d = d.copy()
          uuid = None
          if "uuid" in d:
              uuid = d["uuid"]
          elif "uri" in d or "identity" in d:
              uuid = _dlite.get_uuid(str(d.get("uri", d.get("identity"))))

          if id and uuid and _dlite.get_uuid(id) != uuid:
              raise _dlite.DLiteInconsistentDataError(
                  f"id '{id}' is not consistent with existing uuid: {uuid}"
              )
          elif not id and not uuid:
              raise _dlite.DLiteValueError(
                  "`id` argument is required when dict has no 'uuid', 'uri' "
                  "or 'identity' key"
              )
          elif not uuid:
              assert id
              uuid = _dlite.get_uuid(id)

          assert uuid
          d.setdefault("uuid", uuid)
          if id and id != uuid:
              d.setdefault("uri", id)


          self.load_json(json.dumps(d, cls=_JSONEncoder))

      def get_dict(self, id=None, soft7=True, single=None, with_uuid=None,
                   with_meta=False, with_parent=True, urikey=False):
          """Return dict representation of the store or item with given id.

          Arguments:
              id: If given, return dict-representation of this id.
                  Otherwise, return dict-representation of the store.
              soft7: Whether to use soft7 formatting.
              single: Whether to return in single-instance format.
                  If None, single-instance format is used for metadata and
                  multi-instance format for data instances.
              with_uuid: Whether to include UUID in the dict.  The default
                  is true if `single=True` and URI is None, otherwise it
                  is false.
              with_meta: Whether to always include "meta" (even for metadata)
              with_parent: Whether to include parent info for transactions.
              urikey: Whether the URI is the preferred keys in multi-instance
                  format.

          """
          d = {}
          if id:
              d[id] = json.loads(self.get_json(id))
          else:
              if single is None:
                  single = False
              for _id in self.get_ids():
                  d[_id] = json.loads(self.get_json(_id))

          return format_dict(
              d, id=id, soft7=soft7, single=single, with_uuid=with_uuid,
              with_meta=with_meta, with_parent=with_parent, urikey=urikey
          )

  %}
}
