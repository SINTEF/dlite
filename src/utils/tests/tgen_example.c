/* Example program demonstrating tgen */
#include <stdio.h>
#include "tgen.h"

#define STRINGIFY(s) _STRINGIFY(s)
#define _STRINGIFY(s) # s

#define UNUSED(x) (void)(x)

/* Person records */
typedef struct {
  char *first_name;
  char *last_name;
  char *country;
} Record;

/* Pointer to substitutions for loop variables.  We make it global
    such that it is accessible by deinit() */
static TGenSubs *loopsubs=NULL;

/* Called at program exit to free up allocated resources (valgrind-friendly) */
static void deinit()
{
  tgen_subs_deinit(loopsubs);
}

/* Expands {list_members} using the data catalogue passed via `context`. */
static int list_members(TGenBuf *s, const char *template, int len,
                        const TGenSubs *subs, void *context)
{
  int i;
  Record *catalogue = (Record *)context;

  UNUSED(subs);  /* to get rid of compiler warnings about unused argument... */

  /* Initialise substitution table for `list_members` */
  if (!loopsubs) {
    static TGenSubs subs2;
    loopsubs = &subs2;
    tgen_subs_init(loopsubs);
    atexit(deinit);
  };

  /* For each record, update local substitutions and call tgen_append() */
  for (i=0; catalogue[i].first_name; i++) {
    Record *r = catalogue + i;
    tgen_subs_set(loopsubs, "first_name", r->first_name, NULL);
    tgen_subs_set(loopsubs, "last_name", r->last_name, NULL);
    tgen_subs_set(loopsubs, "country", r->country, NULL);
    tgen_append(s, template, len, loopsubs, context);
  }
  return 0;
}


int main()
{
  Record catalogue[] = {
    {"Adam", "Smidth", "Germany"},
    {"Jack", "Daniel", "USA"},
    {"Fritjof", "Nansen", "Norway"},
    {NULL, NULL, NULL}
  };
  char *filename = STRINGIFY(TESTDIR) "/tgen_template.txt";
  char *template = tgen_readfile(filename);
  TGenSubs subs;

  /* Create substitutions */
  tgen_subs_init(&subs);
  tgen_subs_set(&subs, "group_name", "skiers", NULL);
  tgen_subs_set(&subs, "group_location", "mountains", NULL);
  tgen_subs_set(&subs, "list_members", NULL, list_members);

  /* Generate and display output */
  char *str = tgen(template, &subs, catalogue);
  printf("%s", str);

  /* Clean up */
  free(str);
  tgen_subs_deinit(&subs);
  return 0;
}
