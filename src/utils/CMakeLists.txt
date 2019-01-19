# -*- Mode: cmake -*-
project(dlite-src-utils C)

#-----------------------------------------------------
# System tests
#-----------------------------------------------------

# Generate config.h
# -----------------

# -- check for headers
include(CheckIncludeFile)
check_include_file(stdarg.h HAVE_STDARG_H)
check_include_file(stddef.h HAVE_STDDEF_H)
check_include_file(stdint.h HAVE_STDINT_H)
check_include_file(stdlib.h HAVE_STDLIB_H)
check_include_file(float.h HAVE_FLOAT_H)
check_include_file(inttypes.h HAVE_INTTYPES_H)
check_include_file(locale.h HAVE_LOCALE_H)

# -- check for symbols
include(CheckSymbolExists)
check_symbol_exists(strdup       string.h   HAVE_STRDUP)
check_symbol_exists(_strdup      string.h   HAVE__STRDUP)
check_symbol_exists(strndup      string.h   HAVE_STRNDUP)
check_symbol_exists(_strndup     string.h   HAVE__STRNDUP)
check_symbol_exists(strcasecmp   string.h   HAVE_STRCASECMP)
check_symbol_exists(_stricmp     string.h   HAVE__STRICMP)
check_symbol_exists(strncasecmp  string.h   HAVE_STRNCASECMP)
check_symbol_exists(_strnicmp    string.h   HAVE__STRNICMP)

check_symbol_exists(getopt       unistd.h   HAVE_GETOPT)
check_symbol_exists(getopt_long  getopt.h   HAVE_GETOPT_LONG)

check_symbol_exists(snprintf     stdio.h    HAVE_SNPRINTF)
check_symbol_exists(_snprintf    stdio.h    HAVE__SNPRINTF)
check_symbol_exists(vsnprintf    stdio.h    HAVE_VSNPRINTF)
check_symbol_exists(_vsnprintf   stdio.h    HAVE__VSNPRINTF)
check_symbol_exists(vasprintf    stdio.h    HAVE_VASPRINTF)
check_symbol_exists(_vasprintf   stdio.h    HAVE__VASPRINTF)
check_symbol_exists(asprintf     stdio.h    HAVE_ASPRINTF)
check_symbol_exists(_asprintf    stdio.h    HAVE__ASPRINTF)
check_symbol_exists(localeconv   locale.h   HAVE__LOCALECONV)
check_symbol_exists(va_copy      stdarg.h   HAVE_VA_COPY)
check_symbol_exists(__va_copy    stdarg.h   HAVE___VA_COPY)

# -- check for types
include(CheckTypeSize)
check_type_size("long double"             HAVE_LONG_DOUBLE)
check_type_size("long long int"           HAVE_LONG_LONG_INT)
check_type_size("unsigned long long int"  HAVE_UNSIGNED_LONG_LONG_INT)
check_type_size(intmax_t                  HAVE_INTMAX_T)
check_type_size(uintmax_t                 HAVE_UINTMAX_T)
check_type_size(uintptr_t                 HAVE_UINTPTR_T)
check_type_size(ptrdiff_t                 HAVE_PTRDIFF_T)

# -- check for structure members
include(CheckStructHasMember)
check_struct_has_member("struct lconv"   decimal_point   locale.h
  HAVE_LCONV_DECIMAL_POINT)
check_struct_has_member("struct lconv"   thousands_sep   locale.h
  HAVE_LCONV_THOUSANDS_SEP)

# -- additional checks...
include(CheckCSourceCompiles)

check_c_source_compiles("
__thread int tls;
int main(void) { return 0; }" HAVE_GCC_THREAD_LOCAL_STORAGE)

check_c_source_compiles("
__declspec(thread) int tls;
int main(void) { return 0; }" HAVE_WIN32_THREAD_LOCAL_STORAGE)

check_c_source_compiles("
#define F(x, ...) f(x, __VA_ARGS__)
void f(int a, ...) { }
int main(void) { F(1, 2); return 0; }
" HAVE___VA_ARGS__)

check_c_source_compiles("
#define F(x, ...) f(x, __VA_ARGS__)
void f(int a, ...) { }
int main(void) { F(1); return 0; }
" HAVE_EXT__VA_ARGS__)

check_c_source_compiles("
#define F(x, ...) f(x, ##__VA_ARGS__)
void f(int a, ...) { }
int main(void) { F(1); return 0; }
" HAVE_EXT2__VA_ARGS__)

check_c_source_compiles("
#define F(x, ...) f(x __VA_OPT__(,) __VA_ARGS__)
void f(int a, ...) { }
int main(void) { F(1); return 0; }
" HAVE___VA_OPT__)

check_c_source_compiles("
int main(void) { char *f = __func__; return 0; }
" HAVE___FUNC__)

check_c_source_compiles("
int main(void) { char *f = __FUNCTION__; return 0; }
" HAVE___FUNCTION__)


# -- override default dynamic shared library extension in debug config on MSVS
if(MSVS AND CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(DSL_EXT "_d.dll")
endif()

configure_file(config.h.in config.h)

#-----------------------------------------------------
# Compile library
#-----------------------------------------------------
set(sources
  compat.c
  err.c
  fileutils.c
  globmatch.c
  map.c
  strtob.c
  tgen.c
  tmpfileplus.c
  )
if(NOT HAVE_GETOPT)
  list(APPEND sources compat/getopt.c)
endif()
if(NOT (HAVE_SNPRINTF OR HAVE__SNPRINTF) OR
   NOT (HAVE_ASPRINTF OR HAVE__ASPRINTF))
  list(APPEND sources compat/snprintf.c)
endif()

add_definitions(
  -DHAVE_CONFIG_H
  )

# Create .def files for Windows
#set(defs )
#if(MSVC)
#  set(scriptfile "${CMAKE_CURRENT_BINARY_DIR}/config_deps.cmake")
#  file(WRITE ${scriptfile} "configure_file(\${infile} \${outfile})")
#  foreach(source ${sources})
#    get_filename_component(basename ${source} NAME_WE)
#    set(infile "${CMAKE_CURRENT_SOURCE_DIR}/${basename}.def.in")
#    set(outfile "${CMAKE_CURRENT_BINARY_DIR}/${basename}.def")
#    if(EXISTS ${infile})
#      add_custom_command(
#        OUTPUT ${outfile}
#        COMMAND ${CMAKE_COMMAND}
#          -DLIBRARY=$<TARGET_FILE_NAME:dlite-utils>
#          -Dinfile=${infile}
#          -Doutfile=${outfile}
#          -P ${scriptfile}
#        DEPENDS
#          ${scriptfile}
#          ${infile}
#        COMMENT "Generating ${basename}.def"
#        )
#      list(APPEND defs ${outfile})
#    endif()
#  endforeach()
#endif()
include(GenerateDefs)
generate_defs(dlite-utils "${sources}" defs)

add_library(dlite-utils SHARED ${sources} ${defs})
add_library(dlite-utils-static STATIC ${sources})

set(dlite-utils_INCLUDE_DIRS
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  )
target_include_directories(dlite-utils PUBLIC ${dlite-utils_INCLUDE_DIRS})
target_include_directories(dlite-utils-static PUBLIC ${dlite-utils_INCLUDE_DIRS})

install(
  TARGETS dlite-utils dlite-utils-static
  DESTINATION lib
)

install(
  DIRECTORY .
  DESTINATION include/dlite/utils
  FILES_MATCHING
  PATTERN "*.h"
  PATTERN tests EXCLUDE
  PATTERN old EXCLUDE
)

# Tests
add_subdirectory(tests)
