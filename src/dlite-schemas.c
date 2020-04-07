#include <stddef.h>
#include <stdlib.h>

#include "dlite.h"
#include "dlite-collection.h"
#include "dlite-transaction.h"
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
  "89cc72b5-1ced-54eb-815b-8fffc16c42d1",        /* uuid (corresponds to uri) */
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
  "BasicMetadataSchema",                         /* schema_name */
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
  "57742a73-ba65-5797-aebf-c1a270c4d02b",     /* uuid (corresponds to uri) */
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
  "EntitySchema",                             /* schema_name */
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
  {"nrelations",  "Number of relations."},
};
static int collection_schema_prop_relations_dims[] = {0};
static DLiteProperty collection_schema_properties[] = {
  {
  "relations",                               /* name */
  dliteRelation,                             /* type */
  sizeof(DLiteRelation),                     /* size */
  1,                                         /* ndims */
  collection_schema_prop_relations_dims,     /* dims */
  NULL,                                      /* unit */
  "Array of relations (subject, predicate, "
  "object, relation-id)."                    /* description */
  }
};
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
  "a2e66e0e-d733-5067-b987-6b5e5d54fb12",        /* uuid (corresponds to uri) */
  DLITE_COLLECTION_SCHEMA,                       /* uri */
  1,                                             /* refcount, never free */
  (DLiteMeta *)&basic_metadata_schema,           /* meta */

  1,                                             /* ndimensions */
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
  1,                                             /* ndims */
  1,                                             /* nprops */
  0,                                             /* nrels */
  /* -- value of each property */
  "CollectionSchema",                            /* schema_name */
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



/***********************************************************
 * transaction_schema
 ***********************************************************/
static DLiteDimension transaction_schema_dimensions[] = {
  {"ndimensions", "Number of dimensions."},
  {"nproperties", "Number of properties."}
};
static int transaction_schema_prop_dimensions_dims[] = {0};
static int transaction_schema_prop_properties_dims[] = {1};
static DLiteProperty transaction_schema_properties[] = {
  {
   "name",                                    /* name */
   dliteStringPtr,                            /* type */
   sizeof(char *),                            /* size */
   0,                                         /* ndims */
   NULL,                                      /* dims */
   NULL,                                      /* unit */
   "Transaction name."                        /* description */
  },
  {
   "version",                                 /* name */
   dliteStringPtr,                            /* type */
   sizeof(char *),                            /* size */
   0,                                         /* ndims */
   NULL,                                      /* dims */
   NULL,                                      /* unit */
   "Transaction version."                     /* description */
  },
  {
   "namespace",                               /* name */
   dliteStringPtr,                            /* type */
   sizeof(char *),                            /* size */
   0,                                         /* ndims */
   NULL,                                      /* dims */
   NULL,                                      /* unit */
   "Transaction namespace."                   /* description */
  },
  {
   "parent_id",                               /* name */
   dliteStringPtr,                            /* type */
   sizeof(char *),                            /* size */
   0,                                         /* ndims */
   NULL,                                      /* dims */
   NULL,                                      /* unit */
   "Id of parent transaction."                /* description */
  },
  {
   "parent_hash",                             /* name */
   dliteStringPtr,                            /* type */
   sizeof(char *),                            /* size */
   0,                                         /* ndims */
   NULL,                                      /* dims */
   NULL,                                      /* unit */
   "Hash value of parent transaction."        /* description */
  },
  {
   "description",                             /* name */
   dliteStringPtr,                            /* type */
   sizeof(char *),                            /* size */
   0,                                         /* ndims */
   NULL,                                      /* dims */
   NULL,                                      /* unit */
   "Description of transaction."              /* description */
  },
  {
   "dimensions",                              /* name */
   dliteDimension,                            /* type */
   sizeof(DLiteDimension),                    /* size */
   1,                                         /* ndims */
   transaction_schema_prop_dimensions_dims,   /* dims */
   NULL,                                      /* unit */
   "Transaction dimensions."                  /* description */
  },
  {
   "properties",                              /* name */
   dliteProperty,                             /* type */
   sizeof(DLiteProperty),                     /* size */
   1,                                         /* ndims */
   transaction_schema_prop_properties_dims,   /* dims */
   NULL,                                      /* unit */
   "Transaction properties."                  /* description */
  }
};
static struct _TransactionSchema {
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
  size_t offsets[8];
} transaction_schema = {
  /* -- header */
  "dd4a20a7-a110-561b-9710-90cccdb4d9d6",     /* uuid (corresponds to uri) */
  DLITE_TRANSACTION_SCHEMA,                   /* uri */
  1,                                          /* refcount, never free */
  (DLiteMeta *)&basic_metadata_schema,        /* meta */

  2,                                          /* ndimensions */
  8,                                          /* nproperties */
  0,                                          /* nrelations */

  transaction_schema_dimensions,              /* dimensions */
  transaction_schema_properties,              /* properties */
  NULL,                                       /* relations */

  0,                                          /* headersize */
  dlite_transaction_init,                     /* init */
  dlite_transaction_deinit,                   /* deinit */

  0,                                          /* dimoffset */
  NULL,                                       /* propoffsets */
  0,                                          /* reloffset */
  0,                                          /* pooffset */
  /* -- length of each dimention */
  2,                                          /* ndims */
  8,                                          /* nprops */
  0,                                          /* nrels */
  /* -- value of each property */
  "TransactionSchema",                        /* schema_name */
  "0.1",                                      /* schema_version */
  "http://meta.sintef.no",                    /* schema_namespace */
  "Meta-metadata description of transaction.",/* schema_description */
  transaction_schema_dimensions,              /* schema_dimensions */
  transaction_schema_properties,              /* schema_properties */
  NULL,                                       /* schema_relations */
  /* -- value of each relation */
  /* -- array of memory offsets to each instance property */
  {0, 0, 0, 0, 0, 0, 0, 0}                    /* offsets */
};



/**************************************************************
 * Functions returning a pointer to above static schemas
 **************************************************************/

const DLiteMeta *dlite_get_basic_metadata_schema()
{
  return (DLiteMeta *)&basic_metadata_schema;
}

const DLiteMeta *dlite_get_entity_schema()
{
  return (DLiteMeta *)&entity_schema;
}

const DLiteMeta *dlite_get_collection_schema()
{
  return (DLiteMeta *)&collection_schema;
}

const DLiteMeta *dlite_get_transaction_schema()
{
  return (DLiteMeta *)&transaction_schema;
}
