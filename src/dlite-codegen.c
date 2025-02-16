#include <assert.h>
#include <stdio.h>
#include <ctype.h>

#include "utils/compat.h"
#include "utils/err.h"
#include "utils/tgen.h"
#include "utils/fileutils.h"

#include "config.h"
#include "config-paths.h"

#include "dlite.h"
#include "dlite-macros.h"
#include "dlite-codegen.h"

#define GLOBALS_ID "dlite-codegen-globals-id"


/* Context for looping over properties */
typedef struct {
  const DLiteInstance *inst;  /* pointer to the instance */
  int iprop;                  /* index of current property or -1 */
  int metameta;               /* whether to list inst->meta instead of inst */
}  Context;

typedef struct {
  /* Pointer to template paths.  Access it via dlite_codegen_path_get(). */
  FUPaths template_paths;

  /* Whether template_paths are initialised */
  int template_paths_initialised;

  /* Whether native typenames should be used.  Defaults to portable type names. */
  int use_native_typenames;
} Globals;


/* Free global state for this module */
static void free_globals(void *globals)
{
  Globals *g = globals;
  dlite_codegen_path_free();
  free(g);
}

/* Return a pointer to global state for this module */
static Globals *get_globals(void)
{
  Globals *g = dlite_globals_get_state(GLOBALS_ID);
  if (!g) {
    if (!(g = calloc(1, sizeof(Globals))))
     FAILCODE(dliteMemoryError, "allocation failure");

    dlite_globals_add_state(GLOBALS_ID, g, free_globals);
  }
  return g;
 fail:
  if (g) free(g);
  return NULL;
}


/* Generator function that simply copies the template.

   This might be useful to e.g. apply formatting. Should it be a part of
   tgen?
*/
static int copy(TGenBuf *s, const char *template, int len,
		TGenSubs *subs, void *context)
{
  return tgen_append(s, template, len, subs, context);
}


/* Help function for list_dimensions.  If `metameta` is non-zero,
   `subs` is assigned the dimensions of `meta->meta`, otherwise it is
   assigned the dimensions of `meta`. Returns non-zero on error. */
static int list_dimensions_helper(TGenBuf *s, const char *template, int len,
                                  TGenSubs *subs, void *context,
                                  int metameta)
{
  int retval = 0;
  const DLiteInstance *inst = ((Context *)context)->inst;
  const DLiteMeta *meta = (const DLiteMeta *)inst;
  const DLiteMeta *m = (metameta) ? meta->meta : meta;
  TGenSubs dsubs;
  size_t i;
  if (!dlite_meta_is_metameta(meta->meta))
    return err(TGenSyntaxError,
               "\"list_dimensions\" only works for metadata");

  if ((retval = tgen_subs_copy(&dsubs, subs))) goto fail;
  dsubs.parent = subs;
  for (i=0; i < m->_ndimensions; i++) {
    DLiteDimension *d = m->_dimensions + i;
    tgen_subs_set(&dsubs, "dim.name", d->name, NULL);
    tgen_subs_set(&dsubs, "dim.descr", d->description, NULL);
    tgen_subs_set_fmt(&dsubs, "dim.value", NULL, "%lu", (unsigned long)
                      dlite_instance_get_dimension_size_by_index(inst, i));
    tgen_subs_set_fmt(&dsubs, "dim.i",     NULL, "%lu", (unsigned long)i);
    tgen_subs_set(&dsubs, ",",  (i < m->_ndimensions-1) ? ","  : "", NULL);
    tgen_subs_set(&dsubs, ", ", (i < m->_ndimensions-1) ? ", " : "", NULL);
    if ((retval = tgen_append(s, template, len, &dsubs, context))) goto fail;
  }
 fail:
  tgen_subs_deinit(&dsubs);
  return retval;
}


/* Generator function for listing relations. */
static int list_relations(TGenBuf *s, const char *template, int len,
                          TGenSubs *subs, void *context)
{
  int retval = 0;
  DLiteMeta *meta = (DLiteMeta *)((Context *)context)->inst;
  TGenSubs rsubs;
  size_t i;
  if (!dlite_meta_is_metameta(meta->meta))
    return err(TGenSyntaxError,
               "\"list_relations\" only works for metadata");

  if ((retval = tgen_subs_copy(&rsubs, subs))) goto fail;
  rsubs.parent = subs;
  for (i=0; i < meta->_nrelations; i++) {
    DLiteRelation *r = meta->_relations + i;
    tgen_subs_set(&rsubs, "rel.s",  r->s, NULL);
    tgen_subs_set(&rsubs, "rel.p",  r->p, NULL);
    tgen_subs_set(&rsubs, "rel.o",  r->o, NULL);
    tgen_subs_set(&rsubs, "rel.id", r->id, NULL);
    tgen_subs_set_fmt(&rsubs, "rel.i", NULL, "%lu", (unsigned long)i);
    tgen_subs_set(&rsubs, ",",  (i < meta->_nrelations-1) ? ","  : "", NULL);
    tgen_subs_set(&rsubs, ", ", (i < meta->_nrelations-1) ? ", " : "", NULL);
    if ((retval = tgen_append(s, template, len, &rsubs, context))) goto fail;
  }
 fail:
  tgen_subs_deinit(&rsubs);
  return retval;
}


/* Generator function for listing property dimensions. */
static int list_dims(TGenBuf *s, const char *template, int len,
                     TGenSubs *subs, void *context)
{
  int retval = 1;
  Context *c = (Context *)context;
  DLiteMeta *meta = (DLiteMeta *)((c->metameta) ? c->inst->meta : (const DLiteMeta *)c->inst);
  int iprop = ((Context *)context)->iprop;
  DLiteProperty *p = meta->_properties + iprop;
  TGenSubs psubs;
  int i;
  if (!dlite_meta_is_metameta(meta->meta))
    return err(TGenSyntaxError,
               "\"list_dims\" only works for metadata");
  if (iprop >= (int)meta->_nproperties)
    return err(1, "RuntimeError: iprop=%d is out of range: (0:%lu)",
               iprop, (unsigned long)(meta->_nproperties-1));

  if (tgen_subs_copy(&psubs, subs)) goto fail;
  psubs.parent = subs;
  for (i=0; i < p->ndims; i++) {
    tgen_subs_set(&psubs, "dim.name",  p->shape[i] , NULL);
    tgen_subs_set_fmt(&psubs, "dim.i",     NULL, "%d",  i);
    tgen_subs_set(&psubs, ",",  (i < p->ndims-1) ? ","  : "", NULL);
    tgen_subs_set(&psubs, ", ", (i < p->ndims-1) ? ", " : "", NULL);
    if ((retval = tgen_append(s, template, len, &psubs, context))) goto fail;
  }
  retval = 0;
 fail:
  tgen_subs_deinit(&psubs);
  return retval;
}


/* Help function for list_properties.  If `metameta` is non-zero,
   `subs` is assigned the properties of `meta->meta`, otherwise it is
   assigned the properties of `meta`. Returns non-zero on error. */
static int list_properties_helper(TGenBuf *s, const char *template, int len,
                                  TGenSubs *subs, void *context,
                                  int metameta)
{
  Globals *g = get_globals();
  int retval = 0;
  const DLiteMeta *meta = (const DLiteMeta *)((Context *)context)->inst;
  const DLiteMeta *m = (metameta) ? meta->meta : meta;
  TGenSubs psubs;
  char *meta_name=NULL, *meta_uname=NULL;;
  size_t i;
  if (!dlite_meta_is_metameta(meta->meta))
    return err(TGenSyntaxError,
               "\"list_properties\" only works for metadata");

  if (metameta) {
    dlite_split_meta_uri(meta->uri, &meta_name, NULL, NULL);
    meta_uname = tgen_convert_case(meta_name, -1, 'u');
  }

  if ((retval = tgen_subs_copy(&psubs, subs))) goto fail;
  psubs.parent = subs;

  for (i=0; i < m->_nproperties; i++) {
    DLiteProperty *p = m->_properties + i;
    const char *type = dlite_type_get_dtypename(p->type);
    const char *dtype = dlite_type_get_enum_name(p->type);
    char *ref = (p->ref) ? p->ref : "NULL";
    char *unit = (p->unit) ? p->unit : "";
    char *descr = (p->description) ? p->description : "";
    size_t nref = (p->ndims > 0) ? 1 : 0;
    int isallocated = dlite_type_is_allocated(p->type);
    char typename[32], pcdecl[64], ftype[25], isoctype[64];
    dlite_type_set_typename(p->type, p->size, typename, sizeof(typename));
    dlite_type_set_cdecl(p->type, p->size, p->name, nref, pcdecl,
			 sizeof(pcdecl), g->use_native_typenames);
    dlite_type_set_ftype(p->type, p->size, ftype, sizeof(ftype));
    dlite_type_set_isoctype(p->type, p->size, isoctype, sizeof(isoctype));

    ((Context *)context)->iprop = i;
    ((Context *)context)->metameta = metameta;
    tgen_subs_set(&psubs, "prop.name",     p->name,  NULL);
    tgen_subs_set(&psubs, "prop.type",     type,     NULL);
    tgen_subs_set(&psubs, "prop.typename", typename, NULL);
    tgen_subs_set(&psubs, "prop.dtype",    dtype,    NULL);
    tgen_subs_set(&psubs, "prop.ftype",    ftype,    NULL);
    tgen_subs_set(&psubs, "prop.isoctype", isoctype, NULL);
    tgen_subs_set(&psubs, "prop.cdecl",    pcdecl,   NULL);
    tgen_subs_set(&psubs, "prop.ref",      ref,      NULL);
    tgen_subs_set(&psubs, "prop.unit",     unit,     NULL);
    tgen_subs_set(&psubs, "prop.descr",    descr,    NULL);
    tgen_subs_set(&psubs, "prop.shape",     NULL,     list_dims);
    tgen_subs_set_fmt(&psubs, "prop.typeno",      NULL, "%d",  p->type);
    tgen_subs_set_fmt(&psubs, "prop.size",        NULL, "%u",(unsigned)p->size);
    tgen_subs_set_fmt(&psubs, "prop.ndims",       NULL, "%d",  p->ndims);
    tgen_subs_set_fmt(&psubs, "prop.isallocated", NULL, "%d",  isallocated);
    tgen_subs_set_fmt(&psubs, "prop.i",           NULL, "%u", (unsigned)i);
    tgen_subs_set_fmt(&psubs, "prop.dimind",      NULL, "%u",
                      (unsigned)m->_propdiminds[i]);
    tgen_subs_set(&psubs, ",",  (i < m->_nproperties-1) ? ","  : "", NULL);
    tgen_subs_set(&psubs, ", ", (i < m->_nproperties-1) ? ", " : "", NULL);

    if (metameta) {
      if (p->ndims == 0 && p->type == dliteStringPtr) {
        char **ptr = dlite_instance_get_property((DLiteInstance *)meta,
                                                 p->name);
        tgen_subs_set_fmt(&psubs, "prop.value",  NULL, "%s", *ptr);
        tgen_subs_set_fmt(&psubs, "prop.cvalue", NULL, "\"%s\"", *ptr);
      } else {
        const TGenSub *sub;
        tgen_subs_set_fmt(&psubs, "prop.value", NULL, "%s_%s",
                          meta_uname, p->name);
        tgen_subs_set_fmt(&psubs, "prop.cvalue", NULL, "%s_%s",
                          meta_uname, p->name);
        sub = tgen_subs_get(&psubs, "prop.value");
        tgen_setcase(sub->repl, -1, 'l');
      }
    }

    if ((retval = tgen_append(s, template, len, &psubs, context)))
      goto fail;
  }
 fail:
  ((Context *)context)->iprop = -1;
  ((Context *)context)->metameta = 0;
  tgen_subs_deinit(&psubs);
  if (meta_name) free(meta_name);
  if (meta_uname) free(meta_uname);
  return retval;
}

/* Generator function for listing property dimensions. */
static int list_propdims(TGenBuf *s, const char *template, int len,
                         TGenSubs *subs, void *context)
{
  int retval = 1;
  const DLiteInstance *inst = ((Context *)context)->inst;
  const DLiteMeta *meta = inst->meta;
  size_t *propdims = (size_t *)((char *)inst + meta->_propdimsoffset);
  TGenSubs psubs;
  size_t i;
  if (tgen_subs_copy(&psubs, subs)) goto fail;
  psubs.parent = subs;

  for (i=0; i < meta->_npropdims; i++) {
    tgen_subs_set_fmt(&psubs, "propdim.i", NULL, "%lu", (unsigned long)i);
    tgen_subs_set_fmt(&psubs, "propdim.n", NULL, "%lu",
                      (unsigned long)propdims[i]);
    tgen_subs_set(&psubs, ",",  (i < meta->_npropdims-1) ? ","  : "", NULL);
    tgen_subs_set(&psubs, ", ", (i < meta->_npropdims-1) ? ", " : "", NULL);
    if ((retval = tgen_append(s, template, len, &psubs, context))) goto fail;
  }
  retval = 0;
 fail:
  tgen_subs_deinit(&psubs);
  return retval;
}

/* Generator function for listing dimensions. */
static int list_dimensions(TGenBuf *s, const char *template, int len,
                           TGenSubs *subs, void *context)
{
  return list_dimensions_helper(s, template, len, subs, context, 0);
}

/* Generator function for listing dimensions. */
static int list_meta_dimensions(TGenBuf *s, const char *template, int len,
                                TGenSubs *subs, void *context)
{
  return list_dimensions_helper(s, template, len, subs, context, 1);
}

/* Generator function for listing properties. */
static int list_properties(TGenBuf *s, const char *template, int len,
                           TGenSubs *subs, void *context)
{
  return list_properties_helper(s, template, len, subs, context, 0);
}

/* Generator function for listing metadata properties. */
static int list_meta_properties(TGenBuf *s, const char *template, int len,
                                TGenSubs *subs, void *context)
{
  return list_properties_helper(s, template, len, subs, context, 1);
}

/* Generator function for listing metadata relations. */
static int list_meta_relations(TGenBuf *s, const char *template, int len,
                               TGenSubs *subs, void *context)
{
  Context c;
  c.inst = (DLiteInstance *)((Context *)context)->inst->meta;
  c.iprop = ((Context *)context)->iprop;
  c.metameta = 0;
  return list_relations(s, template, len, subs, &c);
}



/*
  Assign/update substitutions based on the instance `inst`.

  Returns non-zero on error.
*/
int dlite_instance_subs(TGenSubs *subs, const DLiteInstance *inst)
{
  char *name=NULL, *version=NULL, *namespace=NULL, **descr;
  const DLiteMeta *meta = inst->meta;
  int isdata=0, ismeta=0, ismetameta=0;

  /* DLite version */
  tgen_subs_set(subs, "dlite.version", dlite_VERSION, NULL);
  tgen_subs_set_fmt(subs, "dlite.version.major",NULL,"%d",dlite_VERSION_MAJOR);
  tgen_subs_set_fmt(subs, "dlite.version.minor",NULL,"%d",dlite_VERSION_MINOR);
  tgen_subs_set_fmt(subs, "dlite.version.patch",NULL,"%d",dlite_VERSION_PATCH);

  /* Determine what this data is */
  if (dlite_meta_is_metameta(meta)) {
    ismeta = 1;
    if (dlite_meta_is_metameta((DLiteMeta *)inst)) ismetameta = 1;
  } else {
    isdata = 1;
  }
  tgen_subs_set_fmt(subs, "isdata", NULL, "%d", isdata);
  tgen_subs_set_fmt(subs, "ismeta", NULL, "%d", ismeta);
  tgen_subs_set_fmt(subs, "ismetameta", NULL, "%d", ismetameta);

  /* General (all types of instances) */
  tgen_subs_set(subs, "uuid", inst->uuid, NULL);
  tgen_subs_set(subs, "uri", (inst->uri) ? inst->uri : "", NULL);
  if (inst->uri)
    tgen_subs_set(subs, "uri",        inst->uri,  NULL);

  /* About metadata */
  dlite_split_meta_uri(meta->uri, &name, &version, &namespace);
  descr = dlite_instance_get_property((DLiteInstance *)meta, "description");
  tgen_subs_set(subs, "meta.uuid",       meta->uuid, NULL);
  tgen_subs_set(subs, "meta.uri",        meta->uri,  NULL);
  tgen_subs_set(subs, "meta.name",       name,       NULL);
  tgen_subs_set(subs, "meta.version",    version,    NULL);
  tgen_subs_set(subs, "meta.namespace",  namespace,  NULL);
  tgen_subs_set(subs, "meta.descr",      *descr,     NULL);
  tgen_subs_set_fmt(subs, "meta._ndimensions", NULL, "%lu",
                    (unsigned long)meta->meta->_ndimensions);
  tgen_subs_set_fmt(subs, "meta._nproperties", NULL, "%lu",
                    (unsigned long)meta->meta->_nproperties);
  tgen_subs_set_fmt(subs, "meta._nrelations", NULL, "%lu",
                    (unsigned long)meta->meta->_nrelations);
  tgen_subs_set_fmt(subs, "meta._npropdims", NULL, "%lu",
                    (unsigned long)meta->_npropdims);
  free(name);
  free(version);
  free(namespace);

  /* DLiteInstance_HEAD */
  tgen_subs_set(subs, "_uuid", inst->uuid, NULL);
  tgen_subs_set(subs, "_uri", (inst->uri) ? inst->uri : "", NULL);

  /* For all metadata  */
  if (dlite_meta_is_metameta(inst->meta)) {
    DLiteMeta *meta = (DLiteMeta *)inst;
    dlite_split_meta_uri(inst->uri, &name, &version, &namespace);
    descr = dlite_instance_get_property((DLiteInstance *)meta, "description");

    tgen_subs_set(subs, "name",       name,       NULL);
    tgen_subs_set(subs, "version",    version,    NULL);
    tgen_subs_set(subs, "namespace",  namespace,  NULL);
    tgen_subs_set(subs, "descr",      *descr,     NULL);
    free(name);
    free(version);
    free(namespace);

  /* DLiteMeta_HEAD */
    tgen_subs_set_fmt(subs, "_ndimensions", NULL, "%lu",
                      (unsigned long)meta->_ndimensions);
    tgen_subs_set_fmt(subs, "_nproperties", NULL, "%lu",
                      (unsigned long)meta->_nproperties);
    tgen_subs_set_fmt(subs, "_nrelations",  NULL, "%lu",
                      (unsigned long)meta->_nrelations);
    tgen_subs_set_fmt(subs, "_npropdims",   NULL, "%lu",
                      (unsigned long)meta->_npropdims);

    tgen_subs_set_fmt(subs, "_headersize",  NULL, "%lu",
                      (unsigned long)meta->_headersize);
    tgen_subs_set_fmt(subs, "_init",        NULL, "NULL");
    tgen_subs_set_fmt(subs, "_deinit",      NULL, "NULL");
    tgen_subs_set_fmt(subs, "_gethash",     NULL, "NULL");
    tgen_subs_set_fmt(subs, "_getdim",      NULL, "NULL");
    tgen_subs_set_fmt(subs, "_setdim",      NULL, "NULL");
    tgen_subs_set_fmt(subs, "_loadprop",    NULL, "NULL");
    tgen_subs_set_fmt(subs, "_saveprop",    NULL, "NULL");

    tgen_subs_set_fmt(subs, "_npropdims",   NULL, "%lu",
                      (unsigned long)meta->_npropdims);

    tgen_subs_set_fmt(subs, "_dimoffset",   NULL, "%lu",
                      (unsigned long)meta->_dimoffset);
    tgen_subs_set_fmt(subs, "_reloffset",   NULL, "%lu",
                      (unsigned long)meta->_reloffset);
    tgen_subs_set_fmt(subs, "_propdimsoffset",   NULL, "%lu",
                      (unsigned long)meta->_propdimsoffset);
    tgen_subs_set_fmt(subs, "_propdimindsoffset",NULL, "%lu",
                      (unsigned long)meta->_propdimindsoffset);
  }

  /* Lists */
  tgen_subs_set(subs, "list_dimensions",      NULL, list_dimensions);
  tgen_subs_set(subs, "list_properties",      NULL, list_properties);
  tgen_subs_set(subs, "list_relations",       NULL, list_relations);
  tgen_subs_set(subs, "list_meta_dimensions", NULL, list_meta_dimensions);
  tgen_subs_set(subs, "list_meta_properties", NULL, list_meta_properties);
  tgen_subs_set(subs, "list_meta_relations",  NULL, list_meta_relations);
  tgen_subs_set(subs, "list_propdims",        NULL, list_propdims);
  tgen_subs_set(subs, ".copy",                NULL, copy);

  return 0;
}


/*
  Assign/update substitutions based on `options`.

  Returns non-zero on error.
*/
int dlite_option_subs(TGenSubs *subs, const char *options)
{
  const char *v, *k = options;
  while (k && *k && *k != '#') {
    size_t vlen, klen = strcspn(k, "=;&#");
    if (k[klen] != '=')
      return errx(1, "no value for key '%.*s' in option string '%s'",
                  (int)klen, k, options);
    v = k + klen + 1;
    vlen = strcspn(v, ";&#");
    tgen_subs_setn_fmt(subs, k, klen, NULL, "%.*s", (int)vlen, v);
    k = v + vlen;
    if (*k) k++;
  }
  return 0;
}


/* Returns a pointer to current template paths. */
FUPaths *dlite_codegen_path_get(void)
{
  Globals *g = get_globals();
  if (!g->template_paths_initialised) {
    if (fu_paths_init(&g->template_paths, "DLITE_TEMPLATE_DIRS") < 0)
      return errx(1, "failure initialising codegen template paths"), NULL;

    fu_paths_set_platform(&g->template_paths, dlite_get_platform());

    if (dlite_use_build_root())
      fu_paths_extend(&g->template_paths, dlite_TEMPLATES, NULL);
    else
      fu_paths_extend_prefix(&g->template_paths, dlite_pkg_root_get(),
                             DLITE_TEMPLATE_DIRS, NULL);

    g->template_paths_initialised = 1;
  }
  return &g->template_paths;
}

/* Free template paths */
void dlite_codegen_path_free(void)
{
  Globals *g = get_globals();
  if (g->template_paths_initialised) {
    fu_paths_deinit(&g->template_paths);
    g->template_paths_initialised = 1;
  }
}

/* Returns whether to use native typenames */
int dlite_codegen_get_native_typenames(void)
{
  Globals *g = get_globals();
  return g->use_native_typenames;
}


/* Sets whether to use native typenames. If zero, use portable type names. */
void dlite_codegen_set_native_typenames(int use_native_typenames)
{
  Globals *g = get_globals();
  g->use_native_typenames = use_native_typenames;
}


/*
  Returns a newly malloc'ed string with a generated document based on
  `template` and instanse `inst`.  `options` is a semicolon (;) separated
  string with additional options.

  Returns NULL on error.
 */
char *dlite_codegen(const char *template, const DLiteInstance *inst,
                    const char *options)
{
  TGenSubs subs;
  char *text;
  Context context;

  context.inst = inst;
  context.iprop = -1;
  context.metameta = 0;

  tgen_subs_init(&subs);
  if (dlite_instance_subs(&subs, inst)) return NULL;
  if (dlite_option_subs(&subs, options)) return NULL;
  text = tgen(template, &subs, &context);
  tgen_subs_deinit(&subs);
  return text;
}


/*
  Returns a pointer to malloc'ed template file name, given a template
  name (e.g. "c-header", "c-meta-header", "c-source", ...) or
  NULL on error.
 */
char *dlite_codegen_template_file(const char *template_name)
{
  FUPaths *paths;
  char *pattern=NULL, *retval=NULL;
  const char *template_file;
  FUIter *iter=NULL;

  if (!(paths = dlite_codegen_path_get())) return NULL;

  if (asprintf(&pattern, "%s.txt", template_name) < 0)
    FAILCODE(dliteMemoryError, "allocation failure");

  if (!(iter = fu_pathsiter_init(paths, pattern)))
    FAIL("failure creating codegen template path iterator");
  if (!(template_file = fu_pathsiter_next(iter))) {
      const char **path;
      TGenBuf msg;
      tgen_buf_init(&msg);
      tgen_buf_append_fmt(&msg, "cannot find template file \"%s\" in paths:\n",
                          template_name);
      for (path=fu_paths_get(paths); *path; path++)
        tgen_buf_append_fmt(&msg, "  - %s\n", *path);
      errx(1, "%s", tgen_buf_get(&msg));
      tgen_buf_deinit(&msg);
      goto fail;
  }
  retval = strdup(template_file);

 fail:
  if (iter) fu_pathsiter_deinit(iter);
  if (pattern) free(pattern);

  return retval;
}
