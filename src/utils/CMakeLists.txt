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
check_include_file(endian.h     HAVE_ENDIAN_H)
check_include_file(byteswap.h   HAVE_BYTESWAP_H)
check_include_file(locale.h     HAVE_LOCALE_H)
check_include_file(time.h       HAVE_TIME_H)
check_include_file(unistd.h     HAVE_UNISTD_H)
check_include_file(windows.h    HAVE_WINDOWS_H)  # Windows-specific
check_include_file(io.h         HAVE_IO_H)       # Windows-specific

# -- check for symbols
set(CMAKE_C_FLAGS_saved ${CMAKE_C_FLAGS})
set(CMAKE_C_FLAGS "")
if(CMAKE_COMPILER_IS_GNUCC)
  set(CMAKE_REQUIRED_DEFINITIONS "-D_GNU_SOURCE")
endif()

include(CheckSymbolExists)
check_symbol_exists(strdup              string.h     HAVE_STRDUP)
check_symbol_exists(_strdup             string.h     HAVE__STRDUP)
check_symbol_exists(strndup             string.h     HAVE_STRNDUP)
check_symbol_exists(_strndup            string.h     HAVE__STRNDUP)
check_symbol_exists(strcasecmp          string.h     HAVE_STRCASECMP)
check_symbol_exists(_stricmp            string.h     HAVE__STRICMP)
check_symbol_exists(strncasecmp         string.h     HAVE_STRNCASECMP)
check_symbol_exists(_strnicmp           string.h     HAVE__STRNICMP)

check_symbol_exists(setenv              stdlib.h     HAVE_SETENV)
check_symbol_exists(unsetenv            stdlib.h     HAVE_UNSETENV)

check_symbol_exists(realpath            stdlib.h     HAVE_REALPATH)
check_symbol_exists(stat                sys/stat.h   HAVE_STAT)
check_symbol_exists(exec                unistd.h     HAVE_EXEC)
check_symbol_exists(clock               time.h       HAVE_CLOCK)
check_symbol_exists(P_tmpdir            stdio.h      HAVE_P_TMPDIR)
check_symbol_exists(_fileno             stdio.h      HAVE__FILENO)
check_symbol_exists(isatty              unistd.h     HAVE_ISATTY)
check_symbol_exists(_isatty             io.h         HAVE__ISATTY)

if(HAVE_WINDOWS_H)
check_symbol_exists(GetFullPathNameW    windows.h    HAVE_GetFullPathNameW)
check_symbol_exists(GetFullPathName     windows.h    HAVE_GetFullPathName)
check_symbol_exists(GetFileAttributesW  windows.h    HAVE_GetFileAttributesW)
check_symbol_exists(GetFileAttributes   windows.h    HAVE_GetFileAttributes)
endif()

list(APPEND CMAKE_REQUIRED_LIBRARIES shlwapi.lib)
check_symbol_exists(PathFileExistsW     shlwapi.h    HAVE_PathFileExistsW)
check_symbol_exists(PathFileExists      shlwapi.h    HAVE_PathFileExists)
list(REMOVE_ITEM CMAKE_REQUIRED_LIBRARIES shlwapi.lib)

check_symbol_exists(CreateProcessA      processthreadsapi.h HAVE_CreateProcessA)
check_symbol_exists(GetSystemTime       windows.h    HAVE_GetSystemTime)
check_symbol_exists(BCryptGenRandom     bcrypt.h     HAVE_BCryptGenRandom)

check_symbol_exists(mbstowcs            stdlib.h     HAVE_MBSTOWCS)
check_symbol_exists(wcstombs            stdlib.h     HAVE_WCSTOMBS)
check_symbol_exists(mbstowcs_s          stdlib.h     HAVE_MBSTOWCS_S)
check_symbol_exists(wcstombs_s          stdlib.h     HAVE_WCSTOMBS_S)

check_symbol_exists(strlcpy             string.h     HAVE_STRLCPY)
check_symbol_exists(strlcat             string.h     HAVE_STRLCAT)

check_symbol_exists(getopt              unistd.h     HAVE_GETOPT)
check_symbol_exists(getopt_long         getopt.h     HAVE_GETOPT_LONG)

# snprintf() is broken on windows - use own implementation there
if(NOT WIN32)
check_symbol_exists(snprintf            stdio.h      HAVE_SNPRINTF)
check_symbol_exists(_snprintf           stdio.h      HAVE__SNPRINTF)
check_symbol_exists(vsnprintf           stdio.h      HAVE_VSNPRINTF)
check_symbol_exists(_vsnprintf          stdio.h      HAVE__VSNPRINTF)
check_symbol_exists(asprintf            stdio.h      HAVE_ASPRINTF)
check_symbol_exists(_asprintf           stdio.h      HAVE__ASPRINTF)
check_symbol_exists(vasprintf           stdio.h      HAVE_VASPRINTF)
check_symbol_exists(_vasprintf          stdio.h      HAVE__VASPRINTF)
check_symbol_exists(asnprintf           stdio.h      HAVE_ASPPRINTF)
check_symbol_exists(asnprintf           stdio.h      HAVE_ASPPRINTF)
endif()
check_symbol_exists(localeconv          locale.h     HAVE__LOCALECONV)
check_symbol_exists(va_copy             stdarg.h     HAVE_VA_COPY)
check_symbol_exists(__va_copy           stdarg.h     HAVE___VA_COPY)

# C11 mutex
check_symbol_exists(mtx_init            threads.h    HAVE_MTX_INIT)
check_symbol_exists(mtx_lock            threads.h    HAVE_MTX_LOCK)
check_symbol_exists(mtx_timedlock       threads.h    HAVE_MTX_TIMEDLOCK)
check_symbol_exists(mtx_trylock         threads.h    HAVE_MTX_TRYLOCK)
check_symbol_exists(mtx_unlock          threads.h    HAVE_MTX_UNLOCK)
check_symbol_exists(mtx_destroy         threads.h    HAVE_MTX_DESTROY)



# -- typedefs
include(CheckTypeSize)
list(APPEND CMAKE_EXTRA_INCLUDE_FILES time.h)
check_type_size(clock_t                  HAVE_CLOCK_T)
list(POP_BACK CMAKE_EXTRA_INCLUDE_FILES)

set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS_saved})


# -- check for structure members
include(CheckStructHasMember)
check_struct_has_member("struct lconv"   decimal_point   locale.h
  HAVE_LCONV_DECIMAL_POINT)
check_struct_has_member("struct lconv"   thousands_sep   locale.h
  HAVE_LCONV_THOUSANDS_SEP)


# -- check for threads
if(WITH_THREADS)
  find_package(Threads)
  if(Threads_FOUND)
    set(HAVE_THREADS TRUE)
    add_definitions(-DHAVE_THREADS)

    # On some 32-bit systems (like manylinux2014_i686) we have to
    # explicitly link with the threads library
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
      list(APPEND extra_link_libraries ${CMAKE_THREAD_LIBS_INIT})
    endif()

  endif()
endif()

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
int f(int a, int b) { return a + b; }
int main(void) { F(1, 2); return 0; }
" HAVE___VA_ARGS__)

check_c_source_compiles("
#define F(x, ...) f(x, __VA_ARGS__)
int f(int a) { return a; }
int main(void) { F(1); return 0; }
" HAVE_EXT__VA_ARGS__)

check_c_source_compiles("
#define F(x, ...) f(x, ##__VA_ARGS__)
int f(int a) { return a; }
int main(void) { F(1); return 0; }
" HAVE_EXT2__VA_ARGS__)

check_c_source_compiles("
#define F(x, ...) f(x __VA_OPT__(,) __VA_ARGS__)
int f(int a) { return a; }
int main(void) { F(1); return 0; }
" HAVE___VA_OPT__)

check_c_source_compiles("
int main(void) { const char *f = __func__; (void)(f); return 0; }
" HAVE___FUNC__)

check_c_source_compiles("
int main(void) { const char *f = __FUNCTION__; (void)(f); return 0; }
" HAVE___FUNCTION__)


# -- override default dynamic shared library extension in debug config on MSVS
#if(MSVS AND CMAKE_BUILD_TYPE STREQUAL "Debug")
#  set(DSL_EXT "_d.dll")
#endif()

# .. generate config.h
configure_file(config.h.in config.h)


# Generate byteorder.h
# --------------------
check_include_file(byteswap.h   HAVE_BYTESWAP_H)
check_include_file(endian.h     HAVE_ENDIAN_H)

if(NOT HAVE_BYTESWAP_H)
  configure_file(byteorder.h.in byteswap.h)
endif()
if(NOT HAVE_ENDIAN_H)
  configure_file(byteorder.h.in endian.h)
endif()

if(WIN64)
  set(IS_BIG_ENDIAN 0 CACHE INTERNAL "Windows is not big endian" FORCE)
  set(HAVE_IS_BIG_ENDIAN 0 CACHE INTERNAL "Windows is not big endian" FORCE)
else()
  include(TestBigEndian)
  test_big_endian(IS_BIG_ENDIAN)
endif()

check_symbol_exists(bswap_16 byteswap.h HAVE_BSWAP_16)
check_symbol_exists(bswap_32 byteswap.h HAVE_BSWAP_32)
check_symbol_exists(bswap_64 byteswap.h HAVE_BSWAP_64)
check_symbol_exists(bswap_128 byteswap.h HAVE_BSWAP_128)

check_symbol_exists(_byteswap_ushort stdlib.h HAVE__BYTESWAP_USHORT)
check_symbol_exists(_byteswap_ulong  stdlib.h HAVE__BYTESWAP_ULONG)
check_symbol_exists(_byteswap_uint64 stdlib.h HAVE__BYTESWAP_UINT64)
check_symbol_exists(_byteswap_uint128 stdlib.h HAVE__BYTESWAP_UINT128)

check_symbol_exists(htobe16 endian.h HAVE_HTOBE16)
check_symbol_exists(htobe32 endian.h HAVE_HTOBE32)
check_symbol_exists(htobe64 endian.h HAVE_HTOBE64)
check_symbol_exists(htobe128 endian.h HAVE_HTOBE128)

configure_file(byteorder.h.in byteorder.h)


# Generate boolean.h
# ------------------
include(CheckIncludeFile)
check_include_file(stdbool.h HAVE_STDBOOL_H)

include(CheckTypeSize)
check_type_size(_Bool SIZEOF_BOOL)

configure_file(boolean.h.in boolean.h)


# Generate integers.h
# -------------------
check_type_size("long double"             HAVE_LONG_DOUBLE)
check_type_size("long long int"           HAVE_LONG_LONG_INT)
check_type_size("unsigned long long int"  HAVE_UNSIGNED_LONG_LONG_INT)
check_type_size(uint8_t                   HAVE_UINT8_T)
check_type_size(uint16_t                  HAVE_UINT16_T)
check_type_size(uint32_t                  HAVE_UINT32_T)
check_type_size(uint64_t                  HAVE_UINT64_T)
check_type_size(uint128_t                 HAVE_UINT128_T)
check_type_size(__uint128_t               HAVE___UINT128_T)
check_type_size(int128_t                  HAVE_INT128_T)
check_type_size(__int128_t                HAVE___INT128_T)
check_type_size(intmax_t                  HAVE_INTMAX_T)
check_type_size(uintmax_t                 HAVE_UINTMAX_T)
check_type_size(uintptr_t                 HAVE_UINTPTR_T)
check_type_size(ptrdiff_t                 HAVE_PTRDIFF_T)
set(CMAKE_REQUIRED_INCLUDES stdlib.h)
check_type_size(size_t                    HAVE_SIZE_T)
unset(CMAKE_REQUIRED_INCLUDES)

check_type_size(char                      SIZEOF_CHAR)
check_type_size(short                     SIZEOF_SHORT)
check_type_size(int                       SIZEOF_INT)
check_type_size(long                      SIZEOF_LONG)
check_type_size("long long"               SIZEOF_LONG_LONG)
check_type_size(size_t                    SIZEOF_SIZE_T)

configure_file(integers.h.in integers.h)


# Generate floats.h
# ------------------
check_type_size(float         SIZEOF_FLOAT)
check_type_size(double        SIZEOF_DOUBLE)
check_type_size("long double" SIZEOF_LONG_DOUBLE)

configure_file(floats.h.in floats.h)

# Generate config_snprintf.h
# --------------------------
if(NOT (HAVE_SNPRINTF OR HAVE__SNPRINTF) OR
   NOT (HAVE_VSPRINTF OR HAVE__VSPRINTF) OR
   NOT (HAVE_ASPRINTF OR HAVE__ASPRINTF) OR
   NOT (HAVE_VASPRINTF OR HAVE__VASPRINTF))
  set(WITH_SNPRINTF 1)
  configure_file(compat/config_snprintf.h.in config_snprintf.h)
endif()


#-----------------------------------------------------
# Compile library
#-----------------------------------------------------

set(sources
  compat.c
  err.c
  unicode.c
  strutils.c
  fileinfo.c
  fileutils.c
  urlsplit.c
  execprocess.c
  globmatch.c
  plugin.c
  map.c
  clp2.c
  strtob.c
  tgen.c
  tmpfileplus.c
  infixcalc.c
  jsmnx.c
  jstore.c
  bson.c
  session.c
  rng.c
  uri_encode.c

  md5.c
  sha1.c
  sha3.c
  uuid.c
  uuid4.c
  )
if(NOT HAVE_GETOPT)
  list(APPEND sources compat/getopt.c)
endif()
if(WITH_SNPRINTF)
  list(APPEND sources compat/snprintf.c)
  add_definitions(-DHAVE_CONFIG_SNPRINTF_H)
endif()
if(NOT HAVE_REALPATH AND UNIX)
  list(APPEND sources compat/realpath.c)
endif()

add_definitions(
  -DHAVE_CONFIG_H
  )

# FIXME - Should we add this here?
#if(WINDOWS)
#  add_definitions(-DDLLEXPORT)
#endif()

add_library(dlite-utils SHARED ${sources})
add_library(dlite-utils-static STATIC ${sources})

set_target_properties(dlite-utils PROPERTIES
  WINDOWS_EXPORT_ALL_SYMBOLS ON
  )
set_target_properties(dlite-utils-static PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  )

target_include_directories(dlite-utils PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include/dlite/utils>
  )
target_include_directories(dlite-utils-static PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include/dlite/utils>
  )

set(link_libs)
if(HAVE_PathFileExists OR HAVE_PathFileExistsW)
  list(APPEND link_libs shlwapi)
endif()
if(Threads_FOUND)
  add_definitions(-DHAVE_THREADS)
  list(APPEND link_libs Threads::Threads)
endif()
target_link_libraries(dlite-utils ${link_libs})
target_link_libraries(dlite-utils-static ${link_libs})


## create list of headers to install from ${sources}
set(headers "")
foreach(source ${sources})
  string(REPLACE ".c" ".h" header ${source})
  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${header})
    list(APPEND headers ${header})
  endif()
endforeach()
list(APPEND headers
  ${CMAKE_CURRENT_BINARY_DIR}/boolean.h
  ${CMAKE_CURRENT_BINARY_DIR}/byteorder.h
  ${CMAKE_CURRENT_BINARY_DIR}/integers.h
  ${CMAKE_CURRENT_BINARY_DIR}/floats.h
  )
set_target_properties(dlite-utils PROPERTIES PUBLIC_HEADER "${headers}")

install(
  TARGETS dlite-utils dlite-utils-static
  EXPORT  dliteTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  PUBLIC_HEADER DESTINATION include/dlite/utils
  INCLUDES DESTINATION include/dlite/utils
  )

# Install jsmn.h explicitly since it has no corresponding .c file
# included in `sources`
install(
  FILES
    jsmn.h
  DESTINATION include/dlite/utils
  )

install(
  FILES
    compat/dirent.h
    compat/getopt.h
  DESTINATION include/dlite/utils/compat
  )


# Tests
add_subdirectory(tests)
