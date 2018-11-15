/* dlite-codegen.c -- tool for generating code for a DLite instance */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#ifdef HAVE_GETOPT
# include <getopt.h>
#else
# include "getopt.h"
#endif

/*
#include "config.h"
#include "compat.h"
*/
#include "err.h"
#include "tgen.h"
#include "dlite.h"
#include "dlite-macros.h"


static int list_dimensions(TGenBuf *s, const char *template, int len,
                           const TGenSubs *subs, void *context)
{
  int retval = 1;
  DLiteInstance *inst = (DLiteInstance *)context;
  TGenSubs dsubs;
  size_t i;
  if (tgen_subs_copy(&dsubs, subs)) goto fail;
  for (i=0; i < inst->meta->ndimensions; i++) {
    tgen_subs_set(&dsubs, "dim_name", inst->meta->dimensions[i].name, NULL);
    tgen_subs_set(&dsubs, "dim_descr", inst->meta->dimensions[i].description, NULL);
    tgen_subs_set_fmt(&dsubs, "dim_value", NULL, "%lu", DLITE_DIM(inst, i));
    tgen_append(s, template, len, &dsubs, inst);
  }
  retval = 0;
 fail:
  tgen_subs_deinit(&dsubs);
  return retval;
}



/*
  Assign substitutions based on the instance `inst`.

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
  if (instance_subs(&subs, inst)) return NULL;
  if (option_subs(&subs, options)) return NULL;
  return tgen(template, &subs, (void *)inst);
}


void help()
{
  char **p, *msg[] = {
    "Usage: dlite-codegen [OPTIONS] [ID]",
    "Generates code for DLite instance.",
    "  -d, --driver=STRING          Driver for loading input instance.",
    "                               Default is \"json\".",
    "  -D, --driver-options=STRING  Options for loading input instance.",
    "                               Default is \"mode=r\".",
    "  -f, --format=STRING          Output format (that is emplate name)",
    "                               if -t is not given.",
    "  -h, --help                   Prints this help and exit.",
    "  -o, --output=PATH            Output file.  Default is stdout.",
    "  -O, --options=STRING         Options for updating substitutions.",
    "  -t, --template=PATH          Template file to load.",
    "  -u, --uri=URI                Input uri.",
    "",
    "Reads the instance (typically an entity) identified by ID from storage",
    "using the --driver, --uri and --options options.  ID may be omitted if",
    "the storage only contains one entity.",
    "",
    "If --uri is not provided, ID should refer to a built-in metadata.",
    "",
    "ID should either be an UUID or (more convinient) a namespace/version/name",
    "uri.",
    "",
    NULL
  };
  for (p=msg; *p; p++) printf("%s\n", *p);
}


int main(int argc, char *argv[])
{
  int retval = 1;
  DLiteInstance *inst;
  TGenBuf opts;
  char *text, *template;

  /* Command line arguments */
  char *driver = "json";
  char *driver_options = "mode=r";
  char *format = NULL;
  char *output = NULL;
  char *options = "";
  char *template_file = NULL;
  char *uri = NULL;
  char *id = NULL;

  err_set_prefix("dlite-codegen");

  tgen_buf_init(&opts);

  /* Parse options and arguments */
  while (1) {
    int longindex = 0;
    struct option longopts[] = {
      {"driver",         1, NULL, 'd'},
      {"driver-options", 1, NULL, 'D'},
      {"format",         1, NULL, 'f'},
      {"help",           0, NULL, 'h'},
      {"output",         1, NULL, 'o'},
      {"options",        1, NULL, 'O'},
      {"template",       1, NULL, 't'},
      {"uri",            1, NULL, 'u'},
      {NULL, 0, NULL, 0}
    };
    int c = getopt_long(argc, argv, "d:f:ho:O:u:", longopts, &longindex);
    if (c == -1) break;
    switch (c) {
    case 'd':  driver = optarg; break;
    case 'D':  driver_options = optarg; break;
    case 'f':  format = optarg; break;
    case 'h':  help(stdout); exit(0);
    case 'o':  output = optarg; break;
    case 'O':  options = optarg; break;
    case 't':  template_file = optarg; break;
    case 'u':  uri = optarg; break;
    case '?':  exit(1);
    default:   abort();
    }
  }
  if (optind < argc) id = argv[optind++];
  if (optind != argc)
    return errx(1, "Too many arguments");

  /* Load instance */
  if (uri) {
    DLiteStorage *s = dlite_storage_open(driver, uri, driver_options);
    if (!(inst = dlite_instance_load(s, id, NULL)))
      FAIL2("cannot read id '%s' from uri '%s'", id, uri);
    dlite_storage_close(s);
  } else if (id) {
    if (!(inst = (DLiteInstance *)dlite_metastore_get(id)))
      FAIL1("not a built-in id: %s", id);
  } else {
    FAIL("Either ID or --uri must be provided.\n"
         "Try: \"dlite-codegen --help\"");
  }

  /* Load template */
  template = tgen_readfile(template_file);

  /* Generate */
  text = dlite_codegen(template, inst, options);

  /* Write output */
  if (output) {
    FILE *fp = fopen(output, "wb");
    if (!fp) FAIL1("cannot open \"%s\" for writing", output);
    fwrite(text, 1, strlen(output), fp);
    fclose(fp);
  } else {
    fwrite(text, 1, strlen(text), stdout);
  }

  /* Cleanup */
  retval = 0;
 fail:
  dlite_instance_decref(inst);
  tgen_buf_deinit(&opts);
  if (template) free(template);
  if (text) free(text);
  return retval;
}
