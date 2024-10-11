# -*- Mode: cmake -*-

include(CheckCSourceCompiles)

# Check if clock_settime (used by Python) need linking with -lrt
set(CMAKE_REQUIRED_LINK_OPTIONS_save ${CMAKE_REQUIRED_LINK_OPTIONS})
set(CMAKE_REQUIRED_LINK_OPTIONS "-lrt")
check_c_source_compiles("
#define _XOPEN_SOURCE 600
#include <time.h>
int main(void) {
  clockid_t clock = CLOCK_REALTIME;
  struct timespec ts;
  clock_gettime(clock, &ts);
  return 0;
}" need_lrt)

if(need_lrt)
  list(APPEND extra_link_libraries "-lrt")
endif()

set(CMAKE_REQUIRED_LINK_OPTIONS ${CMAKE_REQUIRED_LINK_OPTIONS_save})
unset(CMAKE_REQUIRED_LINK_OPTIONS_save)
unset(need_lrt)


set(pyembed_sources
  pyembed/dlite-pyembed.c
  pyembed/dlite-pyembed-utils.c
  pyembed/dlite-python-singletons.c
  pyembed/dlite-python-storage.c
  pyembed/dlite-python-mapping.c
  pyembed/dlite-python-protocol.c
  PARENT_SCOPE
  )
