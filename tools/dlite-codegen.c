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

#include "err.h"
#include "dlite.h"
#include "dlite-macros.h"


/*
  Writes instance `inst` as JSON to file `filename`.
 */
static int write_json(const char *filename, DLiteInstance *inst)
{
  DLiteStorage *s;
  if (!(s = dlite_storage_open("json", filename, "mode=w;meta=true")))
    return errx(1, "cannot open storage: '%s'", filename);
  if (dlite_instance_save(s, inst))
    return errx(1, "cannot save instance to storage: '%s'", filename);
  if (dlite_storage_close(s))
    return errx(1, "cannot close storage: '%s'", filename);
  return 0;
}


/*
  Writes instance `inst` as C header to `fp`.
*/
static int write_c_header(FILE *fp, DLiteInstance *inst)
{
  int retval=1;
  char **q, *name=NULL, *version=NULL, *namespace=NULL, *NAME=NULL, **descr;
  DLiteMeta *meta = (DLiteMeta *)inst;
  int ismetameta = dlite_meta_is_metameta(meta);
  size_t i, j, len, maxlen;
  char buf[256];

  if (!dlite_meta_is_metameta(inst->meta))
    FAIL("writing data instances as C header is not supported");

  if (inst->uri) {
    dlite_split_meta_uri(inst->uri, &name, &version, &namespace);
  } else if ((q = dlite_instance_get_property(inst, "name")) &&
             (name = strdup(*q)) &&
             (q = dlite_instance_get_property(inst, "version")) &&
             (version = strdup(*q)) &&
             (q = dlite_instance_get_property(inst, "namespace")) &&
             (namespace = strdup(*q))) {
    inst->uri = dlite_join_meta_uri(name, version, namespace);
  } else {
    FAIL("cannot determine meta uri");
  }

  NAME = strdup(name);
  for (i=0; NAME[i]; i++) NAME[i] = toupper(NAME[i]);

  descr = dlite_instance_get_property(inst, "description");

  fprintf(fp, "/* This file is generated with dlite-codegen -- "
          "do not edit! */\n");
  fprintf(fp, "\n");
  if (descr && *descr) fprintf(fp, "/* %s */\n", *descr);
  fprintf(fp, "#ifndef _%s_H\n", NAME);
  fprintf(fp, "#define _%s_H\n", NAME);
  fprintf(fp, "\n");
  fprintf(fp, "#include \"boolean.h\"\n");
  fprintf(fp, "#include \"integers.h\"\n");
  fprintf(fp, "#include \"floats.h\"\n");
  fprintf(fp, "\n");
  fprintf(fp, "typedef struct _%s {\n", name);
  fprintf(fp, "  /* -- header */\n");
  fprintf(fp, "  char uuid[%d+1];   "
              "/*!< UUID for this data instance. */\n", DLITE_UUID_LENGTH);
  fprintf(fp, "  const char *uri;   "
              "/*!< Unique name or uri of the data instance.\n");
  fprintf(fp, "                          Can be NULL. */\n");
  fprintf(fp, "\n");
  fprintf(fp, "  size_t refcount;   "
              "/*!< Number of references to this instance. */\n");
  fprintf(fp, "\n");
  fprintf(fp, "  const void *meta;  "
              "/*!< Pointer to the metadata describing this instance. */\n");
  fprintf(fp, "\n");

  if (ismetameta) {
    fprintf(fp, "  size_t ndimensions;    "
                "/*!< Number of dimensions of instance. */\n");

    fprintf(fp, "  size_t nproperties;    "
                "/*!< Number of dimensions of properties. */\n");

    fprintf(fp, "  size_t nrelations;     "
                "/*!< Number of dimensions of relations. */\n");
    fprintf(fp, "\n");
    fprintf(fp, "  DLiteDimension *dimensions;  "
                "/*!< Array of dimensions. */\n");
    fprintf(fp, "  DLiteProperty *properties;   "
                "/*!< Array of properties. */\n");
    fprintf(fp, "  DLiteRelation *relations;    "
                "/*!< Array of relations. */\n");
    fprintf(fp, "\n");
    fprintf(fp, "  size_t headersize;   "
                "/*!< Size of instance header. */\n");
    fprintf(fp, "  DLiteInit init;      "
                "/*!< Function initialising an instance. */\n");
    fprintf(fp, "  DLiteDeInit deinit;  "
                "/*!< Function deinitialising an instance. */\n");
    fprintf(fp, "\n");
    fprintf(fp, "  size_t dimoffset;     "
                "/*!< Offset of first dimension value. */\n");
    fprintf(fp, "  size_t *propoffsets;  "
                "/*!< Pointer to array (in this metadata) of offsets\n"
                "                             to property values in "
                "instance. */\n");
    fprintf(fp, "  size_t reloffset;     "
                "/*!< Offset of first relation value. */\n");
    fprintf(fp, "  size_t pooffset;      "
                "/*!< Offset to array of property offsets. */\n");
    fprintf(fp, "\n");
  }

  fprintf(fp, "  /* -- dimension values */\n");
  for (i=maxlen=0; i < meta->ndimensions; i++)
    if ((len = strlen(meta->dimensions[i].name)) > maxlen) maxlen = len;
  for (i=0; i < meta->ndimensions; i++) {
    fprintf(fp, "  size_t %s;  ", meta->dimensions[i].name);
    for (j=0; j < maxlen - strlen(meta->dimensions[i].name); j++)
      fprintf(fp, " ");
    fprintf(fp, "/*!< %s */\n", meta->dimensions[i].description);
  }
  fprintf(fp, "\n");

  fprintf(fp, "  /* -- property values */\n");
  for (i=maxlen=0; i < meta->nproperties; i++) {
    DLiteProperty *p = meta->properties + i;
    size_t nref = (p->ndims) ? 1 : 0;
    dlite_type_set_cdecl(p->type, p->size, p->name, nref, buf, sizeof(buf));
    if ((len = strlen(buf)) > maxlen) maxlen = len;
  }
  for (i=0; i < meta->nproperties; i++) {
    DLiteProperty *p = meta->properties + i;
    size_t nref = (p->ndims) ? 1 : 0;
    int n;
    dlite_type_set_cdecl(p->type, p->size, p->name, nref, buf, sizeof(buf));
    n = fprintf(fp, "  %s;  ", buf);
    assert(0 <= n && n <= (int)maxlen + 5);
    for (j=0; j < maxlen + 5 - n; j++) fprintf(fp, " ");
    fprintf(fp, "/*!< %s ", p->description);
    if (p->unit) fprintf(fp, "(%s) ", p->unit);
    if (p->ndims) {
      char *sep = "";
      fprintf(fp, "[");
      for (j=0; j < (size_t)p->ndims; j++) {
        fprintf(fp, "%s%s", sep, meta->dimensions[p->dims[j]].name);
        sep = ", ";
      }
      fprintf(fp, "] ");
    }
    fprintf(fp, "*/\n");
  }

  if (meta->nrelations) {
    fprintf(fp, "\n");
    fprintf(fp, "  /* -- relation values*/\n");
    // FIXME - write relations
  }

  if (ismetameta) {
    fprintf(fp, "\n");
    fprintf(fp, "  /* -- instance property offsets*/\n");
    fprintf(fp, "  size_t offsets[%lu];  /*!< Property offsets. */\n",
            meta->nproperties);
  }
  fprintf(fp, "} %s;\n", name);
  fprintf(fp, "\n");
  fprintf(fp, "#endif /* _%s_H */\n", NAME);

  retval = 0;

 fail:
  if (NAME) free(NAME);
  if (name) free(name);
  if (version) free(version);
  if (namespace) free(namespace);
  return retval;
}


/*
   Writes instance `inst` to file `output` in the given format.
   If `output` is NULL or "-", the instance is written to stdout.
   Currently supported formats are "h" (C header) and "json".

   Returns non-zero on error.
*/
int dlite_codegen(const char *output, const char *format, DLiteInstance *inst)
{
  FILE *fp;
  int stat;

  if (!output || strcmp(output, "-") == 0)
    fp = stdout;
  else if (!(fp = fopen(output, "w")))
    return err(1, "cannot open output file: '%s'", output);

  if (strcmp(format, "json") == 0) {  /* -- json */
    if (fp == stdout)
      return errx(1, "json output to stdout is not supported");
    if ((stat = write_json(output, inst))) return stat;
  } else if (strcmp(format, "h") == 0) {  /* -- h (C header) */
    if ((stat = write_c_header(fp, inst))) return stat;
  } else {
    return errx(1, "not a supported output format: %s", format);
  }

  if (fp != stdout) fclose(fp);
  return 0;
}

char *header[] = {
  "/* This is a generated with DLite codegen -- do not edit! */",
  "",
  "/* Philib interpolator */",
  "#ifndef _INTERPOLATOR_H",
  "#define _INTERPOLATOR_H",
  "",
  "#include \"integers.h\"",
  "#include \"boolean.h\"",
  "",
  "",
  "typedef struct _Interpolator {",
  NULL
};
char *footer[] = {
  "} Interpolator;",
  "",
  "#endif /* _INTERPOLATOR_H */",
  NULL
};



void help()
{
  char **p, *msg[] = {
    "Usage: dlite-codegen [OPTIONS] [ID]",
    "Generates code for DLite instance.",
    "  -d, --driver=STRING   Input driver.  Defaults to \"json\".",
    "  -f, --format=STRING   Output format.  Currently supported formats: ",
    "                        \"h\" (C header, default), \"json\"",
    "  -h, --help            Prints this help and exit.",
    "  -o, --output=PATH     Output file.  Default is to write to stdout.",
    "  -O, --options=STRING  Input options.",
    "  -u, --uri=URI         Input uri.",
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
  DLiteInstance * inst;

  /* Command line arguments */
  char *driver = "json";
  char *format = "h";
  char *output = NULL;
  char *options = "mode=r";
  char *uri = NULL;
  char *id = NULL;

  err_set_prefix("dlite-codegen");

  while (1) {
    int longindex = 0;
    struct option longopts[] = {
        {"driver",  1, NULL, 'd'},
        {"format",  1, NULL, 'f'},
        {"help",    0, NULL, 'h'},
        {"output",  1, NULL, 'o'},
        {"options", 1, NULL, 'O'},
        {"uri",     1, NULL, 'u'},
        {NULL, 0, NULL, 0}
    };
    int c = getopt_long(argc, argv, "d:f:ho:O:u:", longopts, &longindex);
    if (c == -1) break;
    switch (c) {
    case 'd':  driver = optarg; break;
    case 'f':  format = optarg; break;
    case 'h':  help(stdout); exit(0);
    case 'o':  output = optarg; break;
    case 'O':  options = optarg; break;
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
    DLiteStorage *s = dlite_storage_open(driver, uri, options);
    if (!(inst = dlite_instance_load(s, id, NULL)))
      return errx(1, "cannot read id '%s' from uri '%s'", id, uri);
    dlite_storage_close(s);
  } else if (id) {
    if (!(inst = (DLiteInstance *)dlite_metastore_get(id)))
      return errx(1, "not a built-in id: %s", id);
  } else {
    return errx(1, "Either ID or --uri must be provided.\n"
                "Try: \"dlite-codegen --help\"");
  }

  /* Generate */
  dlite_codegen(output, format, inst);

  /* Cleanup */
  dlite_instance_decref(inst);
  return 0;
}
