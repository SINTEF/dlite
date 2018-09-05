#include <stdlib.h>

#include "dlite.h"



static DLiteDimension entity_schema_dimensions[] = {
  {"ndimensions", "Number of dimensions."},
  {"nproperties", "Number of properties."}
};
static int entity_schema_prop0_dims[] = {0};
static int entity_schema_prop1_dims[] = {1};
static DLiteProperty entity_schema_properties[] = {
  {
    "dimensions",                              /* name */
    dliteDimension,                            /* type */
    sizeof(DLiteDimension),                    /* size */
    1,                                         /* ndims */
    entity_schema_prop0_dims,                  /* dims */
    NULL,                                      /* unit */
    "Name and description of each dimension."  /* description */
  },
  {
    "properties",                              /* name */
    dliteProperty,                             /* type */
    sizeof(DLiteProperty),                     /* size */
    1,                                         /* ndims */
    entity_schema_prop1_dims,                  /* dims */
    NULL,                                      /* unit */
    "Name and description of each property."   /* description */
  }
};
struct _DLiteEntitySchema {
  DLiteMeta_HEAD
  /* size of each dimension */
  size_t ndims;
  size_t nprops;
  /* property offsets */
  size_t offset_dims;
  size_t offset_props;
  /* property values, use pointers because they are arrays */
  DLiteDimension *dims;
  DLiteProperty *props;
} entity_schema = {
  "d1ce1dc7-e2b1-51a1-8957-3277815aed18",           /* uuid (from uri) */
  "http://meta.sintef.no/0.1/entity_schema",        /* uri  */
  1,                                                /* refcount, never free */
  (DLiteMeta *)&entity_schema,                      /* meta */
  "Schema for Entities",                            /* description */

  /* These will be initialised correctly the first time they are needed */
  0,                                                /* size */
  0,                                                /* structsize */
  0,                                                /* dimoffset */
  0,                                                /* propoffset */
  0,                                                /* reloffset */

  NULL,                                             /* init */
  NULL,                                             /* deinit */
  //NULL,                                           /* loadprop */
  //NULL,                                           /* saveprop */
  entity_schema_dimensions,                         /* dimensions */
  entity_schema_properties,                         /* properties */
  NULL,                                             /* relations */
  2,                                                /* ndimensions, dubl. */
  2,                                                /* nproperties, dubl. */
  0,                                                /* nrelations, dubl. */
  //dliteIsMeta,                                    /* flags */

  2,                                                /* ndims (dim0) */
  2,                                                /* nprops (dim1) */
  offsetof(struct _DLiteEntitySchema, dimensions),  /* offset_dims */
  offsetof(struct _DLiteEntitySchema, properties),  /* offset_props */
  entity_schema_dimensions,                         /* dims (prop0) */
  entity_schema_properties                          /* props (prop1) */
};



const DLiteMeta *dlite_EntitySchema = &entity_schema;
