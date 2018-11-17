/* dlite-codegen.c -- tool for generating code for a DLite instance */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "compat.h"

#include "err.h"
#include "tgen.h"
#include "dlite.h"
#include "dlite-macros.h"
#include "dlite-codegen.h"



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
  DLiteInstance *inst = NULL;
  TGenBuf opts;
  char *text=NULL, *template=NULL;

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
    int c = getopt_long(argc, argv, "d:D:f:ho:O:t:u:", longopts, &longindex);
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
    dlite_instance_incref(inst);
  } else {
    FAIL("Either ID or --uri must be provided.\n"
         "Try: \"dlite-codegen --help\"");
  }

  /* Load template */
  if (!(template = tgen_readfile(template_file))) goto fail;

  /* Generate */
  if (!(text = dlite_codegen(template, inst, options))) goto fail;

  /* Write output */
  if (output) {
    FILE *fp = fopen(output, "wb");
    if (!fp) FAIL1("cannot open \"%s\" for writing", output);
    fwrite(text, 1, strlen(text), fp);
    fclose(fp);
  } else {
    fwrite(text, 1, strlen(text), stdout);
  }

  /* Cleanup */
  retval = 0;
 fail:
  if (inst) dlite_instance_decref(inst);
  tgen_buf_deinit(&opts);
  if (template) free(template);
  if (text) free(text);
  return retval;
}
