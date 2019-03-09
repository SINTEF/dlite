#ifndef _DLITE_TRANSLATOR_H
#define _DLITE_TRANSLATOR_H

/**
  @file
  @brief Translate instances of one metadata to another

  Translators are used to translate an instance of one entity to an
  instance of another entity.

  Translators can be registered as plugins and invoked transparently
  by DLite.  For this to work, DLite search all paths listed in the
  DLITE_TRANSLATOR_PLUGINS environment variable for plugins.
 */

typedef struct {
  char *uuid;
  char *meta_uri;
} ;



#endif /* _DLITE_TRANSLATOR_H */
