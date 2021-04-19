#ifndef _DLITE_TRANSACTION_H
#define _DLITE_TRANSACTION_H

#include "triplestore.h"
#include "dlite-entity.h"
#include "dlite-type.h"


/**
  @file
  @brief A DLite Transaction.

  Transactions are a special type of fully persistent [1] instances that
  implement copy-on-write semantics [2].

  [1]: https://en.wikipedia.org/wiki/Persistent_data_structure
  [2]: https://en.wikipedia.org/wiki/Copy-on-write
*/


/**
  Initiates a transaction instance.

  Returns non-zero on error.
 */
int dlite_transaction_init(DLiteInstance *inst);


/**
  Deinitiates a transaction instance.

  Returns non-zero on error.
 */
int dlite_transaction_deinit(DLiteInstance *inst);


/**
  Returns the number of instances that are stored in the transaction or
  -1 on error.
*/
int dlite_transaction_count(DLiteInstance *inst);


#endif /* _DLITE_TRANSACTION_H */
