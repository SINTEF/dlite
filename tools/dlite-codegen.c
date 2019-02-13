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
#include "utils/fileutils.h"
#include "dlite.h"
#include "dlite-macros.h"
#include "dlite-codegen.h"


void help()
{
  char **p, *msg[] = {
    "Usage: dlite-codegen [OPTIONS] URL",
    "Generates code from a template and a DLite instance.",
    "  -b, --built-in               Whether the URL refers to an built-in",
    "                               instance, rather than an instance located",
    "                               in a storage.",
    "  -f, --format=STRING          Output format if -t is not given.",
    "                               It should correspond to a template name.",
    "                               Defaults to \"c-header\"",
    "  -h, --help                   Prints this help and exit.",
    "  -n, --native-typenames       Whether to use native typenames.  The",
    "                               default is to use portable typenames.",
    "                               Ex. \"double\" instead of \"float64_t\".",
    "  -o, --output=PATH            Output file.  Default is stdout.",
    "  -s, --storage-plugins=PATH   Additional paths to look for storage ",
    "                               plugins.  May be provided multiple times.",
    "  -t, --template=PATH          Template file to load.",
    "  -v, --variables=STRING       Assignment of additional variable(s).",
    "                               STRING is a semicolon-separated string of",
    "                               VAR=VALUE pairs.  This option may be ",
    "                               provided more than once.",
    "",
    "The template is either provided via the --template option, or (if",
    "--template is not given) read from stdin.",
    "",
    "The URL identifies the instance and should be of the general form:",
    "",
    "    driver://loc?options#id",
    "",
    "Parts:",
    "  - `driver` is the driver used for loading the instance (default: json).",
    "  - `loc` is the file or network path. If omitted, `id` should refer ",
    "    to a built-in metadata.",
    "  - `options` is a set of semicolon-separated options of the form ",
    "    KEY=VAL.  Defaults to \"mode=r\"",
    "  - `id` identifies the instance.  It should either be an UUID or ",
    "    (more convinient) a namespace/version/name uri.  It may be omitted",
    "    the storage only contains one entry."
    "",
    "The DLITE_TEMPLATE environment variable will be searched for additional",
    "templates.",
    "",
    NULL
  };
  for (p=msg; *p; p++) printf("%s\n", *p);
}


int main(int argc, char *argv[])
{
  int retval = 1;
  size_t n;
  int builtin = 0;
  DLiteInstance *inst = NULL;
  char *text=NULL, *template=NULL;

  /* Command line arguments */
  char *url = NULL;
  char *format = NULL;
  char *output = NULL;
  const char *template_file = NULL;
  TGenBuf variables;

  err_set_prefix("dlite-codegen");

  tgen_buf_init(&variables);

  /* Parse options and arguments */
  while (1) {
    int longindex = 0;
    struct option longopts[] = {
      {"built-in",         0, NULL, 'b'},
      {"format",           1, NULL, 'f'},
      {"help",             0, NULL, 'h'},
      {"native-typenames", 0, NULL, 'n'},
      {"output",           1, NULL, 'o'},
      {"storage-plugins",  1, NULL, 's'},
      {"template",         1, NULL, 't'},
      {"variables",        1, NULL, 'v'},
      {NULL, 0, NULL, 0}
    };
    int c = getopt_long(argc, argv, "bf:hno:s:t:v:", longopts, &longindex);
    if (c == -1) break;
    switch (c) {
    case 'b':  builtin = 1; break;
    case 'f':  format = optarg; break;
    case 'h':  help(stdout); exit(0);
    case 'n':  dlite_codegen_use_native_typenames = 1; break;
    case 'o':  output = optarg; break;
    case 's':  dlite_storage_plugin_path_append(optarg); break;
    case 't':  template_file = optarg; break;
    case 'v':  tgen_buf_append_fmt(&variables, "%s;",optarg); break;
    case '?':  exit(1);
    default:   abort();
    }
  }
  if (optind < argc) url = argv[optind++];
  if (optind != argc)
    return errx(1, "Too many arguments");

  /* Remove trailing semicolon or ampersand from variables */
  if ((n = tgen_buf_length(&variables)) &&
      strchr(";&", tgen_buf_get(&variables)[n-1]))
    tgen_buf_unappend(&variables, 1);

  /* Load instance */
  if (builtin) {
    /* FIXME - this should be updated when default paths for entity lookup
       has been implemented... */
    if (!(inst = (DLiteInstance *)dlite_metastore_get(url))) goto fail;
    dlite_instance_incref(inst);
  } else {
    if (!(inst = dlite_instance_load_url(url))) goto fail;
  }

  /* Load template */
  if (template_file) {
    if (!(template = tgen_readfile(template_file))) goto fail;
  } else if (format) {
    FUPaths paths;
    char *pattern=NULL;
    FUIter *iter=NULL;
    if (fu_paths_init(&paths, "DLITE_TEMPLATES") >= 0 &&
        fu_paths_append(&paths, DLITE_TEMPLATES_PATH) >= 0 &&
        asprintf(&pattern, "%s.txt", format) > 0 &&
        (iter = fu_startmatch(pattern, &paths)) &&
        (template_file = fu_nextmatch(iter)))
      template = tgen_readfile(template_file);
    fu_paths_deinit(&paths);
    if (pattern) free(pattern);
    if (iter) fu_endmatch(iter);
    if (!template) goto fail;
  } else {
    FAIL("either --template or --format must be given");
  }

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
