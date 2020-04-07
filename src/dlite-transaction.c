#include <assert.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "utils/err.h"
#include "dlite-macros.h"
#include "dlite-entity.h"
#include "dlite-schemas.h"


/**************************************************************
 * Transaction
 **************************************************************/

/* Initialise additional data in a transaction */
int dlite_transaction_init(DLiteInstance *inst)
{
  //DLiteTransaction *t = (DLiteTransaction *)inst;

  if (!dlite_instance_is_data(inst))
    return err(1, "expected data instance");
  if (inst->meta->meta != dlite_get_transaction_schema())
    return err(1, "expected transaction data instance");

  return 0;
}


/* De-initialise additional data in a transaction */
int dlite_transaction_deinit(DLiteInstance *inst)
{
  UNUSED(inst);
  return 0;
}


/*
  Returns the number of instances that are stored in the transaction or
  -1 on error.
*/
int dlite_transaction_count(DLiteInstance *inst)
{
  UNUSED(inst);
  return 0;
}
