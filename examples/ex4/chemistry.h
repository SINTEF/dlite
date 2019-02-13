/* This file is generated with dlite-codegen -- do not edit!
 *
 * Template: c-header.txt
 * Metadata: http://www.sintef.no/calm/0.1/Chemistry
 *
 * This file declares a struct for metadata Chemistry that can be included
 * and used in your project without any dependencies (except for the
 * header files boolean.h, integers.h and floats.h that are provided with
 * dlite).
 */

/**
  @file
  @brief Simple entity with alloy and particle compositions.
*/
#ifndef _CHEMISTRY_H
#define _CHEMISTRY_H

#include "boolean.h"
#include "integers.h"
#include "floats.h"

#define CHEMISTRY_NAME      "Chemistry"
#define CHEMISTRY_VERSION   "0.1"
#define CHEMISTRY_NAMESPACE "http://www.sintef.no/calm"
#define CHEMISTRY_URI       "http://www.sintef.no/calm/0.1/Chemistry"
#define CHEMISTRY_UUID      "62bfca3a-cd16-5046-b44b-a3d69b34fcff"
#define CHEMISTRY_META_URI  "http://meta.sintef.no/0.3/EntitySchema"
#define CHEMISTRY_META_UUID "57742a73-ba65-5797-aebf-c1a270c4d02b"


typedef struct _Chemistry {
  /* -- header */
  char _uuid[36+1];   /*!< UUID for this data instance. */
  const char *_uri;   /*!< Unique name or uri of the data instance.
                           Can be NULL. */

  size_t _refcount;   /*!< Number of references to this instance. */

  const void *_meta;  /*!< Pointer to the metadata describing this instance. */

  /* -- dimension values */
  size_t nelements;              /*!< Number of different chemical elements. */
  size_t nphases;                /*!< Number of phases. */

  /* -- property values */
  char *alloy;                   /*!< A human description of the alloying system and temper.
 */
  char **elements;               /*!< Chemical symbol of each chemical element.  By convension the dependent element (e.g. Al) is listed first.
 [nelements] */
  char **phases;                 /*!< Name of each phase.
 [nphases] */
  float64_t *X0;                 /*!< Nominal composition.  Should sum to one.
 [nelements] */
  float64_t *Xp;                 /*!< Average composition of each phase, excluding matrix.  Each row should sum to one.
 [nphases,nelements] */
  float64_t *volfrac;            /*!< Volume fraction of each phase, excluding matrix.
 [nphases] */
  float64_t *rpart;              /*!< Average particle radius of each phase, excluding matrix.
 [nphases] */
  float64_t *atvol;              /*!< Average volume per atom for each phase.
 [nphases] */


  size_t offsets[8];

} Chemistry;


#endif /* _CHEMISTRY_H */
