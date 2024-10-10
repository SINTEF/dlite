#ifndef _DLITE_PYTHON_PROTOCOL_H
#define _DLITE_PYTHON_PROTOCOL_H

/**
  @file
  @brief Python protocol
*/

#ifdef HAVE_CONFIG_H
#undef HAVE_CONFIG_H
#endif


/**
  Returns a pointer to Python protocol paths
*/
FUPaths *dlite_python_protocol_paths(void);


/**
  Clears Python protocol search path.
*/
void dlite_python_protocol_paths_clear(void);

#endif  /* _DLITE_PYTHON_PROTOCOL_H */
