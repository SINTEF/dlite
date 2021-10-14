set(DLITE_VERSION 0.3.1)


####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was dliteConfig.cmake.in                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../" ABSOLUTE)

macro(set_and_check _var _file)
  set(${_var} "${_file}")
  if(NOT EXISTS "${_file}")
    message(FATAL_ERROR "File or directory ${_file} referenced by variable ${_var} does not exist !")
  endif()
endmacro()

macro(check_required_components _NAME)
  foreach(comp ${${_NAME}_FIND_COMPONENTS})
    if(NOT ${_NAME}_${comp}_FOUND)
      if(${_NAME}_FIND_REQUIRED_${comp})
        set(${_NAME}_FOUND FALSE)
      endif()
    endif()
  endforeach()
endmacro()

####################################################################################

#include(configVersion.cmake)
#include(configExport.cmake)

set(DLITE_ROOT "C:/Users/ps-adm/repo/proj/A419409_OntoTRANS/SINTEF/dlite/out/install/x64-Release-vs15")

set_and_check(DLITE_INCLUDE_DIRS  "C:/Users/ps-adm/repo/proj/A419409_OntoTRANS/SINTEF/dlite/out/install/x64-Release-vs15/include/dlite")
set_and_check(DLITE_LIBRARY_DIR   "C:/Users/ps-adm/repo/proj/A419409_OntoTRANS/SINTEF/dlite/out/install/x64-Release-vs15/lib")
set_and_check(DLITE_RUNTIME_DIR   "C:/Users/ps-adm/repo/proj/A419409_OntoTRANS/SINTEF/dlite/out/install/x64-Release-vs15/bin")

set_and_check(DLITE_TEMPLATE_DIRS "C:/Users/ps-adm/repo/proj/A419409_OntoTRANS/SINTEF/dlite/out/install/x64-Release-vs15/share/dlite/templates")
set_and_check(DLITE_STORAGE_PLUGIN_DIRS
  "C:/Users/ps-adm/repo/proj/A419409_OntoTRANS/SINTEF/dlite/out/install/x64-Release-vs15/share/dlite/storage-plugins")
set_and_check(DLITE_MAPPING_PLUGIN_DIRS
  "C:/Users/ps-adm/repo/proj/A419409_OntoTRANS/SINTEF/dlite/out/install/x64-Release-vs15/share/dlite/mapping-plugins")
set_and_check(DLITE_PYTHON_STORAGE_PLUGIN_DIRS
  "C:/Users/ps-adm/repo/proj/A419409_OntoTRANS/SINTEF/dlite/out/install/x64-Release-vs15/share/dlite/python-storage-plugins")
set_and_check(DLITE_PYTHON_MAPPING_PLUGIN_DIRS
  "C:/Users/ps-adm/repo/proj/A419409_OntoTRANS/SINTEF/dlite/out/install/x64-Release-vs15/share/dlite/python-mapping-plugins")
set_and_check(DLITE_STORAGES      "C:/Users/ps-adm/repo/proj/A419409_OntoTRANS/SINTEF/dlite/out/install/x64-Release-vs15/share/dlite/storages")

set(WITH_PYTHON    ON)
set(WITH_JSON      ON)
set(WITH_HDF5      OFF)

if(NOT DLITE_LIBRARIES)
  # Set LITE_LIBRARIES by progressively appending to it

  find_library(dlite_LIBRARY
    NAMES dlite
    PATHS ${DLITE_ROOT}/lib
    )
  list(APPEND dlite_LIBRARIES "${dlite_LIBRARY}")

  find_library(dlite_UTILS_LIBRARY
    NAMES dlite-utils
    PATHS ${DLITE_ROOT}/lib
    )
  list(APPEND dlite_LIBRARIES "${dlite_UTILS_LIBRARY}")

  # Python
  if(WITH_PYTHON)
    set_and_check(DLITE_PYTHON_STORAGE_PLUGIN_DIRS
      "C:/Users/ps-adm/repo/proj/A419409_OntoTRANS/SINTEF/dlite/out/install/x64-Release-vs15/share/dlite/python-storage-plugins")
    set_and_check(DLITE_PYTHON_MAPPING_PLUGIN_DIRS
      "C:/Users/ps-adm/repo/proj/A419409_OntoTRANS/SINTEF/dlite/out/install/x64-Release-vs15/share/dlite/python-mapping-plugins")
    set(Python3_LIBRARIES "C:/Users/ps-adm/anaconda3/libs/python38.lib" CACHE STRING
      "Python libraries")
    set(Python3_INCLUDE_DIRS "C:/Users/ps-adm/anaconda3/include" CACHE STRING
      "Python include  directories")
    list(APPEND dlite_LIBRARIES "${Python3_LIBRARIES}")
  endif()

  # HDF5
  if(WITH_HDF5)
    set(HDF5_LIBRARIES "" CACHE STRING "HDF5 libraries")
    list(APPEND dlite_LIBRARIES "${HDF5_LIBRARIES}")
  endif()

  # FIXME: also add redland

  set(DLITE_LIBRARIES "${dlite_LIBRARIES}" CACHE STRING "DLite libraries")
endif()



check_required_components(dlite)
