project(dlite-bindings-python C)

# Python sources
set(py_sources
  factory.py
  options.py
  utils.py
  mappings.py
  protocol.py
  datamodel.py
  rdf.py
  dataset.py
  quantity.py
  testutils.py
  )

# Python sub-packages
set(py_packages
  #triplestore
)

configure_file(paths.py.in paths.py)
if(dlite_PYTHON_BUILD_REDISTRIBUTABLE_PACKAGE)
  configure_file(__init__standalone.py.in __init__.py)
else()
  configure_file(__init__inbuild.py.in __init__.py)
endif()

# Determine size of size_t
include(CheckTypeSize)
check_type_size(size_t SIZEOF_SIZE_T)
if(${SIZEOF_SIZE_T} EQUAL 4)
  set(NPY_SIZE_T NPY_UINT32)
elseif(${SIZEOF_SIZE_T} EQUAL 8)
  set(NPY_SIZE_T NPY_UINT64)
else()
  message(FATAL "Unexpected size of size_t: ${SIZEOF_SIZE_T}")
endif()


#
# Create Python library
# ---------------------
find_package(SWIG)
include(UseSWIG)
set(swig_flags "-Wall;-keyword;-DNPY_SIZE_T=${NPY_SIZE_T}")
if(${SWIG_VERSION} VERSION_LESS 4.1)
  list(APPEND swig_flags "-py3")
endif()

set(CMAKE_SWIG_OUTDIR ${CMAKE_CURRENT_BINARY_DIR})

include_directories(
  ${Python3_INCLUDE_DIRS}
  ${Python3_NumPy_INCLUDE_DIRS}
  ${dlite-src_SOURCE_DIR}
  ${dlite-src_SOURCE_DIR}/pyembed
  ${dlite-src_BINARY_DIR}
  ${dlite-src-utils_BINARY_DIR}
  )

set_source_files_properties(dlite.i
  PROPERTIES
    SWIG_FLAGS "${swig_flags}"
    SWIG_MODULE_NAME "dlite"
    )

#SET(CMAKE_SKIP_BUILD_RPATH TRUE)
swig_add_library(dlitepy
  TYPE SHARED
  LANGUAGE python
  OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/dlite
  OUTFILE_DIR ${CMAKE_CURRENT_BINARY_DIR}
  SOURCES dlite.i
  )


# Convince _dlite.so that it may search for whatever libraries it depends on in its own directory
set_target_properties(dlitepy PROPERTIES
  BUILD_RPATH "$ORIGIN/"
  INSTALL_RPATH "$ORIGIN/"
  OUTPUT_NAME "dlite"
  )


# Python only looks for .so for compiled extension modules on all Unix platforms including MacOS
# which would usually use .dylib thus change for just the extension module.
if(APPLE)
  set_target_properties(dlitepy PROPERTIES
    SUFFIX ".so"
  )
endif()

swig_link_libraries(dlitepy
  dlite
  dlite-utils
  #${Python3_LIBRARIES}
  )


##
## Create a working Python package in binary dir
## ---------------------------------------------
set(pkgdir ${CMAKE_CURRENT_BINARY_DIR}/dlite)
set(abs_sources)
foreach(source ${py_sources})
  list(APPEND abs_sources ${CMAKE_CURRENT_SOURCE_DIR}/${source})
endforeach()

list(APPEND abs_sources ${CMAKE_CURRENT_BINARY_DIR}/paths.py)
list(APPEND abs_sources ${CMAKE_CURRENT_BINARY_DIR}/__init__.py)

set(abs_targets)
foreach(source ${py_sources})
  list(APPEND abs_targets ${pkgdir}/${source})
endforeach()

set(package_targets)
foreach(package ${py_packages})
  list(APPEND package_targets ${pkgdir}/${package})
endforeach()


# On windows, also copy all dlite DLLs to our Python package
set(dlls)
if(WINDOWS OR MINGW)
  foreach(dll_target ${dll_targets})
    list(APPEND dlls $<TARGET_FILE:${dll_target}>)
  endforeach()
endif()

# Don't complain about unused parameters in SWIG-generated source
include(CheckCCompilerFlag)
check_c_compiler_flag("-Wno-unused-parameter" have_flag)
if(have_flag)
  set_property(TARGET dlitepy PROPERTY COMPILE_OPTIONS "-Wno-unused-parameter")
endif()

add_custom_command(
  TARGET dlitepy
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory ${pkgdir}
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:dlitepy>
    ${dlls}
    ${pkgdir}
  BYPRODUCTS
    ${pkgdir}
    ${pkgdir}/dlite.py
  )

add_custom_target(python_package ALL DEPENDS ${abs_targets} ${package_targets})
add_custom_command(
  OUTPUT
    ${abs_targets}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${pkgdir}
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${abs_sources}
    ${pkgdir}
  DEPENDS
    ${abs_sources}
    dlite-plugins-json
  )

foreach(package ${py_packages})
  file(
    GLOB_RECURSE pyfiles
    CONFIGURE_DEPENDS
    "${package}/**.py"
    )
  add_custom_command(
    OUTPUT
      ${pkgdir}/${package}
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/${package}
    ${pkgdir}/${package}
  DEPENDS
    ${pkgdir}
    ${CMAKE_CURRENT_SOURCE_DIR}/${package}
    ${pyfiles}
  COMMENT
    "Copying Python sub-packages: ${package}"
  )
endforeach()

#
# Add the extension-libraries (*.dll, *.so) to the package directory
# effectively producing a binary distribution ready to be run.
# This can be picked up by a setup.py script with ``python setup.py install``
# (or bdist_wheel)
#
add_custom_command(
  TARGET dlitepy
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory ${pkgdir}/share/dlite/storage-plugins
  COMMAND ${CMAKE_COMMAND} -E make_directory ${pkgdir}/share/dlite/mapping-plugins
  COMMAND ${CMAKE_COMMAND} -E make_directory ${pkgdir}/share/dlite/python-storage-plugins
  COMMAND ${CMAKE_COMMAND} -E make_directory ${pkgdir}/share/dlite/python-mapping-plugins
  COMMAND ${CMAKE_COMMAND} -E make_directory ${pkgdir}/share/dlite/python-protocol-plugins
  COMMAND ${CMAKE_COMMAND} -E make_directory ${pkgdir}/share/dlite/storages
  COMMAND ${CMAKE_COMMAND} -E make_directory ${pkgdir}/share/dlite/bin
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "$<TARGET_FILE:dlite>"
    ${pkgdir}
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "$<TARGET_FILE:dlite-utils>"
    ${pkgdir}
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "$<TARGET_FILE:dlite-plugins-python>"
    ${pkgdir}/share/dlite/storage-plugins
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "$<TARGET_FILE:dlite-plugins-json>"
    ${pkgdir}/share/dlite/storage-plugins
  COMMAND ${CMAKE_COMMAND}
    -DSOURCE_DIR=${dlite_SOURCE_DIR}/storages/python/python-storage-plugins
    -DDEST_DIR=${pkgdir}/share/dlite/python-storage-plugins
    -DPATTERN="*.py"
    -P ${dlite_SOURCE_DIR}/cmake/CopyDirectory.cmake
  COMMAND ${CMAKE_COMMAND}
    -DSOURCE_DIR=${dlite_SOURCE_DIR}/bindings/python/python-mapping-plugins
    -DDEST_DIR=${pkgdir}/share/dlite/python-mapping-plugins
    -DPATTERN="*.py"
    -P ${dlite_SOURCE_DIR}/cmake/CopyDirectory.cmake
  COMMAND ${CMAKE_COMMAND}
    -DSOURCE_DIR=${dlite_SOURCE_DIR}/bindings/python/python-protocol-plugins
    -DDEST_DIR=${pkgdir}/share/dlite/python-protocol-plugins
    -DPATTERN="*.py"
    -P ${dlite_SOURCE_DIR}/cmake/CopyDirectory.cmake
  COMMAND ${CMAKE_COMMAND}
    -DSOURCE_DIR=${dlite_SOURCE_DIR}/storages/python/python-storage-plugins
    -DDEST_DIR=${pkgdir}/share/dlite/storages
    -DPATTERN="*.json"
    -P ${dlite_SOURCE_DIR}/cmake/CopyDirectory.cmake
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${dlite_SOURCE_DIR}/README.md ${dlite_SOURCE_DIR}/LICENSE
    ${pkgdir}/share/dlite
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "$<TARGET_FILE:dlite-codegen>"
    "$<TARGET_FILE:dlite-env>"
    "$<TARGET_FILE:dlite-getuuid>"
    ${pkgdir}/share/dlite/bin
)
add_dependencies(
  dlitepy
  python_package
  dlite-codegen
  dlite-env
  dlite-getuuid
)


#
# Install
# -------
# Based on: http://developer.intra2net.com/git/?p=libftdi;a=blob;f=python/CMakeLists.txt;h=93410544fac2a45eec51cb8dd344708b3bbdfe67;hb=HEAD
install(
  DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/dlite
  DESTINATION "${Python3_SITEARCH}"
  USE_SOURCE_PERMISSIONS
  PATTERN ".gitignore" EXCLUDE
  PATTERN "*~" EXCLUDE
)
install(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/python-mapping-plugins
  DESTINATION ${DLITE_PKG_ROOT}/share/dlite
  USE_SOURCE_PERMISSIONS
  PATTERN ".gitignore" EXCLUDE
  PATTERN "*~" EXCLUDE
)
install(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/python-protocol-plugins
  DESTINATION ${DLITE_PKG_ROOT}/share/dlite
  USE_SOURCE_PERMISSIONS
  PATTERN ".gitignore" EXCLUDE
  PATTERN "*~" EXCLUDE
)
install(
  DIRECTORY ${dlite_SOURCE_DIR}/storages/python/python-storage-plugins
  DESTINATION ${DLITE_PKG_ROOT}/share/dlite
  USE_SOURCE_PERMISSIONS
  PATTERN ".gitignore" EXCLUDE
  PATTERN "*.json" EXCLUDE
  PATTERN "*~" EXCLUDE
)

add_subdirectory(scripts)


#
# Tests
# -----
add_subdirectory(tests)
