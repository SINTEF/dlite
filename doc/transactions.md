Transactions
============
Transactions is a feature that comes from SOFT, which allows to easy manage arbitrary long series of immutable instances while ensuring full provenance.  Conceptually it share many similarities with git.

The figure below shows two examples of transactions. Subfigure (a) shows a linear transaction with instance `A` being the root.  `A` is the parent of `B` and `B` is the parent of `C`.  Subfigure (b) shows a forked transaction with two leaf instances, `C` and `E`.

![Two examples of transactions.](figs/transactions.png)

**Figure**: Schematic figure of (a) a transaction consisting of a linear chain of instances and (b) a transaction with a fork.  The circles stands for instances, while arrows relate them to their parent instance.

All transactions starts with a root instance with no parent instance.  All other instances in a transaction has exactly one parent instance.
All instances in a transaction that serves as a parent are immutable (that is, all instances except the leafs).  Non-root transaction instances store a [SHA-3](https://en.wikipedia.org/wiki/SHA-3) hash of their parents together with the parent UUIDs.  This make it possible to ensure that any of the ancestors of a transaction has not been changed - providing full provenance.


### Implementation in DLite
DLite allow all instances to be used in transactions.  Any instance can be added as a leaf in a transaction using the function `dlite_instance_set_parent()`.  The only requirements are that the instance does not already has a parent and that the parent is immutable.  If the instance is also immutable, it can be used as a parent in a transaction.  The function `dlite_instance_freeze()` can be used to make an instance immutable.

Since it is possible to use a collection to hold the state of a program, transactions can be used to track the complete evolution of the state of a program.


#### Validate a transaction
Use the function `dlite_instance_verify()` to validate a transaction.  If the `recursive` argument is non-zero, all parents of `inst` are also verified.

#### Memory management
This example creates the transaction shown in subfigure (b) above.
```C
DLiteInstance *A = dlite_instance_get("A");
DLiteInstance *B = dlite_instance_get("B");
DLiteInstance *C = dlite_instance_get("C");
DLiteInstance *D = dlite_instance_get("D");
DLiteInstance *E = dlite_instance_get("E");

dlite_instance_freeze(A);
dlite_instance_freeze(B);
dlite_instance_freeze(C);

dlite_instance_set_parent(B, A);
dlite_instance_set_parent(C, B);
dlite_instance_set_parent(D, B);
dlite_instance_set_parent(E, D);

/* Remove references to all instances, except the leafs.  They can easily be
   recovered with dlite_instance_get_parent(). */
dlite_instance_decref(A);
dlite_instance_decref(B);
dlite_instance_decref(D);
```

This will leave the pointers `C` and `E` as valid references in the current scope.  However, because DLite is reference counted and `C` and `E` keeps references to their parents, `A`, `B` and `D` will still be kept in memory. If we are now dereferring `E`
```C
dlite_instance_decref(E);
```
both the memory for both `E` and `D` will be free'ed, while `A` and `B` will stay in memory, since `B` is referred to by `C` and `A` is referred to by `B`.  They will first be free'ed when we derefer `C`.
```C
dlite_instance_decref(C);
```

#### Serialisation of transactions
The biggest issue with the current implementation is that serialisation of transactions requires special attention by storage plugins and is not supported out of the box.
