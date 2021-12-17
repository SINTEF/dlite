#include "minunit/minunit.h"

#include "dlite.h"
#include "dlite-macros.h"
#include "dlite-storage-plugins.h"

MU_TEST(test_save)
{
  DLiteStorage* s = NULL;

  // Load JSON metadata
  char* url
	  = "json://" STRINGIFY(DLITE_ROOT) "/src/tests/test-entity.json?mode=r";
  DLiteInstance* meta = (DLiteInstance*)dlite_meta_load_url(url);
  mu_check(meta);
  mu_check(dlite_instance_is_meta(meta));

  // Save JSON metadata to BSON file
  mu_check(s = dlite_storage_open("bson", "test_meta.bson", "mode=w"));
  mu_assert_int_eq(1, dlite_storage_is_writable(s));
  mu_assert_int_eq(0, dlite_instance_save(s, meta));
  mu_assert_int_eq(0, dlite_storage_close(s));
  
  // Load JSON data (corresponding to the metadata above)
  url = STRINGIFY(DLITE_ROOT) "/src/tests/test-data.json";
  mu_check(s = dlite_storage_open("json", url, "mode=r"));
  DLiteInstance* data1
	  = dlite_instance_load(s, "204b05b2-4c89-43f4-93db-fd1cb70f54ef");
  mu_check(dlite_instance_is_data(data1));
  DLiteInstance* data2
	  = dlite_instance_load(s, "e076a856-e36e-5335-967e-2f2fd153c17d");
  mu_check(dlite_instance_is_data(data2));
  
  // Save JSON data to BSON file
  mu_check(s = dlite_storage_open("bson", "test_data.bson", "mode=w"));
  mu_assert_int_eq(1, dlite_storage_is_writable(s));
  mu_assert_int_eq(0, dlite_instance_save(s, data1));
  mu_assert_int_eq(0, dlite_instance_save(s, data2));
  mu_assert_int_eq(0, dlite_storage_close(s));
  while (dlite_instance_decref(data1)); // Remove all references to data1
  while (dlite_instance_decref(data2)); // Remove all references to data2
  while (dlite_instance_decref(meta)); // Remove all references to meta
}

MU_TEST(test_load)
{
  DLiteStorage* s = NULL;

  // Load JSON metadata
  char* url
	  = "json://" STRINGIFY(DLITE_ROOT) "/src/tests/test-entity.json?mode=r";
  DLiteInstance* json_meta = (DLiteInstance*)dlite_meta_load_url(url);
  char* json_str1 = dlite_json_aprint(json_meta, 0, 1);
  while (dlite_instance_decref(json_meta)); // Remove all json_meta references

  // Load BSON metadata
  mu_check(s = dlite_storage_open("bson", "test_meta.bson", "mode=r"));
  DLiteInstance* bson_meta
	  = dlite_instance_load(s, "2b10c236-eb00-541a-901c-046c202e52fa");
  mu_check(bson_meta);
  char* bson_str1 = dlite_json_aprint(bson_meta, 0, 1);
  while (dlite_instance_decref(bson_meta)); // Remove all bson_meta references

  // Compare JSON and BSON metadata
  mu_assert_int_eq(0, strcmp(json_str1, bson_str1));
  mu_assert_int_eq(0, dlite_storage_close(s));
  
  // Load JSON data (corresponding to the metadata above)
  url = STRINGIFY(DLITE_ROOT) "/src/tests/test-data.json";
  s = dlite_storage_open("json", url, "mode=r");
  DLiteInstance* json_data
	  = dlite_instance_load(s, "204b05b2-4c89-43f4-93db-fd1cb70f54ef");
  json_str1 = dlite_json_aprint(json_data, 0, 1);
  while (dlite_instance_decref(json_data)); // Remove all json_data references
  json_data = dlite_instance_load(s, "e076a856-e36e-5335-967e-2f2fd153c17d");
  char* json_str2 = dlite_json_aprint(json_data, 0, 1);
  while (dlite_instance_decref(json_data)); // Remove all json_data references

  // Load BSON data
  mu_check(s = dlite_storage_open("bson", "test_data.bson", "mode=r"));
  DLiteInstance* bson_data
	  = dlite_instance_load(s, "204b05b2-4c89-43f4-93db-fd1cb70f54ef");
  mu_check(bson_data);
  bson_str1 = dlite_json_aprint(bson_data, 0, 1);
  while (dlite_instance_decref(bson_data)); // Remove all bson_data references
  bson_data = dlite_instance_load(s, "e076a856-e36e-5335-967e-2f2fd153c17d");
  mu_check(bson_data);
  char* bson_str2 = dlite_json_aprint(bson_data, 0, 1);
  while (dlite_instance_decref(bson_data)); // Remove all bson_data references

  // Compare JSON and BSON data
  mu_assert_int_eq(0, strcmp(json_str1, bson_str1));
  mu_assert_int_eq(0, strcmp(json_str2, bson_str2));
  mu_assert_int_eq(0, dlite_storage_close(s));
}

MU_TEST(test_unload_plugins)
{
  dlite_storage_plugin_unload_all();
}

/****************************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_save);
  MU_RUN_TEST(test_load);
  MU_RUN_TEST(test_unload_plugins);
}

int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail ? 1 : 0);
}
