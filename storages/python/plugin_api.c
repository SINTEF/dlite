




static DLiteStoragePlugin dlite_python_plugin = {
  "python",
  NULL,

  dlite_python_open,
  dlite_python_close,

  dlite_python_datamodel,
  dlite_python_datamodel_free,

  dlite_python_get_metadata,
  dlite_python_get_dimension_size,
  dlite_python_get_property,

  /* optional */
  dlite_python_get_uuids,

  dlite_python_set_metadata,
  dlite_python_set_dimension_size,
  dlite_python_set_property,

  dlite_python_has_dimension,
  dlite_python_has_property,

  dlite_python_get_dataname,
  dlite_python_set_dataname,

  /* specialised api */
  dlite_python_get_entity,
  dlite_python_set_entity,

  /* internal data */
  NULL,
  NULL
};


DSL_EXPORT const DLiteStoragePlugin *
get_dlite_storage_plugin_api(int *iter)
{
  UNUSED(iter);
  return &dlite_python_plugin;
}
