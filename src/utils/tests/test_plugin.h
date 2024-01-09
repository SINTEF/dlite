#ifndef TEST_PLUGIN_H
#define TEST_PLUGIN_H

#include "dsl.h"
#include "plugin.h"

typedef int (*Fun1)(int a, int b);
typedef double (*Fun2)(double a);

typedef struct _TestAPI {
  PluginAPI_HEAD
  Fun1 fun1;
  Fun2 fun2;
} TestAPI;

DSL_EXPORT const TestAPI *get_testapi(int *iter);

#endif /* TEST_PLUGIN_H */
