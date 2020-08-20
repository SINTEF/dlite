#include <stdio.h>

#include "config.h"

#include "minunit/minunit.h"
#include "dlite.h"

DLiteMetaModel *model;


/* Internal struct used to hold data values in DLiteMetaModel */
typedef struct {
  char *name;         /* Name of corresponding property in metadata */
  void *data;         /* Pointer to the data */
} Value;

struct _DLiteMetaModel {
  char *uri;
  DLiteMeta *meta;
  char *iri;

  size_t *dimvalues;  /* Array of dimension values used for instance creation */

  size_t nvalues;     /* Number of property values */
  Value *values;      /* Pointer to property values, typically description
                         for metadata and data values for data instances. */

  /* Special storage of dimension, property and relation values */
  size_t ndims;
  size_t nprops;
  size_t nrels;
  DLiteDimension *dims;
  DLiteProperty *props;
  DLiteRelation *rels;
};

void show(void)
{

  size_t i;
  printf("\n===\n");
  printf("uri: %s\n", model->uri);
  printf("meta: %s\n", model->meta->uri);
  printf("iri: %s\n", model->iri);

  printf("dimvalues:");
  for (i=0; i<model->meta->_ndimensions; i++)
    printf("  %lu", model->dimvalues[i]);
  printf("\n");

  printf("nvalues: %lu\n", model->nvalues);
  printf("values:\n");
  for (i=0; i<model->nvalues; i++) {
    printf("  name=%s  data=(%p) \n",
           model->values[i].name, model->values[i].data);
    printf("  --> (%p) \"%s\"\n",
           *((void **)(model->values[i].data)),
           *((char **)(model->values[i].data)));
  }
  printf("ndims: %lu\n", model->ndims);
  printf("nprops: %lu\n", model->nprops);
  printf("nrels: %lu\n", model->nrels);
}



MU_TEST(test_metamodel_create)
{
  model = dlite_metamodel_create("http://meta.sintef.no/0.1/Vehicle",
                                 DLITE_ENTITY_SCHEMA, NULL);
  mu_check(model);
}

MU_TEST(test_metamodel_add_value)
{
  char *descr = strdup("A vehicle like car, bike, etc...");
  printf("\n*** descr=%p --> (%p) \"%s\"\n", (void *)(&descr),
         *((void **)&descr),
         *((char **)&descr));
  mu_assert_int_eq(0, dlite_metamodel_add_value(model, "description", &descr));
  show();
}

//MU_TEST(test_metamodel_add_dimension)
//{
//  char *descr = "Number of EU controls it has been through.";
//  mu_assert_int_eq(0, dlite_metamodel_add_dimension(model, "ncontrols", descr));
//}
//
//MU_TEST(test_metamodel_add_property)
//{
//  mu_assert_int_eq(0, dlite_metamodel_add_property(model, "branch",
//                                                   dliteFixString, 32,
//                                                   NULL, NULL,
//                                                   "Branch of the vehicle."));
//}

//MU_TEST(test_metamodel_create_meta)
//{
//  show();
//
//  //DLiteMeta *meta = dlite_meta_create_from_metamodel(model);
//  //mu_check(meta);
//  //
//  //dlite_instance_print((DLiteInstance *)meta);
//  //
//  //dlite_meta_decref(meta);
//
//}

MU_TEST(test_metamodel_free)
{
  show();
  dlite_metamodel_free(model);
}



/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_metamodel_create);     /* setup */
  MU_RUN_TEST(test_metamodel_add_value);
  //MU_RUN_TEST(test_metamodel_add_dimension);
  //MU_RUN_TEST(test_metamodel_add_property);
  //MU_RUN_TEST(test_metamodel_create_meta);
  MU_RUN_TEST(test_metamodel_free);      /* tear down */
}



int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
