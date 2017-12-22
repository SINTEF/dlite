#ifndef _DLITE_H
#define _DLITE_H

/**
  @file
*/

/** Opaque struct/handle for data items. */
typedef struct _DLite DLite;

/** Basic data types */
typedef enum _DLiteType {
  DTBlob,           /*!< Binary blob */
  DTBool,           /*!< Boolean */
  DTInt,            /*!< Signed integer */
  DTUInt,           /*!< Unigned integer */
  DTFloat,          /*!< Floating point */
  DTString,         /*!< Fix-sized string */
  DTStringPtr       /*!< Pointer to NUL-terminated string */
} DLiteType;



/**
  @name Utility functions
  @{
 */

/**
    Returns descriptive name for \a type or NULL on error.
*/
char *dget_typename(DLiteType type);

/** @} */


/**
  @name Minimum API

  Minimum API that all backends should implement.
  @{
 */

/**
  Opens data item \a id from \a uri using \a driver.
  Returns a opaque pointer to a data handle or NULL on error.

  The \a options are passed to the driver.  Options for known
  drivers are:
    * hdf5
        - rw   Read and write: open existing file or create new file (default)
        - r    Read-only: open existing file for read-only
        - w    Write: truncate existing file or create new file
        - a    Append: open existing file for read and write
*/
DLite *dopen(const char *driver, const char *uri, const char *options,
              const char *id);

/**
  Closes data handle \a d. Returns non-zero on error.
*/
int dclose(DLite *d);

/**
  Returns url to metadata or NULL on error. Do not free.
 */
const char *dget_metadata(const DLite *d);

/**
  Sets metadata.  Returns non-zero on error.
 */
int dset_metadata(DLite *d, const char *metadata);

/**
  Returns the size of dimension \a name or -1 on error.
 */
int dget_dimension_size(const DLite *d, const char *name);

/**
  Sets size of dimension \a name.  Returns non-zero on error.
*/
int dset_dimension_size(DLite *d, const char *name, int size);

/**
  Copies property \a name to memory pointed to by \a ptr.
  Multi-dimensional arrays are supported.

  \param  d      DLite data handle.
  \param  name   Name of the property.
  \param  ptr    Pointer to memory to write to.
  \param  type   Type of data elements.
  \param  size   Size of each data element.
  \param  ndims  Number of dimensions.
  \param  dims   Array of dimension sizes of length \p ndims.

  Returns non-zero on error.
 */
int dget_property(const DLite *d, const char *name, void *ptr,
                  DLiteType type, size_t size, int ndims, const int *dims);


/**
  Sets property \a name to the memory (of \a size bytes) pointed to by
  \a value.
  The argument \a string_pointers has the same meaning as for dget_property().

  Returns non-zero on error.
*/
int dset_property(DLite *d, const char *name, const void *ptr,
                  DLiteType type, size_t size, int ndims, const int *dims);


/** @} */


/**
  @name Optional API

  Optional API that backends are free leave unimplemented.
  All functions below are supported by the HDF5 backend.
  @{
*/

/**
  Returns a NULL-terminated array of string pointers to instance UUID's.
  The caller is responsible to free the returned array.
 */
char **dget_instance_names(const char *driver, const char *uri,
                           const char *options);

/**
  Frees NULL-terminated array of instance names returned by
  dget_instance_names().
*/
void dfree_instance_names(char **names);

/**
   If the uuid was generated from a unique name, return a pointer to a
   newly malloc'ed string with this name.  Otherwise NULL is returned.
*/
char *dget_dataname(DLite *d);


/* TODO: consider removing the functions below... */

/**
  Returns the number of dimensions or -1 on error.
 */
int dget_ndimensions(const DLite *d);

/**
  Returns the name of dimension `n` or NULL on error.  Do not free.
 */
const char*dget_dimension_name(const DLite *d, int n);

/**
  Returns the size of dimension `n` or -1 on error.
 */
int dget_dimension_size_by_index(const DLite *d, int n);

/**
  Returns the number of properties or -1 on error.
 */
int dget_nproperties(const DLite *d);

/**
  Returns a pointer to property name or NULL on error.  Do not free.
 */
const char *dget_property_name(const DLite *d, int n);

/**
  Like dh5_get_property_by_name(), except that the property is
  specified by index \a n instead of name.
 */
int dget_property_by_index(const DLite *d, int n, void *ptr,
                           DLiteType type, size_t size, int ndims,
                           const int *dims);

/** @} */

#endif /* _DLITE_H */
