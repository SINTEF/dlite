#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "minunit/minunit.h"
#include "integers.h"
#include "boolean.h"
#include "dlite.h"
#include "dlite-entity.h"


#include "config.h"

char *datafile = "myentity.h5";
char *id = "myentity";
char *uuid = "71c579d8-7567-51d4-a992-7a8c7a0ea47e";

DLiteEntity entity;


/***************************************************************
 * Test entity
 ***************************************************************/

MU_TEST(test_entity_init)
{
  size_t i;

  /* Assign an entity the hard way... */
  memset(&entity, 0, sizeof(entity));
  memcpy(entity.uuid, uuid, sizeof(entity.uuid));
  entity.url = strdup("http://www.sintef.no/meta/dlite/0.1/EntitySchema");
  entity.meta = NULL;

  entity.ndims = 2;
  entity.nprops = 4;

  entity.name = strdup("EntitySchema");
  entity.version = strdup("0.1");
  entity.namespace = strdup("http://www.sintef.no/meta/dlite");
  entity.description = NULL;

  entity.dimnames = calloc(entity.ndims, sizeof(char **));
  entity.dimnames[0] = strdup("M");
  entity.dimnames[1] = strdup("N");
  entity.dimdescr = calloc(entity.ndims, sizeof(char **));
  entity.dimdescr[0] = strdup("dim M");
  entity.dimdescr[1] = strdup("dim N");

  entity.propnames = calloc(entity.nprops, sizeof(char **));
  entity.propnames[0] = strdup("a-string");
  entity.propnames[1] = strdup("a-float");
  entity.propnames[2] = strdup("an-int-array");
  entity.propnames[3] = strdup("a-string-array");
  entity.proptypes = calloc(entity.nprops, sizeof(DLiteType));
  entity.proptypes[0] = dliteStringPtr;
  entity.proptypes[1] = dliteFloat;
  entity.proptypes[2] = dliteInt;
  entity.proptypes[3] = dliteStringPtr;
  entity.propsizes = calloc(entity.nprops, sizeof(size_t));
  entity.propsizes[0] = sizeof(char *);
  entity.propsizes[1] = sizeof(double);
  entity.propsizes[2] = sizeof(int *);
  entity.propsizes[3] = sizeof(char **);
  entity.propndims = calloc(entity.nprops, sizeof(size_t));
  entity.propndims[0] = 0;
  entity.propndims[1] = 0;
  entity.propndims[2] = 2;
  entity.propndims[3] = 1;
  entity.propdims = calloc(entity.nprops, sizeof(int *));
  for (i=0; i<entity.nprops; i++)
    if (entity.propndims[i])
      entity.propdims[i] = calloc(entity.propndims[i], sizeof(size_t));
  entity.propdims[2][0] = 1;  /* N */
  entity.propdims[2][1] = 0;  /* M */
  entity.propdims[3][0] = 1;  /* N */
  entity.propdescr = calloc(entity.nprops, sizeof(char *));
  entity.propdescr[0] = strdup("A string.");
  entity.propdescr[1] = strdup("A double.");
  entity.propdescr[2] = strdup("An 2D int array.");
  entity.propdescr[3] = strdup("A string array.");
  entity.propunits = calloc(entity.nprops, sizeof(char *));
  entity.propunits[0] = NULL;
  entity.propunits[1] = strdup("m");
  entity.propunits[2] = strdup("numbers");
  entity.propunits[3] = NULL;
}


MU_TEST(test_entity_postinit)
{
  mu_check(dlite_meta_postinit((DLiteMeta *)&entity) == 0);
}


MU_TEST(test_entity_save)
{
  DLiteStorage *s;
  mu_check((s = dlite_storage_open("hdf5", datafile, "w")));
  mu_check(dlite_entity_save(s, &entity) == 0);
  mu_check(dlite_storage_close(s) == 0);
}


MU_TEST(test_entity_load)
{
  DLiteStorage *s;
  DLiteEntity *e;
  mu_check((s = dlite_storage_open("hdf5", datafile, "r")));
  mu_check((e = dlite_entity_load(s, id)));
  mu_check(dlite_storage_close(s) == 0);

  mu_check((s = dlite_storage_open("hdf5", "myentity2.h5", "w")));
  mu_check(dlite_entity_save(s, &entity) == 0);
  mu_check(dlite_storage_close(s) == 0);
}


MU_TEST(test_entity_clear)
{
  dlite_entity_clear(&entity);
}


/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_entity_init);      /* setup */
  MU_RUN_TEST(test_entity_postinit);  /* setup */
  MU_RUN_TEST(test_entity_save);
  MU_RUN_TEST(test_entity_load);

  MU_RUN_TEST(test_entity_clear);     /* tear down */
}



int main(int argc, char *argv[])
{
  if (argc > 1) datafile = argv[1];
  if (argc > 2) id = argv[2];
  printf("datafile: '%s'\n", datafile);
  printf("id:       '%s'\n", id);

  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
