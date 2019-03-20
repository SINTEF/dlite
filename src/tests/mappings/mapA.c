#include "dlite.h"
#include "dlite-macros.h"
#include "dlite-mapping-plugins.h"


DLiteInstance *mapper(DLiteInstance **instances, int n)
{
  DLiteInstance *inst1, *inst2;
  int *p, a, b;
  UNUSED(n);

  inst1 = instances[0];
  inst2 = dlite_instance_create_from_id("http://meta.sintef.no/0.1/ent2",
                                        NULL, NULL);
  p = dlite_instance_get_property(inst1, "a");
  a = *p;
  b = a + 1;
  dlite_instance_set_property(inst2, "b", &b);
  return inst2;
}


const DLiteMappingPlugin *get_dlite_mapping_api(const char *name)
{
  static DLiteMappingPlugin api;
  static const char *input_uris[] = { "http://meta.sintef.no/0.1/ent1" };
  UNUSED(name);

  api.name = "mapA";
  api.output_uri = "http://meta.sintef.no/0.1/ent2";
  api.ninput = 1;
  api.input_uris = input_uris;
  api.mapper = mapper;
  api.cost = 20;
  return &api;
}
