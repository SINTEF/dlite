#include <assert.h>
#include <string.h>

#include "utils/map.h"

#include "dlite-macros.h"
#include "dlite-store.h"
#include "dlite-entity.h"
#include "dlite-mapping-plugins.h"
#include "dlite-mapping.h"


/*
  The main use of the following map types are as an effictive set, but
  the mapping feature may also become handy.
 */
typedef map_int_t Ints;
typedef map_t(DLiteInstance *) Instances;
typedef map_t(DLiteMapping *) Mappings;


/*
  Recursive help function that removes all mappings found in `m` and its
  submappings from `created`.
 */
void mapping_remove_rec(DLiteMapping *m, Mappings *created)
{
  int i;
  if (!m) return;
  map_remove(created, m->output_uri);
  for (i=0; i < m->ninput; i++)
    if (m->input_maps[i])
      mapping_remove_rec((DLiteMapping *)m->input_maps[i], created);
}

/*
  Recursive help function returning a mapping.

  `inputs` maps input URIs to their corresponding index in the array of
      input URIs.
  `visited` maps so far visited output URIs to corresponding mapping.
  `created` maps all created output URIs to corresponding mapping.
  `dead_ends` maps URIs that we cannot create a mapping to, to NULL.
 */
DLiteMapping *mapping_create_rec(const char *output_uri, Ints *inputs,
                                 Mappings *visited, Mappings *created,
                                 Mappings *dead_ends)
{
  int i, lowest_cost=-1;
  DLiteMapping *m=NULL, *retval=NULL;
  DLiteMappingPlugin *api, *cheapest=NULL;
  DLiteMappingPluginIter iter;

  dlite_mapping_plugin_init_iter(&iter);

  /* Ensure that no input URI equals output URI */
  assert(!map_get(inputs, output_uri));

  /* Ensure that this not an already known dead end */
  assert(!map_get(dead_ends, output_uri));

  /* Add output_uri to visited */
  assert(!map_get(visited, output_uri));
  map_set(visited, output_uri, 0);

  /* Find cheapest mapping to output_api */
  while ((api = dlite_mapping_plugin_next(&iter))) {
    int ignore = 0;
    int cost = 0;
    if (strcmp(output_uri, api->output_uri) != 0) continue;

    /* avoid infinite cyclic loops and known dead ends */
    for (i=0; i < api->ninput; i++) {
      if (map_get(visited, api->input_uris[i]) ||
          map_get(dead_ends, api->input_uris[i])) {
        ignore = 1;
        break;
      }
    }
    if (ignore) continue;

    /* avoid mappings that depends on input that cannot be realised and
       calculate cost */
    for (i=0; i < api->ninput; i++) {
      if (!map_get(inputs, api->input_uris[i])) {
        DLiteMapping *mapping=NULL, **mappingp=NULL;
        if (!(mappingp = map_get(created, api->input_uris[i])) &&
            !(mapping = mapping_create_rec(api->input_uris[i], inputs,
                                           visited, created, dead_ends))) {
          ignore = 1;
          break;
        } else {
          if (!mapping) mapping = *mappingp;
          assert(mapping->cost >= 0);
          cost += mapping->cost;
        }
      }
    }
    if (ignore) continue;

    if (lowest_cost < 0 || cost < lowest_cost) {
      cheapest = api;
      lowest_cost = cost;
    }
  }
  if (!(api = cheapest)) goto fail;

  /* create mapping */
  assert(strcmp(output_uri, api->output_uri) == 0);
  if (!(m = calloc(1, sizeof(DLiteMapping)))) FAIL("allocation failure");
  m->name = api->name;
  m->output_uri = api->output_uri;
  m->ninput = api->ninput;
  for (i=0; i < api->ninput; i++) {
    if (!map_get(inputs, api->input_uris[i])) {
      DLiteMapping **p = map_get(created, api->input_uris[i]);
      assert(p);
      m->input_maps[i] = *p;
      assert(m->input_maps[i]);
    } else
      m->input_uris[i] = api->input_uris[i];
  }
  m->cost = lowest_cost;

  retval = m;
 fail:
  map_remove(visited, output_uri);
  if (!retval) map_set(dead_ends, output_uri, NULL);
  return retval;
}


/*
  Returns a new nested mapping structure describing how `n` input
  instances of metadata `input_uris` can be mapped to `output_uri`.
 */
DLiteMapping *mapping_create(const char *output_uri, const char **input_uris,
                             int n)
{
  int i, *p;
  Ints inputs;
  Mappings visited, created, dead_ends;
  DLiteMapping *m=NULL, *retval=NULL;
  map_iter_t iter;
  const char *key;

  map_init(&inputs);
  map_init(&visited);
  map_init(&created);
  map_init(&dead_ends);

  /* Check that all input_uris are unique */
  for (i=0; i<n; i++) {
    if (map_get(&inputs, input_uris[i]))
      FAIL1("more than one mapping input of the same metadata: %s",
            input_uris[i]);
    map_set(&inputs, input_uris[i], i);
  }

  if ((p = map_get(&inputs, output_uri))) {
    /* The trivial case - one of the input URIs equals output URI. */
    if (!(m = calloc(1, sizeof(DLiteMapping))))
      FAIL("allocation failure");
    m->name = NULL;
    m->output_uri = strdup(output_uri);
    m->ninput = 1;
    if (!(m->input_maps = calloc(1, sizeof(DLiteMapping))))
      FAIL("allocation failure");
    if (!(m->input_uris = calloc(1, sizeof(char *))))
      FAIL("allocation failure");
    m->input_uris[0] = strdup(output_uri);

  } else {
    //map_set(&outputs, output_uri, 0);
    m = mapping_create_rec(output_uri, &inputs, &visited, &created, &dead_ends);
  }

  retval = m;
 fail:

  /* Free all created mappings not in retval */
  mapping_remove_rec(retval, &created);
  iter = map_iter(&created);
  while ((key = map_next(&created, &iter))) {
    DLiteMapping **mp = map_get(&created, key);
    assert(mp && *mp);
    free(*mp);
  }

  map_deinit(&inputs);
  map_deinit(&visited);
  map_deinit(&created);
  map_deinit(&dead_ends);
  if (!retval && m) mapping_free(m);
  return retval;
}



/*
  Frees a nested mapping tree.
*/
void mapping_free(DLiteMapping *m)
{
  int i;
  for (i=0; i < m->ninput; i++) {
    assert(m->input_maps[i] || m->input_uris[i]);
    assert(!(m->input_maps[i] && m->input_uris[i]));
    if (m->input_maps[i]) mapping_free((DLiteMapping *)m->input_maps[i]);
  }
  free(m);
}
