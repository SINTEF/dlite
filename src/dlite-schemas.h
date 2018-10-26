#ifndef _DLITE_SCHEMAS_H
#define _DLITE_SCHEMAS_H

/**
  @file
  @brief Hardcoded metadata.
*/

#include "dlite-entity.h"


/* URIs for hardcoded metadata */
#define DLITE_BASIC_METADATA_SCHEMA \
  "http://meta.sintef.no/0.1/basic_metadata_schema"

#define DLITE_ENTITY_SCHEMA \
  "http://meta.sintef.no/0.3/entity_schema"

#define DLITE_COLLECTION_SCHEMA \
  "http://meta.sintef.no/0.6/collection_schema"



/** Pointer to hardcoded basic metadata schema. */
extern const DLiteMeta * const dlite_BasicMetadataSchema;

/** Pointer to hardcoded entity schema. */
extern const DLiteMeta * const dlite_EntitySchema;

/** Pointer to hardcoded collection schema. */
extern const DLiteMeta * const dlite_CollectionSchema;



#endif /* _DLITE_SCHEMAS_H */
