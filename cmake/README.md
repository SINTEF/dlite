Due to problems with FindSWIG in cmake 3.13 and earlier, we have made
a local copy of this module and the modules it depends on from cmake
3.14. This includes:

  - FindSWIG314.cmake
  - UseSWIG314.cmake
  - FindPackageHandleStandardArgs314.cmake
  - FindPackageMessage314.cmake

All these can be removed when we are ready to require cmake 3.14 or later.
