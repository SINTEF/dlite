#include <assert.h>
#include <stdio.h>
#include <ctype.h>

#include "err.h"
#include "map.h"
#include "tgen.h"

#include "dlite-macros.h"
#include "dlite.h"


/* Generator function that simply copies the template.

   This might be useful to e.g. apply formatting. Should it be a part of
   tgen?
*/
static int copy(TGenBuf *s, const char *template, int len,
		const TGenSubs *subs, void *context)
{
  return tgen_append(s, template, len, subs, context);
}

/* Generator function for listing dimensions. */
static int list_dimensions(TGenBuf *s, const char *template, int len,
                           const TGenSubs *subs, void *context)
{
  int retval = 1;
  DLiteInstance *inst = (DLiteInstance *)context;
  DLiteMeta *meta = (DLiteMeta *)context;
  int ismeta = dlite_meta_is_metameta(meta->meta);
  TGenSubs dsubs;
  size_t i;
  if (tgen_subs_copy(&dsubs, subs)) goto fail;
  for (i=0; i < inst->meta->ndimensions; i++) {

    if (ismeta) {
      tgen_subs_set(&dsubs, "dim_name", meta->dimensions[i].name, NULL);
      tgen_subs_set(&dsubs, "dim_descr", meta->dimensions[i].description, NULL);
    }
    tgen_subs_set_fmt(&dsubs, "dim_value", NULL, "%lu", DLITE_DIM(inst, i));
    tgen_subs_set_fmt(&dsubs, "dim_i",     NULL, "%lu", i);

    tgen_append(s, template, len, &dsubs, inst);
  }
  retval = 0;
 fail:
  tgen_subs_deinit(&dsubs);
  return retval;
}


/* Generator function for listing property dimensions. */
/*
static int list_dims(TGenBuf *s, const char *template, int len,
                     const TGenSubs *subs, void *context)
{
}
*/


/* Generator function for listing properties. */
static int list_properties(TGenBuf *s, const char *template, int len,
                           const TGenSubs *subs, void *context)
{
  int retval = 1;
  DLiteMeta *meta = (DLiteMeta *)context;
  TGenSubs psubs;
  size_t i;
  if (!dlite_meta_is_metameta(meta->meta))
    return err(TGenSyntaxError,
               "\"list_properties\" is only works for metadata");

  if (tgen_subs_copy(&psubs, subs)) goto fail;
  for (i=0; i < meta->nproperties; i++) {
    DLiteProperty *p = meta->properties + i;
    const char *type = dlite_type_get_dtypename(p->type);
    char typename[32], cdecl[32];
    char *unit = (p->unit) ? p->unit : "";
    char *descr = (p->description) ? p->description : "";
    size_t nref = (p->ndims > 0) ? 1 : 0;
    dlite_type_set_typename(p->type, p->size, typename, sizeof(typename));
    dlite_type_set_cdecl(p->type, p->size, p->name, nref, cdecl, sizeof(cdecl));

    tgen_subs_set(&psubs, "prop_name",     p->name,  NULL);
    tgen_subs_set(&psubs, "prop_type",     type,     NULL);
    tgen_subs_set(&psubs, "prop_typename", typename, NULL);
    tgen_subs_set(&psubs, "prop_cdecl",    cdecl,    NULL);
    tgen_subs_set(&psubs, "prop_unit",     unit,     NULL);
    tgen_subs_set(&psubs, "prop_descr",    descr,    NULL);
    tgen_subs_set_fmt(&psubs, "prop_typeno", NULL, "%d",  p->type);
    tgen_subs_set_fmt(&psubs, "prop_size",   NULL, "%lu", p->size);
    tgen_subs_set_fmt(&psubs, "prop_ndims",  NULL, "%d",  p->ndims);
    //tgen_subs_set_fmt(&psubs, "prop_dims",   NULL, "%d",  p->ndims);
    tgen_subs_set_fmt(&psubs, "prop_i",      NULL, "%lu", i);

    tgen_append(s, template, len, &psubs, meta);
  }
  retval = 0;
 fail:
  tgen_subs_deinit(&psubs);
  return retval;
}


/* Generator function for listing relations. */
/*
static int list_relations(TGenBuf *s, const char *template, int len,
                          const TGenSubs *subs, void *context)
{
}
*/


/*
  Assign/update substitutions based on the instance `inst`.

  Returns non-zero on error.
*/
int instance_subs(TGenSubs *subs, const DLiteInstance *inst)
{
  char *name, *version, *namespace, **descr;
  const DLiteMeta *meta = inst->meta;

  /* General (all types of instances) */
  tgen_subs_set_fmt(subs, "uuid", NULL, "\"%s\"", inst->uuid);
  if (inst->uri)
    tgen_subs_set(subs, "uri",        inst->uri,  NULL);

  /* About metadata */
  dlite_split_meta_uri(meta->uri, &name, &version, &namespace);
  descr = dlite_instance_get_property((DLiteInstance *)meta, "description");
  tgen_subs_set(subs, "meta_uri",        meta->uri,  NULL);
  tgen_subs_set(subs, "meta_name",       name,       NULL);
  tgen_subs_set(subs, "meta_version",    version,    NULL);
  tgen_subs_set(subs, "meta_namespace",  namespace,  NULL);
  tgen_subs_set(subs, "meta_descr",      *descr,     NULL);

  /* DLiteInstance_HEAD */
  tgen_subs_set_fmt(subs, "_uuid",     NULL, "\"%s\"", inst->uuid);
  if (inst->uri)
    tgen_subs_set_fmt(subs, "_uri",      NULL, "\"%s\"", inst->uri);
  else
    tgen_subs_set_fmt(subs, "_uri",      NULL, "NULL");
  tgen_subs_set_fmt(subs, "_refcount", NULL, "0");
  tgen_subs_set_fmt(subs, "_meta",     NULL, "NULL");

  /* For all metadata  */
  if (dlite_meta_is_metameta(inst->meta)) {
    DLiteMeta *meta = (DLiteMeta *)inst;
    dlite_split_meta_uri(inst->uri, &name, &version, &namespace);
    descr = dlite_instance_get_property((DLiteInstance *)meta, "description");
    tgen_subs_set(subs, "name",       name,       NULL);
    tgen_subs_set(subs, "version",    version,    NULL);
    tgen_subs_set(subs, "namespace",  namespace,  NULL);
    tgen_subs_set(subs, "descr",      *descr,     NULL);

  /* DLiteMeta_HEAD */
    tgen_subs_set_fmt(subs, "_ndimensions", NULL, "%lu", meta->ndimensions);
    tgen_subs_set_fmt(subs, "_nproperties", NULL, "%lu", meta->nproperties);
    tgen_subs_set_fmt(subs, "_nrelations",  NULL, "%lu", meta->nrelations);

    tgen_subs_set_fmt(subs, "_headersize",  NULL, "0");
    tgen_subs_set_fmt(subs, "_init",        NULL, "NULL");
    tgen_subs_set_fmt(subs, "_deinit",      NULL, "NULL");

    tgen_subs_set_fmt(subs, "_dimoffset",   NULL, "0");
    tgen_subs_set_fmt(subs, "_propoffsets", NULL, "NULL");
    tgen_subs_set_fmt(subs, "_reloffset",   NULL, "0");
    tgen_subs_set_fmt(subs, "_pooffset",    NULL, "0");
  }

  /* Lists */
  tgen_subs_set(subs, "list_dimensions", NULL, list_dimensions);
  tgen_subs_set(subs, "list_properties", NULL, list_properties);

  return 0;
}


/*
  Assign/update substitutions based on `options`.

  Returns non-zero on error.
*/
int option_subs(TGenSubs *subs, const char *options)
{
  const char *v, *k = options;
  while (*k && *k != '#') {
    size_t vlen, klen = strcspn(k, "=;&#");
    if (k[klen] != '=')
      return errx(1, "no value for key '%.*s' in option string '%s'",
                  (int)klen, k, options);
    v = k + klen + 1;
    vlen = strcspn(v, ";&#");
    tgen_subs_setn_fmt(subs, k, klen, NULL, "%.*s", vlen, v);
    k = v + vlen;
    if (*k) k++;
  }
  return 0;
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
  tgen_subs_init(&subs);
  if (instance_subs(&subs, inst)) return NULL;
  if (option_subs(&subs, options)) return NULL;
  text = tgen(template, &subs, (void *)inst);
  tgen_subs_deinit(&subs);
  return text;
}
