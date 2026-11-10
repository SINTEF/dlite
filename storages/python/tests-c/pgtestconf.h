/* pgtestconf.h -- configuration of the PostgreSQL server used by the tests
 *
 * The defaults agree with the postgres service in
 * examples/docker-compose.yml and with TEST_SERVICES in
 * bindings/python/testutils.py.  They can be overridden with the
 * environment variables:
 *
 *   DLITE_TEST_POSTGRESQL_HOST
 *   DLITE_TEST_POSTGRESQL_PORT
 *   DLITE_TEST_POSTGRESQL_USER
 *   DLITE_TEST_POSTGRESQL_PASSWORD
 *   DLITE_TEST_POSTGRESQL_DATABASE
 */
#ifndef _PGTESTCONF_H
#define _PGTESTCONF_H

#include <stdio.h>
#include <stdlib.h>

/* Location ("host:port") and options of the PostgreSQL test server.
   Initialised by pgtestconf_init(). */
static char pg_location[256];
static char pg_options[512];

/* Returns the value of environment variable `name` or `default_value`
   if it is not set. */
static const char *pgtestconf_getenv(const char *name,
                                     const char *default_value)
{
  const char *value = getenv(name);
  return (value && *value) ? value : default_value;
}

/* Initialise `pg_location` and `pg_options`. */
static void pgtestconf_init(void)
{
  snprintf(pg_location, sizeof(pg_location), "%s:%s",
           pgtestconf_getenv("DLITE_TEST_POSTGRESQL_HOST", "localhost"),
           pgtestconf_getenv("DLITE_TEST_POSTGRESQL_PORT", "5432"));
  snprintf(pg_options, sizeof(pg_options),
           "database=%s;user=%s;password=%s",
           pgtestconf_getenv("DLITE_TEST_POSTGRESQL_DATABASE", "dlite"),
           pgtestconf_getenv("DLITE_TEST_POSTGRESQL_USER", "dlite"),
           pgtestconf_getenv("DLITE_TEST_POSTGRESQL_PASSWORD", "dlite-test"));
}

#endif /* _PGTESTCONF_H */
