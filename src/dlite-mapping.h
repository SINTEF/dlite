#ifndef _DLITE_MAPPING_H
#define _DLITE_MAPPING_H

#include "dlite-mapping-plugins.h"


/**
  @file
  @brief Map instances of one metadata to another.

  Mappings are used to map one or more instances of certain metadata
  to an instance of another metadata.

  Mappings can be registered as plugins and invoked transparently
  by DLite.  For this to work, DLite search all paths listed in the
  DLITE_MAPPING_PLUGINS environment variable for plugins.
 */

/**
  Struct describing mapping.

  For each input, either the corresponding element in `inputs_maps`
  (if the element is the result of a sub-mapping) or `input_uris` (if
  the input is a provided input instance) is not NULL.
*/
typedef struct _DLiteMapping {
  const char *name;        /*!< Name of mapping. NULL corresponds to the
                                trivial case where one of the input URIs is
                                `output_uri`. */
  const char *output_uri;  /*!< Output metadata URI */
  int ninput;              /*!< Number of inputs */
  const struct _DLiteMapping **input_maps;
                           /*!< Array of input sub-trees. Length: ninput */
  const char **input_uris; /*!< Array of input metadata URIs. Length: ninput */
  DLiteMappingPlugin *api; /*!< Mapping that performs this mapping. */
  int cost;                /*!< The total cost of this mapping.  */
} DLiteMapping;



/**
  Returns a new nested mapping structure describing how `n` input
  instances of metadata `input_uris` can be mapped to `output_uri`.
 */
DLiteMapping *mapping_create(const char *ouput_uri, const char **input_uris,
                             int n);

/**
  Free's a mapping created with mapping_create().
*/
void mapping_free(DLiteMapping *m);



#endif /* _DLITE_MAPPING_H */
