/*
 * getopt - POSIX like getopt for Windows console Application
 *
 * win-c - Windows Console Library
 * Copyright (c) 2015 Koji Takami
 * Released under the MIT license
 * https://github.com/takamin/win-c/blob/master/LICENSE
 */

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#ifdef HAVE_GETOPT
#include <getopt.h>
#else
#ifndef _GETOPT_H_
#define _GETOPT_H_

    int getopt(int argc, char* const argv[],
            const char* optstring);

    extern char *optarg;
    extern int optind, opterr, optopt;

#define no_argument 0
#define required_argument 1
#define optional_argument 2

    struct option {
        const char *name;
        int has_arg;
        int* flag;
        int val;
    };

    int getopt_long(int argc, char* const argv[],
            const char* optstring,
            const struct option* longopts, int* longindex);
/****************************************************************************
    int getopt_long_only(int argc, char* const argv[],
            const char* optstring,
            const struct option* longopts, int* longindex);
****************************************************************************/

#endif  /* HAVE_GETOPT */

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // _GETOPT_H_
