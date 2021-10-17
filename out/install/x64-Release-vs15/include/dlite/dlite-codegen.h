#ifndef _DLITE_CODEGEN_H
#define _DLITE_CODEGEN_H

/**
  @file
  @brief Code generation

  The dlite_codegen() function takes a template and an instance (and
  possible some options) as input and returns a new text string that
  typically is code that will be written to file and compiled.
*/

#include "utils/fileutils.h"


/**
  Assign/update substitutions based on the instance `inst`.

  Returns non-zero on error.
*/
int dlite_instance_subs(TGenSubs *subs, const DLiteInstance *inst);


/**
  Assign/update substitutions based on `options`.

  Returns non-zero on error.
*/
int dlite_option_subs(TGenSubs *subs, const char *options);


/**
  Returns a pointer to current template path.
*/
FUPaths *dlite_codegen_path_get(void);

/**
  Free up memory in template paths.
*/
void dlite_codegen_path_free(void);


/**
  Returns whether to use native typenames
*/
int dlite_codegen_get_native_typenames(void);

/**
  Sets whether to use native typenames. If zero, use portable type names.
*/
  void dlite_codegen_set_native_typenames(int use_native_typenames);


/**
  Returns a newly malloc'ed string with a generated document based on
  `template` and instanse `inst`.  `options` is a semicolon (;) separated
  string with additional options.

  Returns NULL on error.
 */
char *dlite_codegen(const char *template, const DLiteInstance *inst,
                    const char *options);


/**
  Returns a pointer to malloc'ed template file name, given a template
  name (e.g. "c-header", "c-source", "c-ext_header", ...).

  Returns NULL on error.
 */
char *dlite_codegen_template_file(const char *template_name);

#endif /* _DLITE_CODEGEN_H */
