#include "config.h"

#include "minunit/minunit.h"
#include "dlite.h"


MU_TEST(test_write_schemas)
{
  DLiteStorage *s;
  DLiteMeta *meta;

  meta = (DLiteMeta *)
    dlite_instance_get(DLITE_BASIC_METADATA_SCHEMA);
  s = dlite_storage_open("json", "BasicMetadataSchema.json", "mode=w;meta=yes");
  dlite_instance_save(s, (DLiteInstance *)meta);
  dlite_storage_close(s);
  s = dlite_storage_open("json", "basic_metadata_schema.json", "mode=w");
  dlite_instance_save(s, (DLiteInstance *)meta);
  dlite_storage_close(s);

  meta = (DLiteMeta *)
    dlite_instance_get(DLITE_ENTITY_SCHEMA);
  s = dlite_storage_open("json", "EntitySchema.json", "mode=w;meta=true");
  dlite_instance_save(s, (DLiteInstance *)meta);
  dlite_storage_close(s);
  s = dlite_storage_open("json", "entity_schema.json", "mode=w");
  dlite_instance_save(s, (DLiteInstance *)meta);
  dlite_storage_close(s);

  meta = (DLiteMeta *)
    dlite_instance_get(DLITE_COLLECTION_ENTITY);
  s = dlite_storage_open("json", "Collection.json", "mode=w;meta=true");
  dlite_instance_save(s, (DLiteInstance *)meta);
  dlite_storage_close(s);
  s = dlite_storage_open("json", "collection.json", "mode=w");
  dlite_instance_save(s, (DLiteInstance *)meta);
  dlite_storage_close(s);
}


/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
#ifdef WITH_JSON
  MU_RUN_TEST(test_write_schemas);
#endif
}



int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
