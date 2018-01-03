#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <hdf5.h>

#include "config.h"

#include "boolean.h"
#include "err.h"

#include "dlite.h"
#include "dlite-api.h"


/* Macro for getting rid of unused parameter warnings... */
#define UNUSED(x) (void)(x)


/* Data struct for the hdf5 backend. */
typedef struct {
  DLite_HEAD
  hid_t root;       /* h5 file identifier to root */
  hid_t instance;   /* h5 group identifier to instance */
  hid_t properties; /* h5 group identifier to properties */
} DH5;


/* Writes error message for `status` to a static buffer and returns a
   pointer to it. */
static char *h5err(const DH5 *d, hid_t stat)
{
  static _thread_local char buff[256];
  /* H5E_type_t etype; */
  int n=0;
  if (d->uuid[0] && stat != d->root && stat != d->instance)
    n += snprintf(buff, sizeof(buff), "In '%s/%s'", d->uri, d->uuid);
  else
    n += snprintf(buff, sizeof(buff), "In '%s'", d->uri);
  /* H5Eget_msg(stat, &etype, buff + n, sizeof(buff) - n); */
  return buff;
}

/* Convinient error macros */
#define FAIL0(msg) \
  do {err(-1, msg); goto fail;} while (0)

#define FAIL1(msg, a1) \
  do {err(-1, msg, a1); goto fail;} while (0)

#define FAIL2(msg, a1, a2) \
  do {err(-1, msg, a1, a2); goto fail;} while (0)

#define FAIL3(msg, a1, a2, a3) \
  do {err(-1, msg, a1, a2, a3); goto fail;} while (0)


/* Error macros for when DH5 instance d in available */
#define DFAIL0(d, msg) \
  do {err(-1, "%s/%s: " msg, d->uri, d->uuid); goto fail;} while (0)

#define DFAIL1(d, msg, a1) \
  do {err(-1, "%s/%s: " msg, d->uri, d->uuid, a1); goto fail;} while (0)

#define DFAIL2(d, msg, a1, a2) \
  do {err(-1, "%s/%s: " msg, d->uri, d->uuid, a1, a2); goto fail;} while (0)

#define DFAIL3(d, msg, a1, a2, a3) \
  do {err(-1, "%s/%s: " msg, d->uri, d->uuid, a1, a2, a3); goto fail;} while (0)

#define DFAIL4(d, msg, a1, a2, a3, a4)                         \
  do {err(-1, "%s/%s: " msg, d->uri, d->uuid, a1, a2, a3, a4); \
    goto fail;} while (0)


/* Returns the HDF5 type identifier corresponding to `type` and `size`.
   Returns -1 on error.

   On success, the returned identifier should be closed with H5Tclose(). */
static hid_t get_memtype(DLiteType type, size_t size)
{
  hid_t stat, memtype;
  errno = 0;
  switch (type) {

  case DTBlob:
    memtype = H5Tcopy(H5T_NATIVE_OPAQUE);
    if ((stat = H5Tset_size(memtype, size)) < 0)
      return err(-1, "cannot set opaque memtype size to %lu", size);
    return memtype;
  case DTInt:
    switch (size) {
    case 1:     return H5Tcopy(H5T_NATIVE_INT8);
    case 2:     return H5Tcopy(H5T_NATIVE_INT16);
    case 4:     return H5Tcopy(H5T_NATIVE_INT32);
    case 8:     return H5Tcopy(H5T_NATIVE_INT64);
    default:    return errx(-1, "invalid int size: %lu", size);
    }
  case DTBool:
  case DTUInt:
    switch (size) {
    case 1:     return H5Tcopy(H5T_NATIVE_UINT8);
    case 2:     return H5Tcopy(H5T_NATIVE_UINT16);
    case 4:     return H5Tcopy(H5T_NATIVE_UINT32);
    case 8:     return H5Tcopy(H5T_NATIVE_UINT64);
    default:    return errx(-1, "invalid uint size: %lu", size);
    }
  case DTFloat:
    switch (size) {
    case sizeof(float):  return H5Tcopy(H5T_NATIVE_FLOAT);
    case sizeof(double): return H5Tcopy(H5T_NATIVE_DOUBLE);
    default:             return errx(-1, "no native float with size %lu", size);
    }
  case DTString:
    memtype = H5Tcopy(H5T_C_S1);
    if ((stat = H5Tset_size(memtype, size)) < 0)
      return err(-1, "cannot set string memtype size to %lu", size);
    return memtype;

  case DTStringPtr:
    if (size != sizeof(char *))
      return errx(-1, "size for DStringPtr must equal pointer size");
    memtype = H5Tcopy(H5T_C_S1);
    if ((stat = H5Tset_size(memtype, H5T_VARIABLE)) < 0)
      return err(-1, "cannot set DStringPtr memtype size");
    return memtype;

  default:      return errx(-1, "Invalid type number: %d", type);
  }
  abort();  /* sould never be reached */
}


/* Returns the HDF5 data space identifier corresponding to `ndims` and `dims`.
   If `dims` is NULL, length of all dimensions are assumed to be one.
   Returns -1 on error.

   On success, the returned identifier should be closed with H5Sclose(). */
static hid_t get_space(int ndims, const int *dims)
{
  hid_t space=-1;
  hsize_t *hdims=NULL;
  int i;
  if (!(hdims = calloc(ndims, sizeof(hsize_t))))
    return err(-1, "allocation failure");
  for (i=0; i<ndims; i++) hdims[i] = (dims) ? dims[i] : 1;
  if ((space = H5Screate_simple(ndims, hdims, NULL)) < 0)
    space = errx(-1, "cannot create hdf5 data space");
 /* fail: */
  if (hdims) free(hdims);
  return space;
}


/* Returns DLiteType corresponding to hdf5 dtype.

   Note: bool is returned ad int (since we store it as int). */
static DLiteType get_type(hid_t dtype)
{
  /* hid_t booltype=0; */
  htri_t isvariable;
  H5T_sign_t sign;
  H5T_class_t dclass;

  if ((dclass = H5Tget_class(dtype)) < 0)
    return err(-1, "cannot get hdf5 class");
  switch (dclass) {
  case H5T_OPAQUE:
    return DTBlob;
  case H5T_INTEGER:
    if ((sign = H5Tget_sign(dtype)) < 0)
      return err(-1, "cannot determine hdf5 signedness");
    return (sign == H5T_SGN_NONE) ? DTUInt : DTInt;
  case H5T_FLOAT:
    return DTFloat;
  case H5T_STRING:
    if ((isvariable = H5Tis_variable_str(dtype)) < 0)
      return err(-1,"cannot dtermine wheter hdf5 string is of variable length");
    return (isvariable) ? DTStringPtr : DTString;
  default:
    return err(-1, "hdf5 data class is not opaque, integer, float or string");
  }
}


/* Copied hdf5  dataset `name` in `group` to memory pointed to by `ptr`.

   Multi-dimensional arrays are supported.  `size` is the size of each
   data element, `ndims` is the number of dimensions and `dims` is an
   array of dimension sizes.

   Returns non-zero on error.
 */
static int get_data(const DH5 *d, hid_t group, const char *name, void *ptr,
                    DLiteType type, size_t size, int ndims, const int *dims)
{
  hid_t memtype=0, space=0, dspace=0, dset=0, dtype=0;
  htri_t isvariable;
  herr_t stat;
  hsize_t *ddims=NULL;
  DLiteType savedtype;
  size_t dsize;
  int i, dndims, nmemb=1, retval=-1;
  void *buff=ptr;

  errno=0;
  if ((memtype = get_memtype(type, size)) < 0) goto fail;
  if ((space = get_space(ndims, dims)) < 0) goto fail;

  /* Get: dset, dtype, dsize, dspace, dndims, ddims, */
  if ((dset = H5Dopen2(group, name, H5P_DEFAULT)) < 0)
    DFAIL1(d, "cannot open dataset '%s'", name);
  if ((dtype = H5Dget_type(dset)) < 0)
    DFAIL1(d, "cannot get hdf5 type of '%s'", name);
  if ((dsize = H5Tget_size(dtype)) == 0)
    DFAIL1(d, "cannot get size of '%s'", name);
  if ((dspace = H5Dget_space(dset)) < 0)
    DFAIL1(d, "cannot get data space of '%s'", name);
  if ((dndims = H5Sget_simple_extent_ndims(dspace)) < 0)
    DFAIL1(d, "cannot get number of dimimensions of '%s'", name);
  if (!(ddims = calloc(sizeof(hsize_t), dndims)))
    FAIL0("allocation failure");
  if ((stat = H5Sget_simple_extent_dims(dspace, ddims, NULL)) < 0)
    DFAIL1(d, "cannot get dims of '%s'", name);

  /* Check that dimensions matches */
  if (dndims == 0 && ndims == 1)
    dndims++;
  else if (dndims != ndims) {
    DFAIL3(d, "trying to read '%s' with ndims=%d, but ndims=%d",
           name, ndims, dndims);
    for (i=0; i<ndims; i++)
      if (ddims[i] != (hsize_t)((dims) ? dims[i] : 1))
        DFAIL4(d, "dimension %d of '%s': expected %d, got %d",
               i, name, (dims) ? dims[i] : 1, (int)ddims[i]);
  }
  for (i=0; i<ndims; i++) nmemb *= (dims) ? dims[i] : 1;

  /* Get type of data saved in the hdf5 file */
  if ((savedtype = get_type(dtype)) < 0)
    DFAIL1(d, "cannot get type of '%s'", name);

  /* Add space for NUL-termination for non-variable strings */
  if (savedtype == DTString) {
    if((isvariable = H5Tis_variable_str(dtype)) < 0)
      DFAIL1(d, "cannot determine if '%s' is stored as variable length string",
             name);
    if (!isvariable) dsize++;
  }

  /* Allocate temporary buffer for data type convertion */
  if (type == DTStringPtr && savedtype == DTString) {
    if (!(buff = calloc(nmemb, dsize))) FAIL0("allocation failure");
    if (memtype > 0) H5Tclose(memtype);
    if ((memtype = get_memtype(DTString, dsize)) < 0) goto fail;
  } else if (type == DTString && savedtype == DTStringPtr) {
    if (!(buff = calloc(nmemb, sizeof(void *)))) FAIL0("allocation failure");
    if (memtype > 0) H5Tclose(memtype);
    if ((memtype = get_memtype(DTStringPtr, sizeof(void *))) < 0) goto fail;
  } else if (type == DTBool && savedtype == DTUInt) {
    ;  /* pass, bool is saved as uint */
  } else if (savedtype != type) {
    DFAIL3(d, "trying to read '%s' as %s, but it is %s",
           name, dget_typename(type), dget_typename(savedtype));
  }

  if ((stat = H5Dread(dset, memtype, H5S_ALL, H5S_ALL, H5P_DEFAULT, buff)) < 0)
    DFAIL1(d, "cannot read dataset '%s'", name);

  /* Convert data type */
  if (type == DTStringPtr && savedtype == DTString) {
    for (i=0; i<nmemb; i++)
      if (!(((char **)ptr)[i] = strdup((char *)buff + i*dsize)))
        FAIL0("allocation failure");
  } else if (type == DTString && savedtype == DTStringPtr) {
    for (i=0; i<nmemb; i++) {
      strncpy((char *)ptr + i*size, ((char **)buff)[i], size);
      free(((char **)buff)[i]);
    }
  }

  retval = 0;
 fail:
  if (dset > 0) H5Dclose(dset);
  if (dtype > 0) H5Tclose(dtype);
  if (space > 0) H5Sclose(space);
  if (dspace > 0) H5Sclose(dspace);
  if (memtype > 0) H5Tclose(memtype);
  if (buff != ptr) free(buff);
  if (ddims != ptr) free(ddims);
  return retval;
}


/* Copies memory pointed to by `ptr` to hdf5 dataset `name` in `group`.

   Multi-dimensional arrays are supported.  `size` is the size of each
   data element, `ndims` is the number of dimensions and `dims` is an
   array of dimension sizes.

   Returns non-zero on error.
 */
static int set_data(DH5 *d, hid_t group, const char *name, const void *ptr,
                    DLiteType type, size_t size, int ndims, const int *dims)
{
  hid_t memtype=0, space=0, dset=0;
  herr_t stat;
  htri_t exists;
  int retval=-1;

  errno=0;
  if ((exists = H5Lexists(group, name, H5P_DEFAULT)) < 0)
    DFAIL1(d, "cannot determine if dataset '%s' already exists", name);

  /* Delete dataset if it already exists */
  if (exists && H5Ldelete(group, name, H5P_DEFAULT) < 0)
    DFAIL1(d, "cannot delete dataset '%s' for overwrite", name);

  if ((memtype = get_memtype(type, size)) < 0) goto fail;
  if ((space = get_space(ndims, dims)) < 0) goto fail;

  if ((dset = H5Dcreate(group, name, memtype, space,
                        H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0)
    DFAIL1(d, "cannot create dataset '%s'", name);

  if ((stat = H5Dwrite(dset, memtype, H5S_ALL, H5S_ALL, H5P_DEFAULT, ptr)) < 0)
    DFAIL1(d, "cannot write dataset '%s'", name);

  retval = 0;
 fail:
  if (dset > 0) H5Dclose(dset);
  if (space > 0) H5Sclose(space);
  if (memtype > 0) H5Tclose(memtype);
  return retval;
}


#if 0
/* Deletes dataset `name` from group `group`. Returns -1 on error. */
static int delete_data(DH5 *d, hid_t group, const char *name)
{
  UNUSED(d);
  if (H5Ldelete(group, name, H5P_DEFAULT) < 0)
    return errx(-1, "cannot delete dataset '%s'", name);
  return 0;
}
#endif


/* Entry in the linked list of entries created by find_entries() */
typedef struct _EntryList {
  char *name;
  struct _EntryList *next;
} EntryList;

/* Callback function for H5Literate(). For each visited entry, add its
   name to the front of the EntryList pointed to by `op_data`. */
static herr_t find_entries(hid_t loc_id, const char *name,
                           const H5L_info_t *info, void *op_data)
{
  herr_t stat;
  H5O_info_t infobuf;
  EntryList *e, **ep = op_data;
  UNUSED(info);

  if ((stat = H5Oget_info_by_name(loc_id, name, &infobuf, H5P_DEFAULT)) < 0)
    err(-1, "cannot get info about '%s'", name);
  else {
    int n = strlen(name) + 1;
    if (!(e = malloc(sizeof(EntryList)))) return err(-1, NULL);
    if (!(e->name = malloc(n))) return err(-1, NULL);
    memcpy(e->name, name, n);
    e->next = *ep;
    *ep = e;
  }

  return 0;
}

/* Free's all items in an EntryList. */
static void entrylist_free(EntryList *e)
{
  EntryList *next;
  while (e) {
    next = e->next;
    free(e->name);
    free(e);
    e = next;
  }
}



/********************************************************************
 * Required api
 ********************************************************************/

/**
  Returns an url to the metadata.

  Valid \a options are:

    rw   Read and write: open existing file or create new file (default)
    r    Read-only: open existing file for read-only
    a    Append: open existing file for read and write
    w    Write: truncate existing file or create new file

 */
DLite *dh5_open(const char *uri, const char *options, const char *uuid)
{
  DH5 *d = NULL;
  hid_t meta=0, dimensions=0;
  htri_t exists;
  DLite *retval=NULL;

  H5open();  /* Opens hdf5 library */

  if (!(d = calloc(1, sizeof(DH5)))) FAIL0("allocation failure");
  dlite_init((DLite *)d);
  if (!options || !options[0] || strcmp(options, "rw") == 0)  /* default */
    d->root = H5Fopen(uri, H5F_ACC_RDWR | H5F_ACC_CREAT, H5P_DEFAULT);
  else if (strcmp(options, "r") == 0)
    d->root = H5Fopen(uri, H5F_ACC_RDONLY, H5P_DEFAULT);
  else if (strcmp(options, "a") == 0)
    d->root = H5Fopen(uri, H5F_ACC_RDWR, H5P_DEFAULT);
  else if (strcmp(options, "w") == 0)
    d->root = H5Fcreate(uri, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
  else
    FAIL1("invalid options '%s', must be 'rw' (read and write), "
          "'r' (read-only), 'w' (write) or 'a' (append", options);
  if (d->root < 0)
    FAIL2("cannot open: '%s' with mode '%s'", uri, options);

  if ((exists = H5Lexists(d->root, uuid, H5P_DEFAULT)) < 0)
    FAIL2("cannot determine if '%s' exists in %s", uuid, uri);

  if (exists) {
    /* Instance `uuid` already exists */
    if ((d->instance = H5Gopen(d->root, uuid, H5P_DEFAULT)) < 0)
      FAIL2("cannot open instance /%s in %s", uuid, uri);

    if ((d->properties = H5Gopen(d->instance, "properties", H5P_DEFAULT)) < 0)
      FAIL2("cannot open /%s/properties in %s", uuid, uri);

  } else {
    /* Instance `uuid` does not exists: create new group structure */
    if ((d->instance = H5Gcreate(d->root, uuid, H5P_DEFAULT, H5P_DEFAULT,
                                 H5P_DEFAULT)) < 0)
      FAIL1("cannot create instance group in %s", uri);

    if ((d->properties = H5Gcreate(d->instance, "properties", H5P_DEFAULT,
                                   H5P_DEFAULT, H5P_DEFAULT)) < 0)
      FAIL2("cannot create /%s/properties group in %s", uuid, uri);

    if ((meta = H5Gcreate(d->instance, "meta", H5P_DEFAULT, H5P_DEFAULT,
                          H5P_DEFAULT)) < 0)
      FAIL2("cannot create /%s/meta group in %s", uuid, uri);

    if ((dimensions = H5Gcreate(d->instance, "dimensions", H5P_DEFAULT,
                                H5P_DEFAULT, H5P_DEFAULT)) < 0)
      FAIL2("cannot create /%s/dimensions group in %s", uuid, uri);
  }

  retval = (DLite *)d;
 fail:
  if (meta > 0) H5Gclose(meta);
  if (dimensions > 0) H5Gclose(dimensions);
  if (!retval && d) free(d);
  return retval;
}


/**
  Closes data handle dh5. Returns non-zero on error.
 */
int dh5_close(DLite *dh5)
{
  DH5 *d = (DH5 *)dh5;
  hid_t stat;
  int nerr=0;
  if ((stat = H5Gclose(d->properties)) < 0)
    nerr += err(1, "cannot close properties: %s", h5err(d, stat));
  if ((stat = H5Gclose(d->instance)) < 0)
    nerr += err(1, "cannot close instance: %s", h5err(d, stat));
  if ((stat = H5Fclose(d->root)) < 0)
    nerr += err(1, "cannot close root: %s", h5err(d, stat));
  return nerr;
}


/**
  Returns pointer to (malloc'ed) metadata or NULL on error.
 */
const char *dh5_get_metadata(const DLite *dh5)
{
  const DH5 *d = (DH5 *)dh5;
  hid_t meta=0;
  int size = sizeof(char *);
  char *name=NULL, *version=NULL, *namespace, *metadata=NULL;

  err_clear();

  if ((meta = H5Gopen(d->instance, "meta", H5P_DEFAULT)) < 0)
    DFAIL0(d, "missing 'meta' group");

  if (get_data(d, meta, "name", &name, DTStringPtr, size, 1, NULL) < 0)
    goto fail;
  if (get_data(d, meta, "version", &version, DTStringPtr, size, 1, NULL) < 0)
    goto fail;
  if (get_data(d, meta, "namespace", &namespace, DTStringPtr, size, 1,NULL) < 0)
    goto fail;

  /* combine to metadata */
  if (err_getcode() == 0) {
    size = strlen(name) + strlen(version) + strlen(namespace) + 3;
    metadata = malloc(size);
    snprintf(metadata, size, "%s/%s/%s", namespace, version, name);
  }

 fail:
  if (name) free(name);
  if (version) free(version);
  if (namespace) free(namespace);
  if (meta > 0)  H5Gclose(meta);
  return metadata;
}


/**
  Returns the size of dimension `name` or -1 on error.
 */
int dh5_get_dimension_size(const DLite *dh5, const char *name)
{
  DH5 *d = (DH5 *)dh5;
  hid_t dimensions=0;
  int dimsize=-1;

  if ((dimensions = H5Gopen(d->instance, "dimensions", H5P_DEFAULT)) < 0)
    DFAIL0(d, "no 'dimensions' group");

  if (get_data(d, dimensions, name, &dimsize, DTInt,
               sizeof(dimsize), 1, NULL) < 0)
    DFAIL1(d, "cannot get size of dimension '%s'", name);
 fail:
  if (dimensions > 0) H5Gclose(dimensions);
  return dimsize;
}


/**
  Copies property `name` to memory pointed to by `ptr`.
  Returns non-zero on error.
 */
int dh5_get_property(const DLite *dh5, const char *name, void *ptr,
                     DLiteType type, size_t size, int ndims, const int *dims)
{
  DH5 *d = (DH5 *)dh5;
  return get_data(d, d->properties, name, ptr, type, size, ndims, dims);
}


/********************************************************************
 * Optional api
 ********************************************************************/

/**
  Sets metadata.  Returns non-zero on error.
*/
int dh5_set_metadata(DLite *dh5, const char *metadata)
{
  DH5 *d = (DH5 *)dh5;
  hid_t meta=0;
  int len=strlen(metadata), dims[1]={1}, retval=1;
  char *buff=strdup(metadata), *p=buff+len;

  if ((meta = H5Gopen(d->instance, "meta", H5P_DEFAULT)) < 0)
    DFAIL0(d, "cannot open 'meta' group");

  while (p > buff && *p != '/') p--;
  set_data(d, meta, "version", p+1, DTString, strlen(p+1), 1, dims);
  *p = '\0';

  while (p > buff && *p != '/') p--;
  set_data(d, meta, "name", p+1, DTString, strlen(p+1), 1, dims);
  *p = '\0';

  set_data(d, meta, "namespace", buff, DTString, strlen(buff), 1, dims);

  retval = 0;
 fail:
  if (buff) free(buff);
  if (meta > 0) H5Gclose(meta);
  return retval;
}


/**
  Sets size of dimension `name`.  Returns non-zero on error.
*/
int dh5_set_dimension_size(DLite *dh5, const char *name, int size)
{
  DH5 *d = (DH5 *)dh5;
  hid_t dimensions=0;
  int dims[1]={1}, retval=1;
  int64_t dsize=size;

  if ((dimensions = H5Gopen(d->instance, "dimensions", H5P_DEFAULT)) < 0)
    DFAIL0(d, "cannot open 'meta' group");

  set_data(d, dimensions, name, &dsize, DTInt, sizeof(int64_t), 1, dims);

  retval = 0;
 fail:
  if (dimensions > 0) H5Gclose(dimensions);
  return retval;
}


/**
  Sets property `name` to the memory (of `size` bytes) pointed to by `ptr`.
  Returns non-zero on error.
*/
int dh5_set_property(DLite *dh5, const char *name, const void *ptr,
                     DLiteType type, size_t size, int ndims, const int *dims)
{
  DH5 *d = (DH5 *)dh5;
  return set_data(d, d->properties, name, ptr, type, size, ndims, dims);
}


/**
  Returns a NULL-terminated array of string pointers to instance UUID's.
  The caller is responsible to free the returned array.
  Options is ignored.
*/
char **dh5_get_instance_names(const char *uri, const char *options)
{
  hid_t root;
  herr_t stat;
  EntryList *entries=NULL, *e;
  int i, n;
  char **names=NULL;

  UNUSED(options);
  H5open();  /* Opens hdf5 library */

  if ((root = H5Fopen(uri, H5F_ACC_RDONLY, H5P_DEFAULT)) < 0)
    FAIL1("cannot open '%s'", uri);

  if ((stat = H5Literate(root, H5_INDEX_NAME, H5_ITER_NATIVE, NULL,
                         find_entries, &entries)) < 0)
    FAIL0("error finding instances");

  for (n=0, e=entries; e; e=e->next) n++;
  if (!(names = malloc((n + 1)*sizeof(char *)))) FAIL0("allocation failure");
  for (i=0, e=entries; e; i++, e=e->next) {
    size_t len = strlen(e->name) + 1;
    if (!(names[i] = malloc(len))) FAIL0("allocation failure");
    memcpy(names[i], e->name, len);
  }
  names[n] = NULL;

 fail:
  if (entries) entrylist_free(entries);
  if (root >= 0) H5Fclose(root);
  return names;
}


/**
  Returns a positive value if dimension `name` is defined, zero if it
  isn't and a negative value on error.
 */
int dh5_has_dimension(DLite *dh5, const char *name)
{
  DH5 *d = (DH5 *)dh5;
  hid_t dimensions=0;
  htri_t exists;
  int retval=-1;
  if ((dimensions = H5Gopen(d->instance, "dimensions", H5P_DEFAULT)) < 0)
    DFAIL2(d, "no '/%s/dimensions' group in %s", d->uuid, d->uri);
  if ((exists = H5Lexists(dimensions, name, H5P_DEFAULT)) < 0)
    FAIL3("cannot determine if '%s' has dimension '%s' in %s",
	  d->uuid, name, d->uri);
  retval = exists;
 fail:
  if (dimensions > 0) H5Gclose(dimensions);
  return retval;
}

/**
  Returns a positive value if property `name` is defined, zero if it
  isn't and a negative value on error.
 */
int dh5_has_property(DLite *dh5, const char *name)
{
  DH5 *d = (DH5 *)dh5;
  htri_t exists;
  if ((exists = H5Lexists(d->properties, name, H5P_DEFAULT)) < 0)
    FAIL3("cannot determine if '%s' has property '%s' in %s",
	  d->uuid, name, d->uri);
 fail:
  return exists;
}


/**
   If the uuid was generated from a unique name, return a pointer to a
   newly malloc'ed string with this name.  Otherwise NULL is returned.
*/
char *dh5_get_dataname(DLite *dh5)
{
  DH5 *d = (DH5 *)dh5;
  char *s=NULL;
  if (get_data(d, d->instance, "dataname", &s,
               DTStringPtr, sizeof(char *), 1, NULL)) return NULL;
  return s;
}

/**
  Gives the instance a name.  This function should only be called
  if the uuid was generated from `name`.
  Returns non-zero on error.
*/
int dh5_set_dataname(DLite *dh5, const char *name)
{
  DH5 *d = (DH5 *)dh5;
  return set_data(d, d->instance, "dataname", name,
                  DTString, strlen(name), 1, NULL);
}



API h5_api = {
  "hdf5",

  dh5_open,
  dh5_close,

  dh5_get_metadata,
  dh5_get_dimension_size,
  dh5_get_property,

  /* optional */
  dh5_set_metadata,
  dh5_set_dimension_size,
  dh5_set_property,

  dh5_get_instance_names,

  dh5_has_dimension,
  dh5_has_property,

  dh5_get_dataname,
  dh5_set_dataname,
};
