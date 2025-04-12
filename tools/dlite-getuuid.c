/* dlite-getuuid.c -- simple tool for generating UUIDs */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "config.h"

#include "utils/compat-src/getopt.h"
#include "utils/err.h"
#include "dlite-misc.h"


void help()
{
  char **p, *msg[] = {
    "Usage: dlite-getuuid [-h] [STRING]",
    "Generates an UUID.",
    "  -h, --help          Prints this help and exit.",
    "  -n, --normalise-id  Return normalised ID instead of a UUID.",
    "  -u, --uri=URI       Used together with --normalise-id.",
    "                      A optional namespace to prepend to STRING, ",
    "                      if STRING is not a URI.",
    "",
    "If STRING is not given, a random (version 4) UUID is printed to stdout.",
    "",
    "If STRING is is a valid UUID (xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx),",
    "it is printed unmodified to stdout.",
    "",
    "Otherwise, STRING is converted to a version 5 UUID (using its SHA-1",
    "hash and the DNS namespace) and printed to stdout.",
    "",
    NULL
  };
  for (p=msg; *p; p++) printf("%s\n", *p);
}


int main(int argc, char *argv[])
{
  char *id=NULL, *uri=NULL, buf[512];
  int normalise_id = 0;

  /* Command line arguments */
  err_set_prefix("dlite-getuuid");

  /* Parse options and arguments */
  while (1) {
    int longindex = 0;
    struct option longopts[] = {
      {"help",             0, NULL, 'h'},
      {"normalised-id",    0, NULL, 'n'},
      {"uri",              1, NULL, 'u'},
      {NULL, 0, NULL, 0}
    };
    int c = getopt_long(argc, argv, "hnu:", longopts, &longindex);
    if (c == -1) break;
    switch (c) {
    case 'h':  help(); exit(0);
    case 'n':  normalise_id = 1; break;
    case 'u':  uri = optarg; break;
    case '?':  exit(1);
    default:   abort();
    }
  }
  if (optind < argc) id = argv[optind++];
  if (optind != argc)
    return errx(1, "Too many arguments. Try `dlite-getuuid --help`.");

  if (normalise_id)
    dlite_normalise_id(buf, sizeof(buf), id, uri);
  else
    dlite_get_uuid(buf, id);

  printf("%s\n", buf);
  return 0;
}
