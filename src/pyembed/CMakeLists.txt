# -*- Mode: cmake -*-


set(pyembed_sources
  utils/compat-src/setenv.c
  pyembed/dlite-pyembed.c
  pyembed/dlite-pyembed-utils.c
  pyembed/dlite-python-singletons.c
  pyembed/dlite-python-storage.c
  pyembed/dlite-python-mapping.c
  pyembed/dlite-python-protocol.c
  pyembed/dlite-python-path.c
  PARENT_SCOPE
)
