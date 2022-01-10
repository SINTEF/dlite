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
   "Schema dimensions."                       /* description */
  },
  {
   "properties",                              /* name */
   dliteProperty,                             /* type */
   sizeof(DLiteProperty),                     /* size */
   1,                                         /* ndims */
   basic_metadata_schema_prop_properties_dims,/* dims */
   NULL,                                      /* unit */
   NULL,                                      /* iri */
   "Schema properties."                       /* description */
  },
  {
   "relations",                               /* name */
   dliteRelation,                             /* type */
   sizeof(DLiteRelation),                     /* size */
   1,                                         /* ndims */
   basic_metadata_schema_prop_relations_dims, /* dims */
   NULL,                                      /* unit */
   NULL,                                      /* iri */
   "Schema relations."                        /* description */
  }
};
static struct _BasicMetadataSchema {
  /* -- header */
  DLiteMeta_HEAD
  /* -- length of each dimension */
  size_t ndimensions;
  size_t nproperties;
  size_t nrepations;
  /* -- value of each property */
  char *name;
  char *version;
  char *namespace;
  char *description;
  DLiteDimension *dimensions;
  DLiteProperty  *properties;
  DLiteRelation  *relations;
  /* -- value of each relation */
  /* -- array property dimension values */
  size_t __propdims[3];
  /* -- array of first property dimension  */
  size_t __propdiminds[7];
  /* -- array of memory offsets to each instance property */
  size_t __propoffsets[7];
} basic_metadata_schema = {
  /* -- header */
  "a8194052-7d3b-530f-ba1e-7e82fd51bf31",        /* uuid (corresponds to uri) */
  DLITE_BASIC_METADATA_SCHEMA,                   /* uri */
  1,                                             /* _refcount, never free */
  (DLiteMeta *)&basic_metadata_schema,           /* meta */
  NULL,                                          /* iri */

  3,                                             /* _ndimensions */
  7,                                             /* _nproperties */
  0,                                             /* _nrelations */

  basic_metadata_schema_dimensions,              /* _dimensions */
  basic_metadata_schema_properties,              /* _properties */
  NULL,                                          /* _relations */

  0,                                             /* _headersize */
  NULL,                                          /* _init */
  NULL,                                          /* _deinit */
  NULL,                                          /* _getdim */
  NULL,                                          /* _setdim */
  NULL,                                          /* _loadprop */
  NULL,                                          /* _saveprop */

  3,                                             /* _npropdims */
  (size_t *)basic_metadata_schema.__propdiminds, /* _propdiminds */

  offsetof(struct _BasicMetadataSchema, ndimensions),  /* _dimoffset */
  (size_t *)basic_metadata_schema.__propoffsets,       /* _propoffsets */
  offsetof(struct _BasicMetadataSchema, relations),    /* _reloffset */
  offsetof(struct _BasicMetadataSchema, __propdims),   /* _propdimsoffset */
  offsetof(struct _BasicMetadataSchema, __propdiminds),/* _propdimindsoffset */
  /* -- length of each dimention */
  3,                                             /* ndimensions */
  7,                                             /* nproperties */
  0,                                             /* nrelations */
  /* -- value of each property */
  "BasicMetadataSchema",                         /* name */
  "0.1",                                         /* version */
  "http://onto-ns.com/meta",                       /* namespace */
  "Meta-metadata description an entity.",        /* description */
  basic_metadata_schema_dimensions,              /* dimensions */
  basic_metadata_schema_properties,              /* properties */
  NULL,                                          /* relations */
  /* -- value of each relation */
  /* -- array property dimension values */
  {3, 7, 0},                                     /* __propdims */
  /* -- array of first property dimension  */
  {0, 0, 0, 0, 0, 1, 2},                         /* __propdiminds */
  /* -- array of memory offsets to each instance property */
  {                                              /* __propoffsets */
    offsetof(struct _BasicMetadataSchema, name),
    offsetof(struct _BasicMetadataSchema, version),
    offsetof(struct _BasicMetadataSchema, namespace),
    offsetof(struct _BasicMetadataSchema, description),
    offsetof(struct _BasicMetadataSchema, dimensions),
    offsetof(struct _BasicMetadataSchema, properties),
    offsetof(struct _BasicMetadataSchema, relations)
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
  size_t ndimensions;
  size_t nproperties;
  size_t nrelations;
  /* -- value of each property */
  char *name;
  char *version;
  char *namespace;
  char *description;
  DLiteDimension *dimensions;
  DLiteProperty  *properties;
  DLiteRelation  *relation;
  /* -- value of each relation */
  /* -- array property dimension values */
  size_t __propdims[3];
  /* -- array of first property dimension  */
  size_t __propdiminds[6];
  /* -- array of memory offsets to each instance property */
  size_t __propoffsets[6];
} entity_schema = {
  /* -- header */
  "46168985-705c-5029-b856-3ee1cccccefc",     /* uuid (corresponds to uri) */
  DLITE_ENTITY_SCHEMA,                        /* uri */
  1,                                          /* _refcount, never free */
  (DLiteMeta *)&basic_metadata_schema,        /* meta */
  NULL,                                       /* iri */

  2,                                          /* _ndimensions */
  6,                                          /* _nproperties */
  0,                                          /* _nrelations */

  entity_schema_dimensions,                   /* _dimensions */
  entity_schema_properties,                   /* _properties */
  NULL,                                       /* _relations */

  0,                                          /* _headersize */
  NULL,                                       /* _init */
  NULL,                                       /* _deinit */
  NULL,                                       /* _getdim */
  NULL,                                       /* _setdim */
  NULL,                                       /* _loadprop */
  NULL,                                       /* _saveprop */

  0,                                          /* _npropdims */
  NULL,                                       /* _propdiminds */

  0,                                          /* _dimoffset */
  NULL,                                       /* _propoffsets */
  0,                                          /* _reloffset */
  0,                                          /* _propdimsoffset */
  0,                                          /* _propdimindsoffset */
  /* -- length of each dimention */
  2,                                          /* ndimensions */
  6,                                          /* nproperties */
  0,                                          /* nrelations */
  /* -- value of each property */
  "EntitySchema",                             /* name */
  "0.3",                                      /* version */
  "http://onto-ns.com/meta",                  /* namespace */
  "Meta-metadata description an entity.",     /* description */
  entity_schema_dimensions,                   /* dimensions */
  entity_schema_properties,                   /* properties */
  NULL,                                       /* relations */
  /* -- value of each relation */
  /* -- array property dimension values */
  {0, 0, 0},                                  /* __propdims */
  /* -- array of first property dimension */
  {0, 0, 0, 0, 0, 0},                         /* __propdiminds */
  /* -- array of memory offsets to each instance property */
  {0, 0, 0, 0, 0, 0}                          /* __propoffsets */
};



/**************************************************************
 * collection_entity
 **************************************************************/

static DLiteDimension collection_entity_dimensions[] = {
  //{"ndimensions", "Number of common dimmensions."},
  //{"ninstances",  "Number of instances added to the collection."},
  //{"ndim_maps",   "Number of dimension maps."},
  {"nrelations",  "Number of relations."},
};
static char *collection_entity_prop_relations_dims[] = {"nrelations"};
static DLiteProperty collection_entity_properties[] = {
  {
  "relations",                               /* name */
  dliteRelation,                             /* type */
  sizeof(DLiteRelation),                     /* size */
  1,                                         /* ndims */
  collection_entity_prop_relations_dims,     /* dims */
  NULL,                                      /* unit */
  NULL,                                      /* iri */
  "Array of relations (s-p-o triples)."      /* description */
  }
};
static struct _CollectionEntity {
  /* -- header */
  DLiteMeta_HEAD
  /* -- length of each dimension */
  size_t ndimensions;
  size_t nproperties;
  /* -- value of each property */
  char *name;
  char *version;
  char *namespace;
  char *description;
  DLiteDimension *dimensions;
  DLiteProperty  *properties;
  /* -- value of each relation */
  /* -- array property dimension values */
  size_t __propdims[2];
  /* -- array of first property dimension  */
  size_t __propdiminds[1];
  /* -- array of memory offsets to each instance property */
  size_t __propoffsets[1];
} collection_entity = {
  /* -- header */
  "96f31fc3-3838-5cb8-8d90-eddee6ff59ca",        /* uuid (corresponds to uri) */
  DLITE_COLLECTION_ENTITY,                       /* uri */
  1,                                             /* _refcount, never free */
  (DLiteMeta *)&entity_schema,                   /* meta */
  NULL,                                          /* iri */

  1,                                             /* _ndimensions */
  1,                                             /* _nproperties */
  0,                                             /* _nrelations */

  collection_entity_dimensions,                  /* _dimensions */
  collection_entity_properties,                  /* _properties */
  NULL,                                          /* _relations */

  offsetof(DLiteCollection, nrelations),         /* _headersize */
  dlite_collection_init,                         /* _init */
  dlite_collection_deinit,                       /* _deinit */
  dlite_collection_getdim,                       /* _getdim */
  NULL,                                          /* _setdim */
  dlite_collection_loadprop,                     /* _loadprop */
  dlite_collection_saveprop,                     /* _saveprop */

  0,                                             /* _npropdims */
  NULL,                                          /* _propdiminds */

  0,                                             /* _dimoffset */
  NULL,                                          /* _propoffsets */
  0,                                             /* _reloffset */
  0,                                             /* _propdimsoffset */
  0,                                             /* _propdimindsoffset */
  /* -- length of each dimention */
  1,                                             /* ndimensions */
  1,                                             /* nproperties */
  /* -- value of each property */
  "Collection",                                  /* name */
  "0.1",                                         /* version */
  "http://onto-ns.com/meta",                       /* namespace */
  "Meta-metadata description a collection.",     /* description */
  collection_entity_dimensions,                  /* dimensions */
  collection_entity_properties,                  /* properties */
  /* -- value of each relation */
  /* -- array property dimension values */
  {0, 0},                                        /* __propdims */
  /* -- array of first property dimension */
  {0},                                           /* __propdiminds */
  /* -- array of memory offsets to each instance property */
  {0}                                            /* __propoffsets */
};



/**************************************************************
 * Exposed pointers to schemas
 **************************************************************/

/* Forward declaration */
int dlite_meta_init(DLiteMeta *meta);

const DLiteMeta *dlite_get_basic_metadata_schema()
{
  dlite_get_uuid(basic_metadata_schema.uuid, DLITE_BASIC_METADATA_SCHEMA);
  if (!basic_metadata_schema._headersize)
    dlite_meta_init((DLiteMeta *)&basic_metadata_schema);
  return (DLiteMeta *)&basic_metadata_schema;
}

const DLiteMeta *dlite_get_entity_schema()
{
  dlite_get_uuid(entity_schema.uuid, DLITE_ENTITY_SCHEMA);
  if (!entity_schema._headersize)
    dlite_meta_init((DLiteMeta *)&entity_schema);
  return (DLiteMeta *)&entity_schema;
}

const DLiteMeta *dlite_get_collection_entity()
{
  dlite_get_uuid(collection_entity.uuid, DLITE_COLLECTION_ENTITY);
  if (!collection_entity._headersize)
    dlite_meta_init((DLiteMeta *)&collection_entity);
  return (DLiteMeta *)&collection_entity;
}
