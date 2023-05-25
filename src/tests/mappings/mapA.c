#include "utils/err.h"

#include "dlite.h"
#include "dlite-macros.h"
#include "dlite-mapping-plugins.h"

typedef DLiteInstance *
(*Creater)(const char *metaid, const size_t *dims, const char *id);


DLiteInstance *mapper(const DLiteMappingPlugin *api,
                      const DLiteInstance **instances, int n)
{
  const DLiteInstance *inst1;
  DLiteInstance *inst2;
  int *p, a, b;

  Creater creater = dlite_instance_create_from_id;
  printf("*** creater: %p\n", *(void **)&creater);

  UNUSED(api);
  UNUSED(n);

  inst1 = instances[0];

  if (!(inst2 = dlite_instance_create_from_id("http://onto-ns.com/metas/0.1/ent2",
                                              NULL, NULL))) return NULL;

  p = dlite_instance_get_property(inst1, "a");
  a = *p;
  b = a + 1;
  dlite_instance_set_property(inst2, "b", &b);

  printf("*** mapA -> inst2: %s\n", inst2->uuid);
  return inst2;
}


DSL_EXPORT const DLiteMappingPlugin *get_dlite_mapping_api(void *state, int *iter)
{
  static DLiteMappingPlugin api;
  static const char *input_uris[] = { "http://onto-ns.com/meta/0.1/ent1" };
  UNUSED(iter);

  dlite_globals_set(state);

  api.name = "mapA";
  api.output_uri = "http://onto-ns.com/meta/0.1/ent2";
  api.ninput = 1;
  api.input_uris = input_uris;
  api.mapper = mapper;
  api.cost = 20;
  return &api;
}
