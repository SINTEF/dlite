/* dlite-codegen.c -- tool for generating code for a DLite instance */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "config.h"

#include "utils/compat.h"
#include "utils/compat-src/getopt.h"
#include "utils/err.h"
#include "utils/fileutils.h"
#include "utils/tgen.h"
#include "dlite.h"
#include "dlite-storage-plugins.h"
#include "dlite-macros.h"
#include "dlite-codegen.h"


void help()
{
  char **p, *msg[] = {
    "Usage: dlite-codegen [OPTIONS] URL",
    "Generates code from a template and a DLite instance.",
    "  -b, --built-in               Whether the URL refers to a built-in",
    "                               instance, rather than an instance located",
    "                               in a storage.",
    "  -B, --build-root             Whether to look for storage plugins in ",
    "                               the build root directory rather than ",
    "                               under DLITE_ROOT.  Intended for testing.",
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
    "  -m, --metadata=URL           Additional metadata to load.  May be ",
    "                               provided multiple times.",
    "  -t, --template-file=PATH     Template file to load.",
    "  -v, --variables=STRING       Assignment of additional variable(s).",
    "                               STRING is a semicolon-separated string of",
    "                               VAR=VALUE pairs.  This option may be ",
    "                               provided more than once.",
    "  -V, --version                Print dlite version number and exit.",
    "",
    "The template is either specified with the --format or --template-file "
    "options.",
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
  char *text=NULL, *template=NULL, *template_path=NULL;

  /* Command line arguments */
  char *url = NULL;
  char *format = "c-header";
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
      {"build-root",       0, NULL, 'B'},
      {"format",           1, NULL, 'f'},
      {"help",             0, NULL, 'h'},
      {"native-typenames", 0, NULL, 'n'},
      {"output",           1, NULL, 'o'},
      {"storage-plugins",  1, NULL, 's'},
      {"metadata",         1, NULL, 'm'},
      {"template-file",    1, NULL, 't'},
      {"variables",        1, NULL, 'v'},
      {"version",          0, NULL, 'V'},
      {NULL, 0, NULL, 0}
    };
    int c = getopt_long(argc, argv, "bBf:hno:s:m:t:v:V", longopts, &longindex);
    if (c == -1) break;
    switch (c) {
    case 'b':  builtin = 1; break;
    case 'B':  dlite_set_use_build_root(1); break;
    case 'f':  format = optarg; break;
    case 'h':  help(); exit(0);
    case 'n':  dlite_codegen_set_native_typenames(1); break;
    case 'o':  output = optarg; break;
    case 's':  dlite_storage_plugin_path_append(optarg); break;
    case 'm':  dlite_instance_load_url(optarg); break;
    case 't':  template_file = optarg; break;
    case 'v':  tgen_buf_append_fmt(&variables, "%s;", optarg); break;
    case 'V':  printf("%s\n", dlite_VERSION); exit(0);
    case '?':  exit(1);
    default:   abort();
    }
  }
  if (optind < argc) url = argv[optind++];
  if (optind != argc)
    return errx(1, "Too many arguments");

  if (!url) return errx(1, "Missing url argument");

  /* Remove trailing semicolon or ampersand from variables */
  if ((n = tgen_buf_length(&variables)) &&
      strchr(";&", tgen_buf_get(&variables)[n-1]))
    tgen_buf_unappend(&variables, 1);

  /* Load instance */
  if (builtin) {
    /* FIXME - this should be updated when default paths for entity lookup
       has been implemented... */
    if (!(inst = dlite_instance_get(url))) goto fail;
    dlite_instance_incref(inst);
  } else {
    if (!(inst = dlite_instance_load_url(url))) goto fail;
  }

  /* Get template file name */
  if (!template_file) {
    if (!(template_path = dlite_codegen_template_file(format))) goto fail;
    template_file = template_path;
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
  if (template_path) free(template_path);
  if (template) free(template);
  if (text) free(text);
  return retval;
}
