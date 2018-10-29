#include <stddef.h>
#include <stdlib.h>

#include "dlite.h"
#include "dlite-collection.h"
#include "dlite-schemas.h"



/***********************************************************
 * basic_metadata_schema
 ***********************************************************/
static DLiteDimension basic_metadata_schema_dimensions[] = {
  {"ndimensions", "Number of dimensions."},
  {"nproperties", "Number of properties."},
  {"nrelations",  "Number of relations."}
};
static int basic_metadata_schema_prop_dimensions_dims[] = {0};
static int basic_metadata_schema_prop_properties_dims[] = {1};
static int basic_metadata_schema_prop_relations_dims[] = {2};
static DLiteProperty basic_metadata_schema_properties[] = {
  {
   "name",                                    /* name */
   dliteStringPtr,                            /* type */
   sizeof(char *),                            /* size */
   0,                                         /* ndims */
   NULL,                                      /* dims */
   NULL,                                      /* unit */
   "Schema name."                             /* description */
  },
  {
   "version",                                 /* name */
   dliteStringPtr,                            /* type */
   sizeof(char *),                            /* size */
   0,                                         /* ndims */
   NULL,                                      /* dims */
   NULL,                                      /* unit */
   "Schema version."                          /* description */
  },
  {
   "namespace",                               /* name */
   dliteStringPtr,                            /* type */
   sizeof(char *),                            /* size */
   0,                                         /* ndims */
   NULL,                                      /* dims */
   NULL,                                      /* unit */
   "Schema namespace."                        /* description */
  },
  {
   "description",                             /* name */
   dliteStringPtr,                            /* type */
   sizeof(char *),                            /* size */
   0,                                         /* ndims */
   NULL,                                      /* dims */
   NULL,                                      /* unit */
   "Description of schema."                   /* description */
  },
  {
   "dimensions",                              /* name */
   dliteDimension,                            /* type */
   sizeof(DLiteDimension),                    /* size */
   1,                                         /* ndims */
   basic_metadata_schema_prop_dimensions_dims,/* dims */
   NULL,                                      /* unit */
   "Defines schema dimensions."               /* description */
  },
  {
   "properties",                              /* name */
   dliteProperty,                             /* type */
   sizeof(DLiteProperty),                     /* size */
   1,                                         /* ndims */
   basic_metadata_schema_prop_properties_dims,/* dims */
   NULL,                                      /* unit */
   "Defines schema properties."               /* description */
  },
  {
   "relations",                               /* name */
   dliteRelation,                             /* type */
   sizeof(DLiteRelation),                     /* size */
   1,                                         /* ndims */
   basic_metadata_schema_prop_relations_dims, /* dims */
   NULL,                                      /* unit */
   "Defines schema relations."                /* description */
  }
};
//static DLiteRelation basic_metadata_schema_relations[] = {
//  {NULL, NULL, NULL, NULL}
//};
static struct _BasicMetadataSchema {
  /* -- header */
  DLiteMeta_HEAD
  /* -- length of each dimension */
  size_t ndims;
  size_t nprops;
  size_t nrels;
  /* -- value of each property */
  char *schema_name;
  char *schema_version;
  char *schema_namespace;
  char *schema_description;
  DLiteDimension *schema_dimensions;
  DLiteProperty  *schema_properties;
  DLiteRelation  *schema_relations;
  /* -- value of each relation */
  /* -- array of memory offsets to each instance property */
  size_t offsets[7];
} basic_metadata_schema = {
  /* -- header */
  "c25c3bb4-2835-5b30-9046-a0fb971e86c2",        /* uuid (corresponds to uri) */
  DLITE_BASIC_METADATA_SCHEMA,                   /* uri */
  1,                                             /* refcount, never free */
  (DLiteMeta *)&basic_metadata_schema,           /* meta */

  3,                                             /* ndimensions */
  7,                                             /* nproperties */
  0,                                             /* nrelations */

  basic_metadata_schema_dimensions,              /* dimensions */
  basic_metadata_schema_properties,              /* properties */
  NULL,                                          /* relations */

  0,                                             /* headersize */
  NULL,                                          /* init */
  NULL,                                          /* deinit */

  offsetof(struct _BasicMetadataSchema, ndims),  /* dimoffset */
  (size_t *)basic_metadata_schema.offsets,       /* propoffsets */
  offsetof(struct _BasicMetadataSchema, offsets),/* reloffset */
  offsetof(struct _BasicMetadataSchema, offsets),/* poofset */
  /* -- length of each dimention */
  3,                                             /* ndims */
  7,                                             /* nprops */
  0,                                             /* nrels */
  /* -- value of each property */
  "basic_metadata_schema",                       /* schema_name */
  "0.1",                                         /* schema_version */
  "http://meta.sintef.no",                       /* schema_namespace */
  "Meta-metadata description an entity.",        /* schema_description */
  basic_metadata_schema_dimensions,              /* schema_dimensions */
  basic_metadata_schema_properties,              /* schema_properties */
  NULL,                                          /* schema_relations */
  /* -- value of each relation */
  /* -- array of memory offsets to each instance property */
  {                                              /* offsets */
    offsetof(struct _BasicMetadataSchema, schema_name),
    offsetof(struct _BasicMetadataSchema, schema_version),
    offsetof(struct _BasicMetadataSchema, schema_namespace),
    offsetof(struct _BasicMetadataSchema, schema_description),
    offsetof(struct _BasicMetadataSchema, schema_dimensions),
    offsetof(struct _BasicMetadataSchema, schema_properties),
    offsetof(struct _BasicMetadataSchema, schema_relations)
  }
};



/***********************************************************
 * entity_schema
 ***********************************************************/
static DLiteDimension entity_schema_dimensions[] = {
  {"ndimensions", "Number of dimensions."},
  {"nproperties", "Number of properties."}
};
static int entity_schema_prop_dimensions_dims[] = {0};
static int entity_schema_prop_properties_dims[] = {1};
static DLiteProperty entity_schema_properties[] = {
  {
   "name",                                    /* name */
   dliteStringPtr,                            /* type */
   sizeof(char *),                            /* size */
   0,                                         /* ndims */
   NULL,                                      /* dims */
   NULL,                                      /* unit */
   "Entity name."                             /* description */
  },
  {
   "version",                                 /* name */
   dliteStringPtr,                            /* type */
   sizeof(char *),                            /* size */
   0,                                         /* ndims */
   NULL,                                      /* dims */
   NULL,                                      /* unit */
   "Entity version."                          /* description */
  },
  {
   "namespace",                               /* name */
   dliteStringPtr,                            /* type */
   sizeof(char *),                            /* size */
   0,                                         /* ndims */
   NULL,                                      /* dims */
   NULL,                                      /* unit */
   "Entity namespace."                        /* description */
  },
  {
   "description",                             /* name */
   dliteStringPtr,                            /* type */
   sizeof(char *),                            /* size */
   0,                                         /* ndims */
   NULL,                                      /* dims */
   NULL,                                      /* unit */
   "Description of entity."                   /* description */
  },
  {
   "dimensions",                              /* name */
   dliteDimension,                            /* type */
   sizeof(DLiteDimension),                    /* size */
   1,                                         /* ndims */
   entity_schema_prop_dimensions_dims,        /* dims */
   NULL,                                      /* unit */
   "Entity dimensions."                       /* description */
  },
  {
   "properties",                              /* name */
   dliteProperty,                             /* type */
   sizeof(DLiteProperty),                     /* size */
   1,                                         /* ndims */
   entity_schema_prop_properties_dims,        /* dims */
   NULL,                                      /* unit */
   "Entity properties."                       /* description */
  }
};
static struct _EntitySchema {
  /* -- header */
  DLiteMeta_HEAD
  /* -- length of each dimension */
  size_t ndims;
  size_t nprops;
  size_t nrels;
  /* -- value of each property */
  char *schema_name;
  char *schema_version;
  char *schema_namespace;
  char *schema_description;
  DLiteDimension *schema_dimensions;
  DLiteProperty  *schema_properties;
  DLiteRelation  *schema_relation;
  /* -- value of each relation */
  /* -- array of memory offsets to each instance property */
  size_t offsets[6];
} entity_schema = {
  /* -- header */
  "477a5277-b33b-58eb-ad35-6f2e4ac1e64a",     /* uuid (corresponds to uri) */
  DLITE_ENTITY_SCHEMA,                        /* uri */
  1,                                          /* refcount, never free */
  (DLiteMeta *)&basic_metadata_schema,        /* meta */

  2,                                          /* ndimensions */
  6,                                          /* nproperties */
  0,                                          /* nrelations */

  entity_schema_dimensions,                   /* dimensions */
  entity_schema_properties,                   /* properties */
  NULL,                                       /* relations */

  0,                                          /* headersize */
  NULL,                                       /* init */
  NULL,                                       /* deinit */

  0,                                          /* dimoffset */
  NULL,                                       /* propoffsets */
  0,                                          /* reloffset */
  0,                                          /* pooffset */
  /* -- length of each dimention */
  2,                                          /* ndims */
  6,                                          /* nprops */
  0,                                          /* nrels */
  /* -- value of each property */
  "entity_schema",                            /* schema_name */
  "0.3",                                      /* schema_version */
  "http://meta.sintef.no",                    /* schema_namespace */
  "Meta-metadata description an entity.",     /* schema_description */
  entity_schema_dimensions,                   /* schema_dimensions */
  entity_schema_properties,                   /* schema_properties */
  NULL,                                       /* schema_relations */
  /* -- value of each relation */
  /* -- array of memory offsets to each instance property */
  {0, 0, 0, 0, 0, 0}                          /* offsets */
};



/**************************************************************
 * collection_schema
 **************************************************************/

static DLiteDimension collection_schema_dimensions[] = {
  //{"n-dimensions", "Number of common dimmensions."},
  //{"n-instances",  "Number of instances added to the collection."},
  //{"n-dim-maps",   "Number of dimension maps."},
  {"n-relations",  "Number of relations."},
  {"n-rel-items",  "Number of items in a relation - always 4 (s,p,o,id)."}
};
static int collection_schema_prop_relations_dims[] = {0, 1};
static DLiteProperty collection_schema_properties[] = {
  {
  "relations",                               /* name */
  dliteStringPtr,                            /* type */
  sizeof(char *),                            /* size */
  2,                                         /* ndims */
  collection_schema_prop_relations_dims,     /* dims */
  NULL,                                      /* unit */
  "Array of relations (subject, predicate, "
  "object, relation-id)."                    /* description */
  }
};
//static size_t collection_schema_propoffsets[] = {
//  offsetof(DLiteCollection, relations)
//};
static struct _CollectionSchema {
  /* -- header */
  DLiteMeta_HEAD
  /* -- length of each dimension */
  size_t ndims;
  size_t nprops;
  size_t nrels;
  /* -- value of each property */
  char *schema_name;
  char *schema_version;
  char *schema_namespace;
  char *schema_description;
  DLiteDimension *schema_dimensions;
  DLiteProperty  *schema_properties;
  DLiteRelation  *schema_relations;
  /* -- value of each relation */
  /* -- array of memory offsets to each instance property */
  size_t offsets[1];
} collection_schema = {
  /* -- header */
  "b5f55af0-123d-5ce4-95c0-28d068c23c44",        /* uuid (corresponds to uri) */
  DLITE_COLLECTION_SCHEMA,                       /* uri */
  1,                                             /* refcount, never free */
  (DLiteMeta *)&basic_metadata_schema,           /* meta */

  2,                                             /* ndimensions */
  1,                                             /* nproperties */
  0,                                             /* nrelations */

  collection_schema_dimensions,                  /* dimensions */
  collection_schema_properties,                  /* properties */
  NULL,                                          /* relations */

  offsetof(DLiteCollection, nrelations),         /* headersize */
  dlite_collection_init,                         /* init */
  dlite_collection_deinit,                       /* deinit */

  0,                                             /* dimoffset */
  NULL,                                          /* propoffsets */
  0,                                             /* reloffset */
  0,                                             /* pooffset */
  /* -- length of each dimention */
  2,                                             /* ndims */
  1,                                             /* nprops */
  0,                                             /* nrels */
  /* -- value of each property */
  "collection_schema",                           /* schema_name */
  "0.1",                                         /* schema_version */
  "http://meta.sintef.no",                       /* schema_namespace */
  "Meta-metadata description a collection.",     /* schema_description */
  collection_schema_dimensions,                  /* schema_dimensions */
  collection_schema_properties,                  /* schema_properties */
  NULL,                                          /* schema_relations */
  /* -- value of each relation */
  /* -- array of memory offsets to each instance property */
  {0}                                            /* offsets */
};



/**************************************************************
 * Exposed pointers to schemas
 **************************************************************/
const DLiteMeta * const dlite_BasicMetadataSchema =
  (DLiteMeta *)&basic_metadata_schema;
const DLiteMeta * const dlite_EntitySchema =
  (DLiteMeta *)&entity_schema;
const DLiteMeta * const dlite_CollectionSchema =
  (DLiteMeta *)&collection_schema;
