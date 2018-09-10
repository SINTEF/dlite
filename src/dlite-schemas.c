#include <stddef.h>
#include <stdlib.h>

#include "dlite.h"
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
  "d1ce1dc7-e2b1-51a1-8957-3277815aed18",        /* uuid (corresponds to uri) */
  "http://meta.sintef.no/0.1/basic_metadata_schema",  /* uri  */
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
  "0.3",                                         /* schema_version */
  "http://meta.emmc.eu",                         /* schema_namespace */
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
  "d1ce1dc7-e2b1-51a1-8957-3277815aed18",     /* uuid (corresponds to uri) */
  "http://meta.sintef.no/0.1/entity_schema",  /* uri  */
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
  "http://meta.emmc.eu",                      /* schema_namespace */
  "Meta-metadata description an entity.",     /* schema_description */
  entity_schema_dimensions,                   /* schema_dimensions */
  entity_schema_properties,                   /* schema_properties */
  NULL,                                       /* schema_relations */
  /* -- value of each relation */
  /* -- array of memory offsets to each instance property */
  {0, 0, 0, 0, 0, 0}                          /* offsets */
};





DLiteMeta *dlite_EntitySchema = (DLiteMeta *)&entity_schema;
