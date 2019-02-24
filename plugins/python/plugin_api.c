
static DLiteStoragePlugin dlite_python_plugin = {
  "python",

  dlite_json_open,
  dlite_json_close,

  dlite_json_datamodel,
  dlite_json_datamodel_free,

  dlite_json_get_metadata,
  dlite_json_get_dimension_size,
  dlite_json_get_property,

  /* optional */
  dlite_json_get_uuids,

  dlite_json_set_metadata,
  dlite_json_set_dimension_size,
  dlite_json_set_property,

  dlite_json_has_dimension,
  dlite_json_has_property,

  dlite_json_get_dataname,
  dlite_json_set_dataname,

  dlite_json_get_entity,
  dlite_json_set_entity
};


DSL_EXPORT const DLiteStoragePlugin *
get_dlite_storage_plugin_api(const char *name)
{
  UNUSED(name);
  return &dlite_json_plugin;
}
