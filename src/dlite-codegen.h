#ifndef _DLITE_CODEGEN_H
#define _DLITE_CODEGEN_H

/**
  @file
  @brief Code generation

  The dlite_codegen() function takes a template and an instance (and
  possible some options) as input and returns a new text string that
  typically is code that will be written to file and compiled.
*/


/**
  Global variable indicating whether native typenames should be used.
  The default is to use portable type names.
*/
extern int dlite_codegen_use_native_typenames;

/**
  Assign/update substitutions based on the instance `inst`.

  Returns non-zero on error.
*/
int instance_subs(TGenSubs *subs, const DLiteInstance *inst);


/**
  Assign/update substitutions based on `options`.

  Returns non-zero on error.
*/
int option_subs(TGenSubs *subs, const char *options);


/*
  Returns a newly malloc'ed string with a generated document based on
  `template` and instanse `inst`.  `options` is a semicolon (;) separated
  string with additional options.

  Returns NULL on error.
 */
char *dlite_codegen(const char *template, const DLiteInstance *inst,
                    const char *options);


#endif /* _DLITE_CODEGEN_H */
