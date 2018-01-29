/* dh5lite.c -- DLite plugin for hdf5 */
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
#include "dlite-datamodel.h"


/* Macro for getting rid of unused parameter warnings... */
#define UNUSED(x) (void)(x)


/* Storage for hdf5 backend. */
typedef struct {
  DLiteStorage_HEAD
  hid_t root;       /* h5 file identifier to root */
} DH5Storage;


/* Data model for hdf5 backend. */
typedef struct {
  DLiteDataModel_HEAD
  hid_t instance;     /* h5 group identifier to instance */
  hid_t meta;         /* h5 group identifier to metadata */
  hid_t dimensions;   /* h5 group identifier to dimensions */
  hid_t properties;   /* h5 group identifier to properties */
} DH5DataModel;



/* Convinient error macros */
#define FAIL0(msg) \
  do {err(-1, msg); goto fail;} while (0)

#define FAIL1(msg, a1) \
  do {err(-1, msg, a1); goto fail;} while (0)

#define FAIL2(msg, a1, a2) \
  do {err(-1, msg, a1, a2); goto fail;} while (0)

#define FAIL3(msg, a1, a2, a3) \
  do {err(-1, msg, a1, a2, a3); goto fail;} while (0)


/* Error macros for when DLiteDataModel instance d in available */
#define DFAIL0(d, msg) \
  do {err(-1, "%s/%s: " msg, d->s->uri, d->uuid); goto fail;} while (0)

#define DFAIL1(d, msg, a1) \
  do {err(-1, "%s/%s: " msg, d->s->uri, d->uuid, a1); goto fail;} while (0)

#define DFAIL2(d, msg, a1, a2) \
  do {err(-1, "%s/%s: " msg, d->s->uri, d->uuid, a1, a2); goto fail;} while (0)

#define DFAIL3(d, msg, a1, a2, a3) \
  do {err(-1, "%s/%s: " msg, d->s->uri, d->uuid, a1, a2, a3); goto fail;} while (0)

#define DFAIL4(d, msg, a1, a2, a3, a4)                            \
  do {err(-1, "%s/%s: " msg, d->s->uri, d->uuid, a1, a2, a3, a4); \
    goto fail;} while (0)


/* Returns the HDF5 type identifier corresponding to `type` and `size`.
   Returns -1 on error.

   On success, the returned identifier should be closed with H5Tclose(). */
static hid_t get_memtype(DLiteType type, size_t size)
{
  hid_t stat, memtype;
  errno = 0;
  switch (type) {

  case dliteBlob:
    memtype = H5Tcopy(H5T_NATIVE_OPAQUE);
    if ((stat = H5Tset_size(memtype, size)) < 0)
      return err(-1, "cannot set opaque memtype size to %lu", size);
    return memtype;
  case dliteInt:
    switch (size) {
    case 1:     return H5Tcopy(H5T_NATIVE_INT8);
    case 2:     return H5Tcopy(H5T_NATIVE_INT16);
    case 4:     return H5Tcopy(H5T_NATIVE_INT32);
    case 8:     return H5Tcopy(H5T_NATIVE_INT64);
    default:    return errx(-1, "invalid int size: %lu", size);
    }
  case dliteBool:
  case dliteUInt:
    switch (size) {
    case 1:     return H5Tcopy(H5T_NATIVE_UINT8);
    case 2:     return H5Tcopy(H5T_NATIVE_UINT16);
    case 4:     return H5Tcopy(H5T_NATIVE_UINT32);
    case 8:     return H5Tcopy(H5T_NATIVE_UINT64);
    default:    return errx(-1, "invalid uint size: %lu", size);
    }
  case dliteFloat:
    switch (size) {
    case sizeof(float):  return H5Tcopy(H5T_NATIVE_FLOAT);
    case sizeof(double): return H5Tcopy(H5T_NATIVE_DOUBLE);
    default:             return errx(-1, "no native float with size %lu", size);
    }
  case dliteFixString:
    memtype = H5Tcopy(H5T_C_S1);
    if ((stat = H5Tset_size(memtype, size)) < 0)
      return err(-1, "cannot set string memtype size to %lu", size);
    return memtype;

  case dliteStringPtr:
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
static hid_t get_space(size_t ndims, const size_t *dims)
{
  hid_t space=-1;
  hsize_t *hdims=NULL;
  size_t i;
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
    return dliteBlob;
  case H5T_INTEGER:
    if ((sign = H5Tget_sign(dtype)) < 0)
      return err(-1, "cannot determine hdf5 signedness");
    return (sign == H5T_SGN_NONE) ? dliteUInt : dliteInt;
  case H5T_FLOAT:
    return dliteFloat;
  case H5T_STRING:
    if ((isvariable = H5Tis_variable_str(dtype)) < 0)
      return err(-1,"cannot dtermine wheter hdf5 string is of variable length");
    return (isvariable) ? dliteStringPtr : dliteFixString;
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
static int get_data(const DLiteDataModel *d, hid_t group,
                    const char *name, void *ptr,
                    DLiteType type, size_t size,
                    size_t ndims, const size_t *dims)
{
  hid_t memtype=0, space=0, dspace=0, dset=0, dtype=0;
  htri_t isvariable;
  herr_t stat;
  hsize_t *ddims=NULL;
  DLiteType savedtype;
  size_t i, nmemb=1;
  int dsize, dndims, retval=-1;
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
  else if (dndims != (int)ndims) {
    DFAIL3(d, "trying to read '%s' with ndims=%lu, but ndims=%d",
           name, ndims, dndims);
    for (i=0; i<ndims; i++)
      if (ddims[i] != (hsize_t)((dims) ? dims[i] : 1))
        DFAIL4(d, "dimension %lu of '%s': expected %lu, got %d",
               i, name, (dims) ? dims[i] : 1, (int)ddims[i]);
  }
  for (i=0; i<ndims; i++) nmemb *= (dims) ? dims[i] : 1;

  /* Get type of data saved in the hdf5 file */
  if ((savedtype = get_type(dtype)) < 0)
    DFAIL1(d, "cannot get type of '%s'", name);

  /* Add space for NUL-termination for non-variable strings */
  if (savedtype == dliteFixString) {
    if((isvariable = H5Tis_variable_str(dtype)) < 0)
      DFAIL1(d, "cannot determine if '%s' is stored as variable length string",
             name);
    if (!isvariable) dsize++;
  }

  /* Allocate temporary buffer for data type convertion */
  if (type == dliteStringPtr && savedtype == dliteFixString) {
    if (!(buff = calloc(nmemb, dsize))) FAIL0("allocation failure");
    if (memtype > 0) H5Tclose(memtype);
    if ((memtype = get_memtype(dliteFixString, dsize)) < 0) goto fail;
  } else if (type == dliteFixString && savedtype == dliteStringPtr) {
    if (!(buff = calloc(nmemb, sizeof(void *)))) FAIL0("allocation failure");
    if (memtype > 0) H5Tclose(memtype);
    if ((memtype = get_memtype(dliteStringPtr, sizeof(void *))) < 0) goto fail;
  } else if (type == dliteBool && savedtype == dliteUInt) {
    ;  /* pass, bool is saved as uint */
  } else if (savedtype != type) {
    DFAIL3(d, "trying to read '%s' as %s, but it is %s",
           name, dlite_type_get_dtypename(type),
           dlite_type_get_dtypename(savedtype));
  }

  if ((stat = H5Dread(dset, memtype, H5S_ALL, H5S_ALL, H5P_DEFAULT, buff)) < 0)
    DFAIL1(d, "cannot read dataset '%s'", name);

  /* Convert data type */
  if (type == dliteStringPtr && savedtype == dliteFixString) {
    for (i=0; i<nmemb; i++)
      if (!(((char **)ptr)[i] = strdup((char *)buff + i*dsize)))
        FAIL0("allocation failure");
  } else if (type == dliteFixString && savedtype == dliteStringPtr) {
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
static int set_data(DLiteDataModel *d, hid_t group,
                    const char *name, const void *ptr,
                    DLiteType type, size_t size,
                    size_t ndims, const size_t *dims)
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
static int delete_data(DLiteDataModel *d, hid_t group, const char *name)
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
DLiteStorage *dh5_open(const char *uri, const char *options)
{
  DH5Storage *s;
  DLiteStorage *retval=NULL;

  H5open();  /* Opens hdf5 library */

  if (!(s = calloc(1, sizeof(DH5Storage)))) FAIL0("allocation failure");

  if (!options || !options[0] || strcmp(options, "rw") == 0) { /* default */
    s->root = H5Fopen(uri, H5F_ACC_RDWR | H5F_ACC_CREAT, H5P_DEFAULT);
    s->writable = 1;
  } else if (strcmp(options, "r") == 0) {
    s->root = H5Fopen(uri, H5F_ACC_RDONLY, H5P_DEFAULT);
    s->writable = 0;
  } else if (strcmp(options, "a") == 0) {
    s->root = H5Fopen(uri, H5F_ACC_RDWR, H5P_DEFAULT);
    s->writable = 1;
  } else if (strcmp(options, "w") == 0) {
    s->root = H5Fcreate(uri, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    s->writable = 1;
  } else {
    FAIL1("invalid options '%s', must be 'rw' (read and write), "
          "'r' (read-only), 'w' (write) or 'a' (append)", options);
  }

  if (s->root < 0)
    FAIL2("cannot open: '%s' with options '%s'", uri, options);

  retval = (DLiteStorage *)s;
 fail:
  if (!retval && s) free(s);
  return retval;
}


/**
  Closes data handle dh5. Returns non-zero on error.
 */
int dh5_close(DLiteStorage *s)
{
  DH5Storage *sh5 = (DH5Storage *)s;
  int nerr=0;
  if (H5Fclose(sh5->root) < 0)
    nerr += err(1, "cannot close %s", s->uri);
  return nerr;
}


DLiteDataModel *dh5_datamodel(const DLiteStorage *s, const char *uuid)
{
  DH5DataModel *d;
  DLiteDataModel *retval=NULL;
  DH5Storage *sh5 = (DH5Storage *)s;
  htri_t exists;

  if (!(d = calloc(1, sizeof(DH5DataModel)))) FAIL0("allocation failure");

  if ((exists = H5Lexists(sh5->root, uuid, H5P_DEFAULT)) < 0)
    FAIL2("cannot determine if '%s' exists in %s", uuid, sh5->uri);

  if (exists) {
    /* Instance `uuid` already exists: assigh groups */
    if ((d->instance = H5Gopen(sh5->root, uuid, H5P_DEFAULT)) < 0)
      FAIL2("cannot open instance '/%s' in '%s'", uuid, sh5->uri);

    if ((d->meta = H5Gopen(d->instance, "meta", H5P_DEFAULT)) < 0)
      FAIL2("cannot open '/%s/meta' in '%s'", uuid, sh5->uri);

    if ((d->dimensions = H5Gopen(d->instance, "dimensions", H5P_DEFAULT)) < 0)
      FAIL2("cannot open '/%s/dimensions' in '%s'", uuid, sh5->uri);

    if ((d->properties = H5Gopen(d->instance, "properties", H5P_DEFAULT)) < 0)
      FAIL2("cannot open '/%s/properties' in '%s'", uuid, sh5->uri);

  } else {
    /* Instance `uuid` does not exists: create new group structure */
    if ((d->instance = H5Gcreate(sh5->root, uuid, H5P_DEFAULT, H5P_DEFAULT,
                                 H5P_DEFAULT)) < 0)
      FAIL2("cannot open/create group '/%s' in '%s'", uuid, sh5->uri);

    if ((d->meta = H5Gcreate(d->instance, "meta", H5P_DEFAULT, H5P_DEFAULT,
                             H5P_DEFAULT)) < 0)
      FAIL2("cannot create group '/%s/meta' in '%s'", uuid, sh5->uri);

    if ((d->dimensions = H5Gcreate(d->instance, "dimensions", H5P_DEFAULT,
                                   H5P_DEFAULT, H5P_DEFAULT)) < 0)
      FAIL2("cannot create group '/%s/dimensions' in '%s'", uuid, sh5->uri);

    if ((d->properties = H5Gcreate(d->instance, "properties", H5P_DEFAULT,
                                   H5P_DEFAULT, H5P_DEFAULT)) < 0)
      FAIL2("cannot create group '/%s/properties' in '%s'", uuid, sh5->uri);
  }

  retval = (DLiteDataModel *)d;
 fail:
  if (!retval && d) free(d);
  return retval;
}


/**
  Closes data handle dh5. Returns non-zero on error.
 */
int dh5_datamodel_free(DLiteDataModel *d)
{
  DH5DataModel *dh5=(DH5DataModel *)d;
  int nerr=0;
  if (H5Gclose(dh5->properties) < 0)
    nerr += err(1, "cannot close /%s/properties in %s", d->uuid, d->s->uri);
  if (H5Gclose(dh5->dimensions) < 0)
    nerr += err(1, "cannot close /%s/dimensions in %s", d->uuid, d->s->uri);
  if (H5Gclose(dh5->meta) < 0)
    nerr += err(1, "cannot close /%s/meta in %s", d->uuid, d->s->uri);
  if (H5Gclose(dh5->instance) < 0)
    nerr += err(1, "cannot close /%s in %s", d->uuid, d->s->uri);
  return nerr;
}


/**
  Returns pointer to (malloc'ed) metadata or NULL on error.
 */
const char *dh5_get_metadata(const DLiteDataModel *d)
{
  const DH5DataModel *dh5 = (DH5DataModel *)d;
  char *name=NULL, *version=NULL, *namespace=NULL, *metadata=NULL;
  int size;

  err_clear();

  if (get_data(d, dh5->meta, "name", &name, dliteStringPtr,
               sizeof(char *), 1, NULL) < 0) goto fail;
  if (get_data(d, dh5->meta, "version", &version, dliteStringPtr,
               sizeof(char *), 1, NULL) < 0) goto fail;
  if (get_data(d, dh5->meta, "namespace", &namespace, dliteStringPtr,
               sizeof(char *), 1,NULL) < 0) goto fail;

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
  return metadata;
}


/**
  Returns the size of dimension `name` or 0 on error.
 */
size_t dh5_get_dimension_size(const DLiteDataModel *d, const char *name)
{
  DH5DataModel *dh5 = (DH5DataModel *)d;
  int dimsize;
  if (get_data(d, dh5->dimensions, name, &dimsize, dliteInt,
               sizeof(dimsize), 1, NULL) < 0)
    return err(0, "cannot get size of dimension '%s'", name);
  return dimsize;
}


/**
  Copies property `name` to memory pointed to by `ptr`.
  Returns non-zero on error.
 */
int dh5_get_property(const DLiteDataModel *d, const char *name, void *ptr,
                     DLiteType type, size_t size,
                     size_t ndims, const size_t *dims)
{
  DH5DataModel *dh5 = (DH5DataModel *)d;
  return get_data(d, dh5->properties, name, ptr, type, size, ndims, dims);
}


/********************************************************************
 * Optional api
 ********************************************************************/

/**
  Sets metadata.  Returns non-zero on error.
*/
int dh5_set_metadata(DLiteDataModel *d, const char *metadata)
{
  DH5DataModel *dh5 = (DH5DataModel *)d;
  size_t dims[1]={1};
  char *name, *version, *namespace;

  if (dlite_split_metadata(metadata, &name, &version, &namespace))
    return 1;

  set_data(d, dh5->meta, "name", name, dliteFixString,
           strlen(name), 1, dims);
  set_data(d, dh5->meta, "version", version, dliteFixString,
           strlen(version), 1, dims);
  set_data(d, dh5->meta, "namespace", namespace, dliteFixString,
           strlen(namespace), 1, dims);

  free(name);
  free(version);
  free(namespace);
  return 0;
}


/**
  Sets size of dimension `name`.  Returns non-zero on error.
*/
int dh5_set_dimension_size(DLiteDataModel *d, const char *name, size_t size)
{
  DH5DataModel *dh5 = (DH5DataModel *)d;
  size_t dims[1]={1};
  int64_t dsize=size;

  return set_data(d, dh5->dimensions, name, &dsize, dliteInt,
                  sizeof(int64_t), 1, dims);
}


/**
  Sets property `name` to the memory (of `size` bytes) pointed to by `ptr`.
  Returns non-zero on error.
*/
int dh5_set_property(DLiteDataModel *d, const char *name, const void *ptr,
                     DLiteType type, size_t size,
                     size_t ndims, const size_t *dims)
{
  DH5DataModel *dh5 = (DH5DataModel *)d;
  return set_data(d, dh5->properties, name, ptr, type, size, ndims, dims);
}


/**
  Returns a NULL-terminated array of string pointers to instance UUID's.
  The caller is responsible to free the returned array.
  Options is ignored.
*/
char **dh5_get_uuids(const DLiteStorage *s)
{
  DH5Storage *sh5 = (DH5Storage *)s;
  herr_t stat;
  EntryList *entries=NULL, *e;
  int i, n;
  char **names=NULL;

  if ((stat = H5Literate(sh5->root, H5_INDEX_NAME, H5_ITER_NATIVE, NULL,
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
  return names;
}


/**
  Returns a positive value if dimension `name` is defined, zero if it
  isn't and a negative value on error.
 */
int dh5_has_dimension(const DLiteDataModel *d, const char *name)
{
  DH5DataModel *dh5 = (DH5DataModel *)d;
  htri_t exists;
  if ((exists = H5Lexists(dh5->dimensions, name, H5P_DEFAULT)) < 0)
    return err(-1, "cannot determine if '%s' has dimension '%s' in %s",
               d->uuid, name, d->s->uri);
  return exists;
}

/**
  Returns a positive value if property `name` is defined, zero if it
  isn't and a negative value on error.
 */
int dh5_has_property(const DLiteDataModel *d, const char *name)
{
  DH5DataModel *dh5 = (DH5DataModel *)d;
  htri_t exists;
  if ((exists = H5Lexists(dh5->properties, name, H5P_DEFAULT)) < 0)
    return err(-1, "cannot determine if '%s' has property '%s' in %s",
               d->uuid, name, d->s->uri);
  return exists;
}


/**
   If the uuid was generated from a unique name, return a pointer to a
   newly malloc'ed string with this name.  Otherwise NULL is returned.
*/
char *dh5_get_dataname(const DLiteDataModel *d)
{
  DH5DataModel *dh5 = (DH5DataModel *)d;
  char *s=NULL;
  if (get_data(d, dh5->instance, "dataname", &s,
               dliteStringPtr, sizeof(char *), 1, NULL)) return NULL;
  return s;
}

/**
  Gives the instance a name.  This function should only be called
  if the uuid was generated from `name`.
  Returns non-zero on error.
*/
int dh5_set_dataname(DLiteDataModel *d, const char *name)
{
  DH5DataModel *dh5 = (DH5DataModel *)d;
  return set_data(d, dh5->instance, "dataname", name,
                  dliteFixString, strlen(name), 1, NULL);
}



DLitePlugin h5_plugin = {
  "hdf5",

  dh5_open,
  dh5_close,

  dh5_datamodel,
  dh5_datamodel_free,

  dh5_get_metadata,
  dh5_get_dimension_size,
  dh5_get_property,

  /* optional */
  dh5_get_uuids,

  dh5_set_metadata,
  dh5_set_dimension_size,
  dh5_set_property,

  dh5_has_dimension,
  dh5_has_property,

  dh5_get_dataname,
  dh5_set_dataname,
};
