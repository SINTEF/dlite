# -*- Mode: cmake -*-
project(dlite-src-utils C)

#-----------------------------------------------------
# System tests
#-----------------------------------------------------

# Generate config.h
# -----------------

# -- check for headers
include(CheckIncludeFile)
check_include_file(stdarg.h     HAVE_STDARG_H)
check_include_file(stddef.h     HAVE_STDDEF_H)
check_include_file(stdint.h     HAVE_STDINT_H)
check_include_file(stdlib.h     HAVE_STDLIB_H)
check_include_file(float.h      HAVE_FLOAT_H)
check_include_file(inttypes.h   HAVE_INTTYPES_H)
check_include_file(locale.h     HAVE_LOCALE_H)

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

check_symbol_exists(setenv       stdlib.h   HAVE_SETENV)
check_symbol_exists(unsetenv     stdlib.h   HAVE_UNSETENV)

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
#if(MSVS AND CMAKE_BUILD_TYPE STREQUAL "Debug")
#  set(DSL_EXT "_d.dll")
#endif()

# .. generate config.h
configure_file(config.h.in config.h)


# Generate byteorder.h
# --------------------
include(TestBigEndian)
test_big_endian(IS_BIG_ENDIAN)

check_include_file(byteswap.h   HAVE_BYTESWAP_H)
check_include_file(endian.h     HAVE_ENDIAN_H)

check_symbol_exists(bswap_16 byteswap.h HAVE_BSWAP_16)
check_symbol_exists(bswap_32 byteswap.h HAVE_BSWAP_32)
check_symbol_exists(bswap_64 byteswap.h HAVE_BSWAP_64)

check_symbol_exists(_byteswap_ushort stdlib.h HAVE__BYTESWAP_USHORT)
check_symbol_exists(_byteswap_ulong  stdlib.h HAVE__BYTESWAP_ULONG)
check_symbol_exists(_byteswap_uint64 stdlib.h HAVE__BYTESWAP_UINT64)

check_symbol_exists(htobe16 endian.h HAVE_HTOBE16)
check_symbol_exists(htobe32 endian.h HAVE_HTOBE32)
check_symbol_exists(htobe64 endian.h HAVE_HTOBE64)

configure_file(byteorder.h.in byteorder.h)


# Generate boolean.h
# ------------------
include(CheckIncludeFile)
check_include_file(stdbool.h HAVE_STDBOOL_H)

include(CheckTypeSize)
check_type_size(_Bool _BOOL)

configure_file(boolean.h.in boolean.h)


# Generate integers.h
# -------------------
include(CheckTypeSize)

check_type_size("long double"             HAVE_LONG_DOUBLE)
check_type_size("long long int"           HAVE_LONG_LONG_INT)
check_type_size("unsigned long long int"  HAVE_UNSIGNED_LONG_LONG_INT)
check_type_size(uint8_t                   HAVE_UINT8_T)
check_type_size(uint16_t                  HAVE_UINT16_T)
check_type_size(uint32_t                  HAVE_UINT32_T)
check_type_size(uint64_t                  HAVE_UINT64_T)
check_type_size(intmax_t                  HAVE_INTMAX_T)
check_type_size(uintmax_t                 HAVE_UINTMAX_T)
check_type_size(uintptr_t                 HAVE_UINTPTR_T)
check_type_size(ptrdiff_t                 HAVE_PTRDIFF_T)

check_type_size(char                      SIZEOF_CHAR)
check_type_size(short                     SIZEOF_SHORT)
check_type_size(int                       SIZEOF_INT)
check_type_size(long                      SIZEOF_LONG)
check_type_size("long long"               SIZEOF_LONG_LONG)

configure_file(integers.h.in integers.h)


# Generate floats.h
# ------------------
check_type_size(float         SIZEOF_FLOAT)
check_type_size(double        SIZEOF_DOUBLE)
check_type_size("long double" SIZEOF_LONG_DOUBLE)

configure_file(floats.h.in floats.h)


#-----------------------------------------------------
# Compile library
#-----------------------------------------------------
set(sources
  compat.c
  err.c
  fileutils.c
  globmatch.c
  plugin.c
  map.c
  strtob.c
  tgen.c
  tmpfileplus.c

  md5.c
  sha1.c
  uuid.c
  uuid4.c
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

add_library(dlite-utils SHARED ${sources})
add_library(dlite-utils-static STATIC ${sources})

set_target_properties(dlite-utils PROPERTIES
  WINDOWS_EXPORT_ALL_SYMBOLS ON
  )

target_include_directories(dlite-utils PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  )
target_include_directories(dlite-utils-static PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  )
target_include_directories(dlite-utils PUBLIC ${dlite-utils_INCLUDE_DIRS})
target_include_directories(dlite-utils-static PUBLIC ${dlite-utils_INCLUDE_DIRS})

install(
  TARGETS dlite-utils dlite-utils-static
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  )

install(
  DIRECTORY .
  DESTINATION include/dlite/utils
  FILES_MATCHING
  PATTERN "*.h"
  PATTERN tests EXCLUDE
  PATTERN old EXCLUDE
  )

install(
  FILES
  ${CMAKE_CURRENT_BINARY_DIR}/byteorder.h
  ${CMAKE_CURRENT_BINARY_DIR}/integers.h
  ${CMAKE_CURRENT_BINARY_DIR}/boolean.h
  ${CMAKE_CURRENT_BINARY_DIR}/floats.h
  DESTINATION include/dlite/utils
  )

# Tests
add_subdirectory(tests)
