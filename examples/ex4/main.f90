#include <string.h>

#include "dlite.h"
#include "chemistry.h"

#define STRINGIFY(s) _STRINGIFY(s)
#define _STRINGIFY(s) # s

dlite_storage_open
dlite_instance_load
dlite_storage_close
dlite_instance_save
dlite_instance_decref


program main

  Pointer s !! DLiteStorage *s;
  Pointer p !! Chemistry *p;

  /* Load a json file containing the instance of an entity */
  s = dlite_storage_open("json", "example-6xxx.json", "mode=r");
  p = dlite_instance_load(s,id);
  call dlite_storage_close(s);
  
  
  /* Store the modified data in a new file */
  s = dlite_storage_open("json", "output.json", "mode=w");
  call dlite_instance_save(s, (DLiteInstance *)p);
  call dlite_storage_close(s);
  
  /* Free instance and its entity */
  call dlite_instance_decref((DLiteInstance *)p);

stop
end