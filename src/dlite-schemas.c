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
static char *basic_metadata_schema_prop_dimensions_dims[] = {"ndimensions"};
static char *basic_metadata_schema_prop_properties_dims[] = {"nproperties"};
static char *basic_metadata_schema_prop_relations_dims[] = {"nrelations"};
static DLiteProperty basic_metadata_schema_properties[] = {
  {
   "name",                                    /* name */
   dliteStringPtr,                            /* type */
   sizeof(char *),                            /* size */
   0,                                         /* ndims */
   NULL,                                      /* dims */
   NULL,                                      /* unit */
   NULL,                                      /* iri */
   "Schema name."                             /* description */
  },
  {
   "version",                                 /* name */
   dliteStringPtr,                            /* type */
   sizeof(char *),                            /* size */
   0,                                         /* ndims */
   NULL,                                      /* dims */
   NULL,                                      /* unit */
   NULL,                                      /* iri */
   "Schema version."                          /* description */
  },
  {
   "namespace",                               /* name */
   dliteStringPtr,                            /* type */
   sizeof(char *),                            /* size */
   0,                                         /* ndims */
   NULL,                                      /* dims */
   NULL,                                      /* unit */
   NULL,                                      /* iri */
   "Schema namespace."                        /* description */
  },
  {
   "description",                             /* name */
   dliteStringPtr,                            /* type */
   sizeof(char *),                            /* size */
   0,                                         /* ndims */
   NULL,                                      /* dims */
   NULL,                                      /* unit */
   NULL,                                      /* iri */
   "Description of schema."                   /* description */
  },
  {
   "dimensions",                              /* name */
   dliteDimension,                            /* type */
   sizeof(DLiteDimension),                    /* size */
   1,                                         /* ndims */
   basic_metadata_schema_prop_dimensions_dims,/* dims */
   NULL,                                      /* unit */
   NULL,                                      /* iri */
   "Defines schema dimensions."               /* description */
  },
  {
   "properties",                              /* name */
   dliteProperty,                             /* type */
   sizeof(DLiteProperty),                     /* size */
   1,                                         /* ndims */
   basic_metadata_schema_prop_properties_dims,/* dims */
   NULL,                                      /* unit */
   NULL,                                      /* iri */
   "Defines schema properties."               /* description */
  },
  {
   "relations",                               /* name */
   dliteRelation,                             /* type */
   sizeof(DLiteRelation),                     /* size */
   1,                                         /* ndims */
   basic_metadata_schema_prop_relations_dims, /* dims */
   NULL,                                      /* unit */
   NULL,                                      /* iri */
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
  /* -- array property dimension values */
  size_t _propdims[3];
  /* -- array of first property dimension  */
  size_t _propdiminds[7];
  /* -- array of memory offsets to each instance property */
  size_t _propoffsets[7];
} basic_metadata_schema = {
  /* -- header */
  "89cc72b5-1ced-54eb-815b-8fffc16c42d1",        /* uuid (corresponds to uri) */
  DLITE_BASIC_METADATA_SCHEMA,                   /* uri */
  1,                                             /* refcount, never free */
  (DLiteMeta *)&basic_metadata_schema,           /* meta */
  NULL,                                          /* iri */

  3,                                             /* ndimensions */
  7,                                             /* nproperties */
  0,                                             /* nrelations */

  basic_metadata_schema_dimensions,              /* dimensions */
  basic_metadata_schema_properties,              /* properties */
  NULL,                                          /* relations */

  0,                                             /* headersize */
  NULL,                                          /* init */
  NULL,                                          /* deinit */

  3,                                             /* npropdims */
  (size_t *)basic_metadata_schema._propdiminds, /* propdiminds */

  offsetof(struct _BasicMetadataSchema, ndims),        /* dimoffset */
  (size_t *)basic_metadata_schema._propoffsets,        /* propoffsets */
  offsetof(struct _BasicMetadataSchema, _propdims),    /* reloffset */
  offsetof(struct _BasicMetadataSchema, _propdims),    /* propdimsoffset */
  offsetof(struct _BasicMetadataSchema, _propdiminds), /* propdimindsoffset */
  /* -- length of each dimention */
  3,                                             /* _ndimensions */
  7,                                             /* _nproperties */
  0,                                             /* _nrelations */
  /* -- value of each property */
  "BasicMetadataSchema",                         /* schema_name */
  "0.1",                                         /* schema_version */
  "http://meta.sintef.no",                       /* schema_namespace */
  "Meta-metadata description an entity.",        /* schema_description */
  basic_metadata_schema_dimensions,              /* schema_dimensions */
  basic_metadata_schema_properties,              /* schema_properties */
  NULL,                                          /* schema_relations */
  /* -- value of each relation */
  /* -- array property dimension values */
  {3, 7, 0},                                     /* _propdims */
  /* -- array of first property dimension  */
  {0, 0, 0, 0, 0, 1, 2},                         /* _propdiminds */
  /* -- array of memory offsets to each instance property */
  {                                              /* _propoffsets */
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
static char *entity_schema_prop_dimensions_dims[] = {"ndimensions"};
static char *entity_schema_prop_properties_dims[] = {"nproperties"};
static DLiteProperty entity_schema_properties[] = {
  {
   "name",                                    /* name */
   dliteStringPtr,                            /* type */
   sizeof(char *),                            /* size */
   0,                                         /* ndims */
   NULL,                                      /* dims */
   NULL,                                      /* unit */
   NULL,                                      /* iri */
   "Entity name."                             /* description */
  },
  {
   "version",                                 /* name */
   dliteStringPtr,                            /* type */
   sizeof(char *),                            /* size */
   0,                                         /* ndims */
   NULL,                                      /* dims */
   NULL,                                      /* unit */
   NULL,                                      /* iri */
   "Entity version."                          /* description */
  },
  {
   "namespace",                               /* name */
   dliteStringPtr,                            /* type */
   sizeof(char *),                            /* size */
   0,                                         /* ndims */
   NULL,                                      /* dims */
   NULL,                                      /* unit */
   NULL,                                      /* iri */
   "Entity namespace."                        /* description */
  },
  {
   "description",                             /* name */
   dliteStringPtr,                            /* type */
   sizeof(char *),                            /* size */
   0,                                         /* ndims */
   NULL,                                      /* dims */
   NULL,                                      /* unit */
   NULL,                                      /* iri */
   "Description of entity."                   /* description */
  },
  {
   "dimensions",                              /* name */
   dliteDimension,                            /* type */
   sizeof(DLiteDimension),                    /* size */
   1,                                         /* ndims */
   entity_schema_prop_dimensions_dims,        /* dims */
   NULL,                                      /* unit */
   NULL,                                      /* iri */
   "Entity dimensions."                       /* description */
  },
  {
   "properties",                              /* name */
   dliteProperty,                             /* type */
   sizeof(DLiteProperty),                     /* size */
   1,                                         /* ndims */
   entity_schema_prop_properties_dims,        /* dims */
   NULL,                                      /* unit */
   NULL,                                      /* iri */
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
  /* -- array property dimension values */
  size_t _propdims[2];
  /* -- array of first property dimension  */
  size_t _propdiminds[6];
  /* -- array of memory offsets to each instance property */
  size_t _propoffsets[6];
} entity_schema = {
  /* -- header */
  "57742a73-ba65-5797-aebf-c1a270c4d02b",     /* uuid (corresponds to uri) */
  DLITE_ENTITY_SCHEMA,                        /* uri */
  1,                                          /* refcount, never free */
  (DLiteMeta *)&basic_metadata_schema,        /* meta */
  NULL,                                       /* iri */

  2,                                          /* ndimensions */
  6,                                          /* nproperties */
  0,                                          /* nrelations */

  entity_schema_dimensions,                   /* dimensions */
  entity_schema_properties,                   /* properties */
  NULL,                                       /* relations */

  0,                                          /* headersize */
  NULL,                                       /* init */
  NULL,                                       /* deinit */

  0,                                          /* npropdims */
  NULL,                                       /* propdiminds */

  0,                                          /* dimoffset */
  NULL,                                       /* propoffsets */
  0,                                          /* reloffset */
  0,                                          /* propdimsoffset */
  0,                                          /* propdimindsoffset */
  /* -- length of each dimention */
  2,                                          /* _ndimensions */
  6,                                          /* _nproperties */
  0,                                          /* _nrelations */
  /* -- value of each property */
  "EntitySchema",                             /* schema_name */
  "0.3",                                      /* schema_version */
  "http://meta.sintef.no",                    /* schema_namespace */
  "Meta-metadata description an entity.",     /* schema_description */
  entity_schema_dimensions,                   /* schema_dimensions */
  entity_schema_properties,                   /* schema_properties */
  NULL,                                       /* schema_relations */
  /* -- value of each relation */
  /* -- array property dimension values */
  {0, 0},                                     /* _propdims */
  /* -- array of first property dimension */
  {0, 0, 0, 0, 0, 0},                         /* _propdiminds */
  /* -- array of memory offsets to each instance property */
  {0, 0, 0, 0, 0, 0}                          /* _propoffsets */
};



/**************************************************************
 * collection_schema
 **************************************************************/

static DLiteDimension collection_schema_dimensions[] = {
  //{"ndimensions", "Number of common dimmensions."},
  //{"ninstances",  "Number of instances added to the collection."},
  //{"ndim_maps",   "Number of dimension maps."},
  {"nrelations",  "Number of relations."},
};
static char *collection_schema_prop_relations_dims[] = {"nrelations"};
static DLiteProperty collection_schema_properties[] = {
  {
  "relations",                               /* name */
  dliteRelation,                             /* type */
  sizeof(DLiteRelation),                     /* size */
  1,                                         /* ndims */
  collection_schema_prop_relations_dims,     /* dims */
  NULL,                                      /* unit */
  NULL,                                      /* iri */
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
  /* -- array property dimension values */
  size_t _propdims[1];
  /* -- array of first property dimension  */
  size_t _propdiminds[1];
  /* -- array of memory offsets to each instance property */
  size_t _propoffsets[1];
} collection_schema = {
  /* -- header */
  "a2e66e0e-d733-5067-b987-6b5e5d54fb12",        /* uuid (corresponds to uri) */
  DLITE_COLLECTION_SCHEMA,                       /* uri */
  1,                                             /* refcount, never free */
  (DLiteMeta *)&basic_metadata_schema,           /* meta */
  NULL,                                          /* iri */

  1,                                             /* ndimensions */
  1,                                             /* nproperties */
  0,                                             /* nrelations */

  collection_schema_dimensions,                  /* dimensions */
  collection_schema_properties,                  /* properties */
  NULL,                                          /* relations */

  offsetof(DLiteCollection, nrelations),         /* headersize */
  dlite_collection_init,                         /* init */
  dlite_collection_deinit,                       /* deinit */

  0,                                             /* npropdims */
  NULL,                                          /* propdiminds */

  0,                                             /* dimoffset */
  NULL,                                          /* propoffsets */
  0,                                             /* reloffset */
  0,                                             /* propdimsoffset */
  0,                                             /* propdimindsoffset */
  /* -- length of each dimention */
  1,                                             /* _ndimensions */
  1,                                             /* _nproperties */
  0,                                             /* _nrelations */
  /* -- value of each property */
  "CollectionSchema",                            /* schema_name */
  "0.1",                                         /* schema_version */
  "http://meta.sintef.no",                       /* schema_namespace */
  "Meta-metadata description a collection.",     /* schema_description */
  collection_schema_dimensions,                  /* schema_dimensions */
  collection_schema_properties,                  /* schema_properties */
  NULL,                                          /* schema_relations */
  /* -- value of each relation */
  /* -- array property dimension values */
  {0},                                           /* _propdims */
  /* -- array of first property dimension */
  {0},                                           /* _propdiminds */
  /* -- array of memory offsets to each instance property */
  {0}                                            /* _propoffsets */
};



/**************************************************************
 * Exposed pointers to schemas
 **************************************************************/

/* Forward declaration */
int dlite_meta_init(DLiteMeta *meta);

const DLiteMeta *dlite_get_basic_metadata_schema()
{
  if (!basic_metadata_schema.headersize)
    dlite_meta_init((DLiteMeta *)&basic_metadata_schema);
  return (DLiteMeta *)&basic_metadata_schema;
}

const DLiteMeta *dlite_get_entity_schema()
{
  if (!entity_schema.headersize)
    dlite_meta_init((DLiteMeta *)&entity_schema);
  return (DLiteMeta *)&entity_schema;
}

const DLiteMeta *dlite_get_collection_schema()
{
  if (!collection_schema.headersize)
    dlite_meta_init((DLiteMeta *)&collection_schema);
  return (DLiteMeta *)&collection_schema;
}
