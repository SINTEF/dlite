#ifndef _DLITE_SCHEMAS_H
#define _DLITE_SCHEMAS_H

/**
  @file
  @brief Hardcoded metadata.
*/

#include "dlite-entity.h"


/** URIs for hardcoded metadata */
#define DLITE_BASIC_METADATA_SCHEMA \
  "http://meta.sintef.no/0.1/BasicMetadataSchema"

#define DLITE_ENTITY_SCHEMA \
  "http://meta.sintef.no/0.3/EntitySchema"

#define DLITE_COLLECTION_SCHEMA \
  "http://meta.sintef.no/0.6/CollectionSchema"


/** Functions returning a pointer to static definitions of basic schemas. */
const DLiteMeta *dlite_get_basic_metadata_schema();
const DLiteMeta *dlite_get_entity_schema();
const DLiteMeta *dlite_get_collection_schema();

#endif /* _DLITE_SCHEMAS_H */
