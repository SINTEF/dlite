/* This file is generated with dlite-codegen -- do not edit!
 *
 * Template: c-header.txt
 * Metadata: http://meta.sintef.no/philib/0.1/PhilibTable
 *
 * This file declares a struct for metadata PhilibTable that can be included
 * and used in your project without any dependencies (except for the
 * header files boolean.h, integers.h and floats.h that are provided with
 * dlite).
 */

/**
  @file
  @brief Tabulation of thermodynamic phase diagramata for philib.
*/
#ifndef _PHILIBTABLE_H
#define _PHILIBTABLE_H

#include "boolean.h"
#include "integers.h"
#include "floats.h"

#define PHILIBTABLE_NAME      "PhilibTable"
#define PHILIBTABLE_VERSION   "0.1"
#define PHILIBTABLE_NAMESPACE "http://meta.sintef.no/philib"
#define PHILIBTABLE_URI       "http://meta.sintef.no/philib/0.1/PhilibTable"
#define PHILIBTABLE_UUID      "f6626c3e-e427-59b9-b5fd-0c296bd5787d"
#define PHILIBTABLE_META_URI  "http://meta.sintef.no/0.3/EntitySchema"
#define PHILIBTABLE_META_UUID "57742a73-ba65-5797-aebf-c1a270c4d02b"


typedef struct _PhilibTable {
  /* -- header */
  char _uuid[36+1];   /*!< UUID for this data instance. */
  const char *_uri;   /*!< Unique name or uri of the data instance.
                           Can be NULL. */

  size_t _refcount;   /*!< Number of references to this instance. */

  const void *_meta;  /*!< Pointer to the metadata describing this instance. */

  /* -- dimension values */
  size_t nelements;              /*!< Number of elements. */
  size_t nphases;                /*!< Number of phases. */
  size_t nvars;                  /*!< Number of free variables. */
  size_t nbounds;                /*!< Variable bounds, always 2 (lower, upper). */
  size_t nconds;                 /*!< Number of conditions. */
  size_t ncalc;                  /*!< Number of calculated variables. */
  size_t npoints;                /*!< Number of tabulated points.  For regular grids, it should equal the product of all values in `ntics`. */

  /* -- property values */
  char *database;                /*!< Name of thermodynamic database.
 */
  char **elements;               /*!< Chemical symbol of each element.
 [nelements] */
  char **phases;                 /*!< Name of each phase.
 [nphases] */
  int32_t *phaseselementdep;     /*!< Indicia of the dependent element of each phase.
 [nphases] */
  char **varnames;               /*!< Name of each free variable.
 [nvars] */
  float64_t *varranges;          /*!< Lower and upper bound for each free variable.
 [nvars,nbounds] */
  char **condnames;              /*!< Name/expression for each condition.
 [nconds] */
  float64_t *condvalues;         /*!< Value of each condition.
 [nconds] */
  char **calcnames;              /*!< Name of each calculated variable.
 [ncalc] */
  float64_t *calcvalues;         /*!< Value of calculated variables for each point.
 [npoints,ncalc] */
  bool regular_grid;             /*!< Whether the points refer to a regular grid.
 */
  int32_t *ticks;                /*!< For a regular grid, the number of tabulated points along each free variable.  Unused if iregular grid.
 [nvars] */
  float64_t *points;             /*!< The values of the free variables for which the calculated variables are calculated.
 [npoints,nvars] */


  size_t offsets[13];

} PhilibTable;


#endif /* _PHILIBTABLE_H */
