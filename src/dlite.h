#ifndef _DLITE_H
#define _DLITE_H

/**
  @file
  @brief Main header file for dlite which import main headers
*/

#ifdef HAVE_CONFIG
#include "config.h"
#endif


#ifndef HAVE_DLITE
#define HAVE_DLITE
#endif

#include "dlite-misc.h"
#include "dlite-type.h"
#include "dlite-schemas.h"
#include "dlite-entity.h"
#include "dlite-storage.h"
#include "dlite-collection.h"
#include "dlite-getlicense.h"
#include "dlite-json.h"


#endif /* _DLITE_H */
