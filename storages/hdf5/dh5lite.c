/* dh5lite.c -- DLite plugin for hdf5 */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <hdf5.h>

#include "config.h"

#include "boolean.h"
#include "utils/err.h"

#include "dlite.h"
#include "dlite-datamodel.h"
#include "dlite-macros.h"


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
      return err(dliteMemoryError, "cannot set opaque memtype size to %lu", size);
    return memtype;
  case dliteInt:
    switch (size) {
    case 1:     return H5Tcopy(H5T_NATIVE_INT8);
    case 2:     return H5Tcopy(H5T_NATIVE_INT16);
    case 4:     return H5Tcopy(H5T_NATIVE_INT32);
    case 8:     return H5Tcopy(H5T_NATIVE_INT64);
    default:    return errx(dliteValueError, "invalid int size: %lu", size);
    }
  case dliteBool:
  case dliteUInt:
    switch (size) {
    case 1:     return H5Tcopy(H5T_NATIVE_UINT8);
    case 2:     return H5Tcopy(H5T_NATIVE_UINT16);
    case 4:     return H5Tcopy(H5T_NATIVE_UINT32);
    case 8:     return H5Tcopy(H5T_NATIVE_UINT64);
    default:    return errx(dliteValueError, "invalid uint size: %lu", size);
    }
  case dliteFloat:
    switch (size) {
    case sizeof(float):  return H5Tcopy(H5T_NATIVE_FLOAT);
    case sizeof(double): return H5Tcopy(H5T_NATIVE_DOUBLE);
    default:             return errx(dliteValueError, "no native float with size %lu", size);
    }
  case dliteFixString:
    memtype = H5Tcopy(H5T_C_S1);
    if ((stat = H5Tset_size(memtype, size)) < 0)
      return err(dliteValueError, "cannot set string memtype size to %lu", size);
    return memtype;

  case dliteStringPtr:
    if (size != sizeof(char *))
      return errx(dliteValueError, "size for DStringPtr must equal pointer size");
    memtype = H5Tcopy(H5T_C_S1);
    if ((stat = H5Tset_size(memtype, H5T_VARIABLE)) < 0)
      return err(dliteAttributeError, "cannot set DStringPtr memtype size");
    return memtype;

  default:      return errx(dliteValueError, "Invalid type number: %d", type);
  }
  abort();  /* sould never be reached */
}


/* Returns the HDF5 data space identifier corresponding to `ndims` and `shape`.
   If ` shape` is NULL, length of all dimensions are assumed to be one.
   Returns -1 on error.

   On success, the returned identifier should be closed with H5Sclose(). */
static hid_t get_space(size_t ndims, const size_t *shape)
{
  hid_t space=-1;
  hsize_t *hdims=NULL;
  size_t i;
  if (!(hdims = calloc(ndims, sizeof(hsize_t))))
    return err(dliteMemoryError, "allocation failure");
  for (i=0; i<ndims; i++) hdims[i] = (shape) ? shape[i] : 1;
  if ((space = H5Screate_simple(ndims, hdims, NULL)) < 0)
    space = errx(dliteIOError, "cannot create hdf5 data space");
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
    return err(dliteValueError, "cannot get hdf5 class");
  switch (dclass) {
  case H5T_OPAQUE:
    return dliteBlob;
  case H5T_INTEGER:
    if ((sign = H5Tget_sign(dtype)) < 0)
      return err(dliteValueError, "cannot determine hdf5 signedness");
    return (sign == H5T_SGN_NONE) ? dliteUInt : dliteInt;
  case H5T_FLOAT:
    return dliteFloat;
  case H5T_STRING:
    if ((isvariable = H5Tis_variable_str(dtype)) < 0)
      return err(dliteValueError,"cannot dtermine wheter hdf5 string is of variable length");
    return (isvariable) ? dliteStringPtr : dliteFixString;
  default:
    return err(dliteValueError, "hdf5 data class is not opaque, integer, float or string");
  }
}


/* Copied hdf5  dataset `name` in `group` to memory pointed to by `ptr`.

   Multi-dimensional arrays are supported.  `size` is the size of each
   data element, `ndims` is the number of dimensions and ` shape` is an
   array of dimension sizes.

   Returns non-zero on error.
 */
static int get_data(const DLiteDataModel *d, hid_t group,
                    const char *name, void *ptr,
                    DLiteType type, size_t size,
                    size_t ndims, const size_t *shape)
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
  if ((space = get_space(ndims, shape)) < 0) goto fail;

  /* Get: dset, dtype, dsize, dspace, dndims, ddims, */
  if ((dset = H5Dopen2(group, name, H5P_DEFAULT)) < 0)
    DFAIL1(dliteStorageOpenError, d, "cannot open dataset '%s'", name);
  if ((dtype = H5Dget_type(dset)) < 0)
    DFAIL1(dliteStorageLoadError, d, "cannot get hdf5 type of '%s'", name);
  if ((dsize = H5Tget_size(dtype)) == 0)
    DFAIL1(dliteStorageLoadError, d, "cannot get size of '%s'", name);
  if ((dspace = H5Dget_space(dset)) < 0)
    DFAIL1(dliteStorageLoadError, d, "cannot get data space of '%s'", name);
  if ((dndims = H5Sget_simple_extent_ndims(dspace)) < 0)
    DFAIL1(dliteStorageLoadError, d, "cannot get number of dimimensions of '%s'", name);
  if (!(ddims = calloc(sizeof(hsize_t), dndims)))
    FAILCODE(dliteMemoryError, "allocation failure");
  if ((stat = H5Sget_simple_extent_dims(dspace, ddims, NULL)) < 0)
    DFAIL1(dliteStorageLoadError, d, "cannot get shape of '%s'", name);

  /* Check that dimensions matches */
  if (dndims == 0 && ndims == 1)
    dndims++;
  else if (dndims != (int)ndims) {
    DFAIL3(dliteIndexError, d, "trying to read '%s' with ndims=%lu, but ndims=%d",
           name, ndims, dndims);
    for (i=0; i<ndims; i++)
      if (ddims[i] != (hsize_t)((shape) ? shape[i] : 1))
        DFAIL4(dliteIndexError, d, "dimension %lu of '%s': expected %lu, got %d",
               i, name, (shape) ? shape[i] : 1, (int)ddims[i]);
  }
  for (i=0; i<ndims; i++) nmemb *= (shape) ? shape[i] : 1;

  /* Get type of data saved in the hdf5 file */
  if ((savedtype = get_type(dtype)) < 0)
    DFAIL1(dliteValueError, d, "cannot get type of '%s'", name);

  /* Add space for NUL-termination for non-variable strings */
  if (savedtype == dliteFixString) {
    if((isvariable = H5Tis_variable_str(dtype)) < 0)
      DFAIL1(dliteValueError, d, "cannot determine if '%s' is stored as variable length string",
             name);
    if (!isvariable) dsize++;
  }

  /* Allocate temporary buffer for data type convertion */
  if (type == dliteStringPtr && savedtype == dliteFixString) {
    if (!(buff = calloc(nmemb, dsize)))
     FAILCODE(dliteMemoryError, "allocation failure");
    if (memtype > 0) H5Tclose(memtype);
    if ((memtype = get_memtype(dliteFixString, dsize)) < 0) goto fail;
  } else if (type == dliteFixString && savedtype == dliteStringPtr) {
    if (!(buff = calloc(nmemb, sizeof(void *))))
     FAILCODE(dliteMemoryError, "allocation failure");
    if (memtype > 0) H5Tclose(memtype);
    if ((memtype = get_memtype(dliteStringPtr, sizeof(void *))) < 0) goto fail;
  } else if (type == dliteBool && savedtype == dliteUInt) {
    ;  /* pass, bool is saved as uint */
  } else if (savedtype != type) {
    DFAIL3(dliteValueError, d, "trying to read '%s' as %s, but it is %s",
           name, dlite_type_get_dtypename(type),
           dlite_type_get_dtypename(savedtype));
  }

  if ((stat = H5Dread(dset, memtype, H5S_ALL, H5S_ALL, H5P_DEFAULT, buff)) < 0)
    DFAIL1(dliteStorageLoadError, d, "cannot read dataset '%s'", name);

#if _WIN32
  /* On Windows it seems that free() cannot be used to release memory
     allocated by H5Dread() for variable length strings...

     As a work around we allocate new strings with malloc() and reclaim
     the memory allocated by H5Dread().
  */
  if (savedtype == dliteStringPtr) {
    char **tmp;
    if (!(tmp = malloc(nmemb*sizeof(char *)))) FAILCODE(dliteMemoryError, "allocation failure");
    for (i=0; i<nmemb; i++)
      if (!(tmp[i] = strdup(((char **)buff)[i])))
        FAILCODE(dliteMemoryError, "allocation failure");
    if (H5Dvlen_reclaim(memtype, space, H5P_DEFAULT, buff) < 0)
      DFAIL1(dliteMemoryError, d, "cannot reclaim memory for dataset '%s'", name);
    for (i=0; i<nmemb; i++)
      ((char **)buff)[i] = tmp[i];
    free(tmp);
  }
#endif

  /* Convert data type */
  if (type == dliteStringPtr && savedtype == dliteFixString) {
    for (i=0; i<nmemb; i++)
      if (!(((char **)ptr)[i] = strdup((char *)buff + i*dsize)))
        FAILCODE(dliteMemoryError, "allocation failure");
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
   data element, `ndims` is the number of dimensions and ` shape` is an
   array of dimension sizes.

   Returns non-zero on error.
 */
static int set_data(DLiteDataModel *d, hid_t group,
                    const char *name, const void *ptr,
                    DLiteType type, size_t size,
                    size_t ndims, const size_t *shape)
{
  hid_t memtype=0, space=0, dset=0;
  herr_t stat;
  htri_t exists;
  int retval=-1;

  errno=0;
  if ((exists = H5Lexists(group, name, H5P_DEFAULT)) < 0)
    DFAIL1(dliteAttributeError, d, "cannot determine if dataset '%s' already exists", name);

  /* Delete dataset if it already exists */
  if (exists && H5Ldelete(group, name, H5P_DEFAULT) < 0)
    DFAIL1(dliteIOError, d, "cannot delete dataset '%s' for overwrite", name);

  if ((memtype = get_memtype(type, size)) < 0) goto fail;
  if ((space = get_space(ndims, shape)) < 0) goto fail;

  if ((dset = H5Dcreate(group, name, memtype, space,
                        H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0)
    DFAIL1(dliteIOError, d, "cannot create dataset '%s'", name);

  if ((stat = H5Dwrite(dset, memtype, H5S_ALL, H5S_ALL, H5P_DEFAULT, ptr)) < 0)
    DFAIL1(dliteIOError, d, "cannot write dataset '%s'", name);

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
    return errx(dliteIOError, "cannot delete dataset '%s'", name);
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
    err(dliteAttributeError, "cannot get info about '%s'", name);
  else {
    int n = strlen(name) + 1;
    if (!(e = malloc(sizeof(EntryList)))) return err(dliteMemoryError, NULL);
    if (!(e->name = malloc(n))) {
      free(e);
      return err(dliteMemoryError, NULL);
    }
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
  Returns a pointer to storage located at uri.

  Valid \a options are:

  - mode : append | r | w
      Valid values are:
      - append   Append to existing file or create new file (default)
      - r        Open existing file for read-only
      - rw       Open existing file for read and write
      - w        Truncate existing file or create new file



    rw   Read and write: open existing file or create new file (default)
    r    Read-only: open existing file for read-only
    a    Append: open existing file for read and write
    w    Write: truncate existing file or create new file

 */
DLiteStorage *
dh5_open(const DLiteStoragePlugin *api, const char *uri, const char *options)
{
  DH5Storage *s=NULL;
  DLiteStorage *retval=NULL;
  char *mode_descr = "How to open storage.  Valid values are: "
    "\"append\" (appends to existing storage or creates a new one); "
    "\"r\" (read-only); "
    "\"w\" (truncate existing storage or create a new one)";
  DLiteOpt opts[] = {
    {'m', "mode",    "append", mode_descr},
    {0, NULL, NULL, NULL}
  };
  char *optcopy = (options) ? strdup(options) : NULL;
  const char **mode = &opts[0].value;
  UNUSED(api);

  if (dlite_option_parse(optcopy, opts, 1)) goto fail;

  H5open();  /* Opens hdf5 library */

  if (!(s = calloc(1, sizeof(DH5Storage))))
   FAILCODE(dliteMemoryError, "allocation failure");

  s->flags |= dliteGeneric;
  if (strcmp(*mode, "append") == 0) {  /* default */
    s->root = H5Fopen(uri, H5F_ACC_RDWR | H5F_ACC_CREAT, H5P_DEFAULT);
    s->flags |= dliteReadable;
    s->flags |= dliteWritable;
  } else if (strcmp(*mode, "r") == 0) {
    s->root = H5Fopen(uri, H5F_ACC_RDONLY, H5P_DEFAULT);
    s->flags |= dliteReadable;
    s->flags &= ~dliteWritable;
  } else if (strcmp(*mode, "rw") == 0) {
    s->root = H5Fopen(uri, H5F_ACC_RDWR, H5P_DEFAULT);
    s->flags |= dliteReadable;
    s->flags &= ~dliteWritable;
  } else if (strcmp(*mode, "w") == 0) {
    s->root = H5Fcreate(uri, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    s->flags &= ~dliteReadable;
    s->flags |= dliteWritable;
  } else {
    FAILCODE1(dliteValueError, "invalid \"mode\" value: '%s'. Must be \"append\", \"r\" "
          "(read-only) or \"w\" (write)", *mode);
  }

  if (s->root < 0)
    FAILCODE2(dliteIOError, "cannot open: '%s' with options '%s'", uri, options);

  s->idflag = dliteIDTranslateToUUID;

  retval = (DLiteStorage *)s;
 fail:
  if (optcopy) free(optcopy);
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
    nerr += err(dliteIOError, "cannot close %s", s->location);
  return nerr;
}


DLiteDataModel *dh5_datamodel(const DLiteStorage *s, const char *uuid)
{
  DH5DataModel *d;
  DLiteDataModel *retval=NULL;
  DH5Storage *sh5 = (DH5Storage *)s;
  htri_t exists;

  if (!(d = calloc(1, sizeof(DH5DataModel))))
   FAILCODE(dliteMemoryError, "allocation failure");

  if ((exists = H5Lexists(sh5->root, uuid, H5P_DEFAULT)) < 0)
    FAILCODE2(dliteIOError, "cannot determine if '%s' exists in %s", uuid, sh5->location);

  if (exists) {
    /* Instance `uuid` already exists: assign groups */
    if ((d->instance = H5Gopen(sh5->root, uuid, H5P_DEFAULT)) < 0)
      FAILCODE2(dliteIOError, "cannot open instance '/%s' in '%s'", uuid, sh5->location);

    if ((d->meta = H5Gopen(d->instance, "meta", H5P_DEFAULT)) < 0)
      FAILCODE2(dliteIOError, "cannot open '/%s/meta' in '%s'", uuid, sh5->location);

    if ((d->dimensions = H5Gopen(d->instance, "dimensions", H5P_DEFAULT)) < 0)
      FAILCODE2(dliteIOError, "cannot open '/%s/dimensions' in '%s'", uuid, sh5->location);

    if ((d->properties = H5Gopen(d->instance, "properties", H5P_DEFAULT)) < 0)
      FAILCODE2(dliteIOError, "cannot open '/%s/properties' in '%s'", uuid, sh5->location);

  } else {
    /* Instance `uuid` does not exists: create new group structure */
    if (!(s->flags & dliteWritable))
      FAILCODE2(dliteIOError, "cannot create new instance '%s' in read-only storage %s",
            uuid, sh5->location);
    if ((d->instance = H5Gcreate(sh5->root, uuid, H5P_DEFAULT, H5P_DEFAULT,
                                 H5P_DEFAULT)) < 0)
      FAILCODE2(dliteIOError, "cannot open/create group '/%s' in '%s'", uuid, sh5->location);

    if ((d->meta = H5Gcreate(d->instance, "meta", H5P_DEFAULT, H5P_DEFAULT,
                             H5P_DEFAULT)) < 0)
      FAILCODE2(dliteIOError, "cannot create group '/%s/meta' in '%s'", uuid, sh5->location);

    if ((d->dimensions = H5Gcreate(d->instance, "dimensions", H5P_DEFAULT,
                                   H5P_DEFAULT, H5P_DEFAULT)) < 0)
      FAILCODE2(dliteIOError, "cannot create group '/%s/dimensions' in '%s'",
            uuid, sh5->location);

    if ((d->properties = H5Gcreate(d->instance, "properties", H5P_DEFAULT,
                                   H5P_DEFAULT, H5P_DEFAULT)) < 0)
      FAILCODE2(dliteIOError, "cannot create group '/%s/properties' in '%s'",
            uuid, sh5->location);
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
    nerr += err(dliteIOError, "cannot close /%s/properties in %s",
                d->uuid, d->s->location);
  if (H5Gclose(dh5->dimensions) < 0)
    nerr += err(dliteIOError, "cannot close /%s/dimensions in %s",
                d->uuid, d->s->location);
  if (H5Gclose(dh5->meta) < 0)
    nerr += err(dliteIOError, "cannot close /%s/meta in %s", d->uuid, d->s->location);
  if (H5Gclose(dh5->instance) < 0)
    nerr += err(dliteIOError, "cannot close /%s in %s", d->uuid, d->s->location);
  return nerr;
}


/**
  Returns pointer to (malloc'ed) metadata uri or NULL on error.
 */
char *dh5_get_meta_uri(const DLiteDataModel *d)
{
  const DH5DataModel *dh5 = (DH5DataModel *)d;
  char *name=NULL, *version=NULL, *namespace=NULL, *uri=NULL;

  if (get_data(d, dh5->meta, "name", &name, dliteStringPtr,
               sizeof(char *), 1, NULL) < 0) goto fail;
  if (get_data(d, dh5->meta, "version", &version, dliteStringPtr,
               sizeof(char *), 1, NULL) < 0) goto fail;
  if (get_data(d, dh5->meta, "namespace", &namespace, dliteStringPtr,
               sizeof(char *), 1,NULL) < 0) goto fail;

  /* combine to name, version and namespace to an uri */
  uri = dlite_join_meta_uri(name, version, namespace);

 fail:
  if (name) free(name);
  if (version) free(version);
  if (namespace) free(namespace);
  return uri;
}


/**
  Returns the size of dimension `name` or -1 on error.
 */
int dh5_get_dimension_size(const DLiteDataModel *d, const char *name)
{
  DH5DataModel *dh5 = (DH5DataModel *)d;
  int dimsize;
  if (get_data(d, dh5->dimensions, name, (void *)&dimsize, dliteInt,
               sizeof(dimsize), 1, NULL) < 0)
    return err(dliteIOError, "cannot get size of dimension '%s'", name);
  return dimsize;
}


/**
  Copies property `name` to memory pointed to by `ptr`.
  Returns non-zero on error.
 */
int dh5_get_property(const DLiteDataModel *d, const char *name, void *ptr,
                     DLiteType type, size_t size,
                     size_t ndims, const size_t *shape)
{
  DH5DataModel *dh5 = (DH5DataModel *)d;
  return get_data(d, dh5->properties, name, ptr, type, size, ndims, shape);
}


/********************************************************************
 * Optional api
 ********************************************************************/

/**
  Sets metadata uri.  Returns non-zero on error.
*/
int dh5_set_meta_uri(DLiteDataModel *d, const char *uri)
{
  DH5DataModel *dh5 = (DH5DataModel *)d;
  size_t shape[1]={1};
  char *name, *version, *namespace;

  if (dlite_split_meta_uri(uri, &name, &version, &namespace))
    return 1;

  set_data(d, dh5->meta, "name", name, dliteFixString,
           strlen(name), 1, shape);
  set_data(d, dh5->meta, "version", version, dliteFixString,
           strlen(version), 1, shape);
  set_data(d, dh5->meta, "namespace", namespace, dliteFixString,
           strlen(namespace), 1, shape);

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
  size_t shape[1]={1};
  int64_t dsize=size;

  return set_data(d, dh5->dimensions, name, &dsize, dliteInt,
                  sizeof(int64_t), 1, shape);
}


/**
  Sets property `name` to the memory (of `size` bytes) pointed to by `ptr`.
  Returns non-zero on error.
*/
int dh5_set_property(DLiteDataModel *d, const char *name, const void *ptr,
                     DLiteType type, size_t size,
                     size_t ndims, const size_t *shape)
{
  DH5DataModel *dh5 = (DH5DataModel *)d;
  return set_data(d, dh5->properties, name, ptr, type, size, ndims, shape);
}


/**
  Returns a NULL-terminated array of string pointers to instance UUID's.
  The caller is responsible to free the returned array.

  Returns NULL (or a partly  on error.
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
    FAILCODE(dliteAttributeError, "error finding instances");

  for (n=0, e=entries; e; e=e->next) n++;
  if (!(names = calloc((n + 1), sizeof(char *))))
   FAILCODE(dliteMemoryError, "allocation failure");
  for (i=0, e=entries; e; i++, e=e->next) {
    size_t len = strlen(e->name) + 1;
    if (!(names[i] = malloc(len)))
     FAILCODE(dliteMemoryError, "allocation failure");
    memcpy(names[i], e->name, len);
  }

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
    return err(dliteIndexError, "cannot determine if '%s' has dimension '%s' in %s",
               d->uuid, name, d->s->location);
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
    return err(dliteAttributeError, "cannot determine if '%s' has property '%s' in %s",
               d->uuid, name, d->s->location);
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



static DLiteStoragePlugin h5_plugin = {
  "hdf5",
  NULL,

  /* basic api */
  dh5_open,                            // open
  dh5_close,                           // close
  NULL,                                // flush
  NULL,                                // help

  /* query api */
  NULL,                                // iterCreate
  NULL,                                // iterNext
  NULL,                                // iterFree

  /* direct api */
  NULL,                                // loadInstance
  NULL,                                // saveInstance
  NULL,                                // deleteInstance

  /* In-memory api */
  NULL,                                // memLoadInstance
  NULL,                                // memSaveInstance

  /* === API to deprecate === */
  dh5_get_uuids,

  /* datamodel api */
  dh5_datamodel,
  dh5_datamodel_free,

  dh5_get_meta_uri,
  NULL, // resolve dimensions
  dh5_get_dimension_size,
  dh5_get_property,

  /* -- datamodel api (optional) */
  dh5_set_meta_uri,
  dh5_set_dimension_size,
  dh5_set_property,

  dh5_has_dimension,
  dh5_has_property,

  dh5_get_dataname,
  dh5_set_dataname,

  /* internal data */
  NULL
};


DSL_EXPORT const DLiteStoragePlugin *
get_dlite_storage_plugin_api(void *state, int *iter)
{
  UNUSED(iter);
  dlite_globals_set(state);
  return &h5_plugin;
}
