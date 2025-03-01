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
    "  -h, --help     Prints this help and exit.",
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
  char buff[37];

  if (argc > 1 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
    help();
    return 0;
  }
  if (argc <= 1)
    dlite_get_uuid(buff, NULL);
  else if (argc == 2)
    dlite_get_uuid(buff, argv[1]);
  else
    return err(1, "dgetuuid: too many arguments.  Try `dgetuuid -h`.");

  printf("%s\n", buff);
  return 0;
}
