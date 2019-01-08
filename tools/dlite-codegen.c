/* dlite-codegen.c -- tool for generating code for a DLite instance */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "config.h"

#include "utils/compat.h"
#include "utils/compat/getopt.h"
#include "utils/err.h"
#include "utils/tgen.h"
#include "dlite.h"
#include "dlite-macros.h"
#include "dlite-codegen.h"



void help()
{
  char **p, *msg[] = {
    "Usage: dlite-codegen [OPTIONS] [ID]",
    "Generates code from a template and a DLite instance.",
    "  -d, --driver=STRING          Driver for loading input instance.",
    "                               Default is \"json\".",
    "  -O, --driver-options=STRING  Options for loading input instance.",
    "                               Default is \"mode=r\".",
    "  -u, --driver-uri=URI         Input uri.",
    "  -f, --format=STRING          Output format (that is emplate name)",
    "                               if -t is not given.",
    "  -h, --help                   Prints this help and exit.",
    "  -n, --native-typenames       Whether to use native typenames.  The",
    "                               default is to use portable typenames.",
    "                               Ex. \"double\" instead of \"float64_t\".",
    "  -o, --output=PATH            Output file.  Default is stdout.",
    "  -v, --variables=STRING       Assignment of additional variable(s).",
    "                               STRING is a semicolon-separated string of",
    "                               VAR=VALUE pairs.  This option may be ",
    "                               provided more than once.",
    "  -t, --template=PATH          Template file to load.",
    "",
    "The template is either provided via the --template option, or (if",
    "--template is not given) read from stdin.",
    "",
    "The instance (typically an entity) identified by ID is read from a",
    "storate using the --driver, --driver-uri and --driver-options options.",
    "ID may be omitted if the storage only contains one entity.  If --uri is",
    "not provided, ID should refer to a built-in metadata.",
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
  size_t n;
  DLiteInstance *inst = NULL;
  char *text=NULL, *template=NULL;

  /* Command line arguments */
  char *driver = "json";
  char *driver_options = "mode=r";
  char *driver_uri = NULL;
  //char *format = NULL;
  char *output = NULL;
  char *template_file = NULL;
  TGenBuf variables;
  char *id = NULL;

  err_set_prefix("dlite-codegen");

  tgen_buf_init(&variables);

  /* Parse options and arguments */
  while (1) {
    int longindex = 0;
    struct option longopts[] = {
      {"driver",           1, NULL, 'd'},
      {"driver-options",   1, NULL, 'O'},
      {"driver-uri",       1, NULL, 'u'},
      {"format",           1, NULL, 'f'},
      {"help",             0, NULL, 'h'},
      {"native-typenames", 0, NULL, 'n'},
      {"output",           1, NULL, 'o'},
      {"template",         1, NULL, 't'},
      {"variables",        1, NULL, 'v'},
      {NULL, 0, NULL, 0}
    };
    int c = getopt_long(argc, argv, "d:O:u:f:hno:t:v:", longopts, &longindex);
    if (c == -1) break;
    switch (c) {
    case 'd':  driver = optarg; break;
    case 'O':  driver_options = optarg; break;
    case 'u':  driver_uri = optarg; break;
      //case 'f':  format = optarg; break;
    case 'h':  help(stdout); exit(0);
    case 'n':  dlite_codegen_use_native_typenames = 1; break;
    case 'o':  output = optarg; break;
    case 't':  template_file = optarg; break;
    case 'v':  tgen_buf_append_fmt(&variables, "%s;",optarg); break;
    case '?':  exit(1);
    default:   abort();
    }
  }
  if (optind < argc) id = argv[optind++];
  if (optind != argc)
    return errx(1, "Too many arguments");

  /* Remove trailing semicolon or ampersand from variables */
  if ((n = tgen_buf_length(&variables)) &&
      strchr(";&", tgen_buf_get(&variables)[n-1]))
    tgen_buf_unappend(&variables, 1);

  /* Load instance */
  if (driver_uri) {
    DLiteStorage *s = dlite_storage_open(driver, driver_uri, driver_options);
    if (!(inst = dlite_instance_load(s, id)))
      FAIL2("cannot read id '%s' from uri '%s'", id, driver_uri);
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
  if (!(text = dlite_codegen(template, inst, tgen_buf_get(&variables))))
    goto fail;

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
  tgen_buf_deinit(&variables);
  if (template) free(template);
  if (text) free(text);
  return retval;
}
