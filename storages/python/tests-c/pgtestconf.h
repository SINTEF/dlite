/* pgtestconf.h -- configuration of the PostgreSQL server used by the tests
 *
 * The defaults agree with the postgres service in
 * test-services/compose.yaml and with TEST_SERVICES in
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

#include "pyembed/dlite-pyembed.h"

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

/* Returns non-zero if a TCP connection can be made to the PostgreSQL
   test server within `timeout` seconds.  Otherwise a message is written
   to stderr and zero is returned.

   Like serverskip() in bindings/python/testutils.py, this is used to
   skip the tests when no server is running, instead of letting them
   fail after a long connection timeout. */
static int pgtestconf_server_available(double timeout)
{
  int available = 0;
  const char *host = pgtestconf_getenv("DLITE_TEST_POSTGRESQL_HOST",
                                       "localhost");
  int port = atoi(pgtestconf_getenv("DLITE_TEST_POSTGRESQL_PORT", "5432"));
  PyObject *socket=NULL, *addr=NULL, *conn=NULL, *ret=NULL;
  PyObject *type, *value, *tb;

  dlite_pyembed_initialise();
  PyErr_Fetch(&type, &value, &tb);
  if ((socket = PyImport_ImportModule("socket")) &&
      (addr = Py_BuildValue("(si)", host, port)) &&
      (conn = PyObject_CallMethod(socket, "create_connection", "(Od)",
                                  addr, timeout))) {
    available = 1;
    ret = PyObject_CallMethod(conn, "close", NULL);
  }
  PyErr_Clear();
  PyErr_Restore(type, value, tb);
  Py_XDECREF(ret);
  Py_XDECREF(conn);
  Py_XDECREF(addr);
  Py_XDECREF(socket);

  if (!available)
    fprintf(stderr, "PostgreSQL server %s:%d is not reachable: "
            "skipping test\n", host, port);
  return available;
}

#endif /* _PGTESTCONF_H */
