#ifndef _DLITE_SCHEMAS_H
#define _DLITE_SCHEMAS_H

/**
  @file
  @brief Hardcoded metadata.
*/

#include "dlite-entity.h"


/* URIs for hardcoded metadata */
#define DLITE_BASIC_METADATA_SCHEMA \
  "http://meta.sintef.no/0.1/BasicMetadataSchema"

#define DLITE_ENTITY_SCHEMA \
  "http://meta.sintef.no/0.3/EntitySchema"

#define DLITE_COLLECTION_SCHEMA \
  "http://meta.sintef.no/0.6/CollectionSchema"



///** Pointer to hardcoded basic metadata schema. */
//extern const DLiteMeta * const dlite_BasicMetadataSchema;
//
///** Pointer to hardcoded entity schema. */
//extern const DLiteMeta * const dlite_EntitySchema;
//
///** Pointer to hardcoded collection schema. */
//extern const DLiteMeta * const dlite_CollectionSchema;

const DLiteMeta *dlite_get_basic_metadata_schema();
const DLiteMeta *dlite_get_entity_schema();
const DLiteMeta *dlite_get_collection_schema();



#endif /* _DLITE_SCHEMAS_H */
