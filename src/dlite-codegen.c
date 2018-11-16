#include <assert.h>
#include <stdio.h>
#include <ctype.h>

#include "err.h"
#include "map.h"
#include "tgen.h"

#include "dlite-macros.h"
#include "dlite.h"


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



#if 0
static DLiteOpt option_list[] = {
  {'h', "header", NULL, "Name of C header file to produce."},
  {'c', "source", NULL, "Name of C source file to produce."},
  {'j', "json",   NULL, "Name of JSON file to produce."},
  {'i', "init",   NULL, "Name of init() function."},
  {'d', "deinit", NULL, "Name of deinit() function."},
  {'n', "name",   NULL, "Name of generated structure."},
  {0, NULL, NULL, NULL}
};


typedef struct {
  TGenSubstitution *subs;
  int size;
  int nsubs;
  map_int_t map;
} DLiteSubs;

/*
struct DLiteProperty instance_header[] = {
  {"_uuid", dliteFixString, 36+1, 0, NULL, NULL,
   "UUID for this data instance."},
  {"_uri", dliteStringPtr, sizeof(char *), 0, NULL, NULL,
   "Unique name or uri of the data instance.  May be NULL."},
  {"_refcount", dliteInt, sizeof(int), 0, NULL, NULL,
   "Unique name or uri of the data instance.  May be NULL."},
};
*/



static char *template_header[] = {
  "/* This file is generated with dlite-codegen -- do not edit! */",
  "",
  "/** {descr} */",
  "#ifndef _{NAME}_H",
  "#define _{NAME}_H",
  "",
  "#include \"boolean.h\"",
  "#include \"integers.h\"",
  "#include \"floats.h\"",
  "",
  "enum {{",
  "{enum_dimensions}",
  "}};",
  "",
  "enum {{",
  "{enum_properties}",
  "}};",
  "",
  "",
  "typedef struct _{Name} {{",
  "  /* -- header */",
  "  char uuid[36+1];   /*!< UUID for this data instance. */",
  "  const char *uri;   /*!< Unique name or uri of the data instance.",
  "                          Can be NULL. */",
  "  size_t refcount;   /*!< Number of references to this instance. */",
  "  const void *meta;  /*!< Pointer to the metadata describing this ",
  "                          instance. */",
  "",
  "  /* -- dimension values */",
  "{cdecl_dimensions:  {cdecl};  /*!< {descr} */\n}",
  "",
  "  /* -- property values */",
  "{cdecl_properties:  {cdecl};  /*!< {descr} */\n}",
  "}} {Name};",
  "",
  "#endif /* _{NAME}_H */",
  NULL
};


/* Initiates memory used by `s`. */
static int init_subs(DLiteSubs *s)
{
  memset(s, 0, sizeof(DLiteSubs));
  map_init(&s->map);
}

/* Cleans up memory used by `s`, but not the memory pointed to by `s`. */
static int clear_subs(DLiteSubs *s)
{
  map_deinit(&s->map);
  if (s->subs) free(s->subs);
  memset(s, 0, sizeof(DLiteSubs));
}



/* Adds substitution to array `s`. Returns non-zero on error. */
static int set_sub(DLiteSubs *s, const char *var, const char *repl, TGenSub fn)
{
  int *p;
  if (!s->subs) init_subs(s);

  if ((p = map_get(&s->map, var))) {
    int i = *p;
    assert(s->subs[i].var == var);
    s->subs[i].repl = repl;
    s->subs[i].sub = fn;
  } else {
    TGenSubstitution sub;
    if (s->nsubs >= s->size) {
      s->size += 64;
      if (!(s->subs = realloc(s->subs, s->size*sizeof(TGenSubstitution))))
        return err(1, "allocation failure");
    }
    sub.var = (char *)var;
    sub.repl = (char *)repl;
    sub.sub = fn;
    map_set(&s->map, var, &s->nsubs);

    memcpy(s->subs + s->nsubs++, &sub, sizeof(TGenSubstitution));
    memset(s->subs + s->nsubs, 0, sizeof(TGenSubstitution));
  }
  return 0;
}


/* Add substitutions from instance inst. */
int dlite_instance_substitutions(DLiteSubs *s, const DLiteInstance *inst)
{
  const char *uri;
  char *Name=NULL, *version=NULL, *namespace=NULL;
  char **q, *name=NULL, *NAME=NULL, **descr;
  int i, retval=1;

  uri = inst->uri;
  if (uri) {
    dlite_split_meta_uri(inst->uri, &name, &version, &namespace);
  } else if ((q = dlite_instance_get_property(inst, "name")) &&
             (name = strdup(*q)) &&
             (q = dlite_instance_get_property(inst, "version")) &&
             (version = strdup(*q)) &&
             (q = dlite_instance_get_property(inst, "namespace")) &&
             (namespace = strdup(*q))) {
    uri = dlite_join_meta_uri(name, version, namespace);
  }
  if (Name) {
    name = strdup(Name);
    NAME = strdup(Name);
    for (i=0; name[i]; i++) name[i] = tolower(name[i]);
    for (i=0; NAME[i]; i++) NAME[i] = toupper(NAME[i]);
  }

  set_sub(s, "uuid", inst->uuid, NULL);
  set_sub(s, "uri", uri, NULL);

  set_sub(s, "Name", Name, NULL);
  set_sub(s, "name", name, NULL);
  set_sub(s, "NAME", NAME, NULL);

  set_sub(s, "version", version, NULL);
  set_sub(s, "namespace", namespace, NULL);

  if ((descr = dlite_instance_get_property(inst, "description")))
    set_sub(s, "descr", *descr, NULL);
  else
    set_sub(s, "descr", "", NULL);

  set_sub(s, "enum_dimensions", "xx", NULL);
  set_sub(s, "enum_properties", "xx", NULL);

  set_sub(s, "cdecl_dimensions", "xx", NULL);
  set_sub(s, "cdecl_properties", "xx", NULL);

  retval = 0;
  //fail:
  if (NAME) free(NAME);
  if (name) free(name);
  if (version) free(version);
  if (namespace) free(namespace);
  return retval;
}


/*
  Returns a newly malloc'ed NULL-terminated array of substitutions.
 */
DLiteSubs *dlite_codegen_substitutions(const DLiteInstance *inst,
                                             DLiteOpt *opts)
{


  int size=64, nsubs=0;
  TGenSubstitution *subs = malloc(size*sizeof(TGenSubstitution));





  DLiteMeta *meta = (DLiteMeta *)inst;



  UNUSED(inst);
  UNUSED(opts);

  if (!(subs = malloc(size*sizeof(TGenSubstitution))))
    return err(1, "allocation failure"), NULL;

  return subs;
}

/*

 */
char *dlite_codegen_header(const TGenSubstitution *subs)
{
  UNUSED(subs);
  return NULL;
}


/*
  Returns non-zero on error.
 */
int dlite_codegen(const DLiteInstance *inst, const char *options)
{
  DLiteOpt opts[countof(option_list)];
  char *optcopy = strdup(options);;
  char *code = NULL;
  const TGenSubstitution *subs, *sub;
  int retval=-1;
  FILE *fp;

  memcpy(opts, option_list, sizeof(option_list));
  dlite_option_parse(optcopy, opts, 1);
  subs = dlite_codegen_substitutions(inst, opts);

  /* write C header */
  sub = tgen_get_substitution(subs, -1, "header", -1);
  assert(sub);
  if (sub->repl) {
    if (!(fp = fopen(sub->repl, "w")))
      FAIL1("cannot open output header file: %s", sub->repl);
    if (!(code = dlite_codegen_header(subs))) goto fail;
    fprintf(fp, "%s\n", code);
    fclose(fp);
  }

  /* write C source */

  retval = 0;
 fail:
  free(optcopy);
  return retval;
}
#endif
