/* -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/*********************************************************************
 *
 * Filename : macros.h (debug)
 * C header.
 *
 * Debugging macros:
 * - debugging statement with DODEBUG(something)
 * - pretty colors
 * - print position (in a function/method)
 *
 * This file is a part of the UPPAAL toolkit.
 * Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
 * All right reserved.
 *
 * v 1.2 reviewed.
 * $Id: macros.h,v 1.13 2005/06/22 13:30:32 adavid Exp $
 *
 **********************************************************************/

#ifndef INCLUDE_DEBUG_MACROS_H
#define INCLUDE_DEBUG_MACROS_H

#include "debug/utils.h"

/**********************************************************************
 * Macros for debugging
 *  NPRETTY_COLORS -> deactivate colorized printouts
 *  NSHORTFILENAME -> deactivate short filename printouts
 *  NDEBUG: controls debugging, standard
 **********************************************************************/

/* Incompatibility: deactive pretty features */
#ifdef _WIN32
#define __PRETTY_FUNCTION__ __FUNCTION__
#ifndef NPRETTY_COLORS
#define NPRETTY_COLORS
#endif
#endif

/* C++ example: std::cout << RED(BOLD) "Warning!" NORMAL << std::endl
 * C example: printf(RED(BOLD)"Warning!"NORMAL"\n")
 */
#ifndef NPRETTY_COLORS
#define THIN       "0"
#define BOLD       "1"
#define RED(S)     "\033[" S ";31m"
#define GREEN(S)   "\033[" S ";32m"
#define YELLOW(S)  "\033[" S ";33m"
#define BLUE(S)    "\033[" S ";34m"
#define MAGENTA(S) "\033[" S ";35m"
#define CYAN(S)    "\033[" S ";36m"
#define WHITE(S)   "\033[" S ";37m"
#define NORMAL     "\033[0;0m"
#else
#define THIN       ""
#define BOLD       ""
#define RED(S)     ""
#define GREEN(S)   ""
#define YELLOW(S)  ""
#define BLUE(S)    ""
#define MAGENTA(S) ""
#define CYAN(S)    ""
#define WHITE(S)   ""
#define NORMAL     ""
#endif

/* C/C++ default output streams.
 * No need to be in conditional def.
 */
#ifdef __cplusplus
#define DEFAULT_OUT std::cout
#define DEFAULT_ERR std::cerr
#else
#define DEFAULT_OUT stdout
#define DEFAULT_ERR stderr
#endif

#ifndef NDEBUG

#include <stdlib.h>  // abort(void)

/* C/C++ print style
 */
#ifdef __cplusplus
#define PRINT(OUT, S)     (OUT) << (S)
#define PRINT_INT(OUT, N) (OUT) << (N)
#else
#define PRINT(OUT, S)     fprintf(OUT, "%s", (S))
#define PRINT_INT(OUT, N) fprintf(OUT, "%d", (N))
#endif

/* Long or short printouts?
 */
#ifndef NSHORTFILENAME
#define PRINT_FILE(OUT) PRINT(OUT, debug_shortSource(__FILE__))
#else
#define PRINT_FILE(OUT) PRINT(OUT, __FILE__)
#endif

/* Statement to execute only for debugging
 * WARNING: there should not be side effect
 * on the normal code.
 */
#define DODEBUG(STATEMENT) STATEMENT

/* Print some info from a function.
 */
#define PRINT_INFO(INFO)                         \
    do {                                         \
        PRINT(DEFAULT_OUT, __PRETTY_FUNCTION__); \
        PRINT(DEFAULT_OUT, ": " INFO "\n");      \
    } while (0)

/* Pretty print info in debugging mode:
 * with and without color.
 * Do not concatenate __PRETTY_FUNCTION__
 * because it is deprecated.
 */
#ifndef NPRETTY_COLORS
#define PRINT_CINFO(COLOR, INFO)                   \
    do {                                           \
        PRINT(DEFAULT_OUT, COLOR);                 \
        PRINT(DEFAULT_OUT, __PRETTY_FUNCTION__);   \
        PRINT(DEFAULT_OUT, ": " INFO NORMAL "\n"); \
    } while (0)
#else
#define PRINT_CINFO(COLOR, INFO) PRINT_INFO(INFO)
#endif

/* This is essentially to avoid warning
 * on printf("")
 */
#ifndef NPRETTY_COLORS
#define PRINT_COLOR(OUT, COLOR) PRINT(OUT, COLOR)
#else
#define PRINT_COLOR(OUT, COLOR)
#endif

/* Like an ordinary assert but allow
 * to print debug information if the
 * assertion is violated.
 */
#define ASSERT(COND, PRINTME)                                                             \
    do {                                                                                  \
        if (!(COND)) {                                                                    \
            PRINT_COLOR(DEFAULT_ERR, RED(BOLD));                                          \
            PRINT_FILE(DEFAULT_ERR);                                                      \
            PRINT(DEFAULT_ERR, "(");                                                      \
            PRINT_INT(DEFAULT_ERR, __LINE__);                                             \
            PRINT(DEFAULT_ERR, ") ");                                                     \
            PRINT(DEFAULT_ERR, __PRETTY_FUNCTION__);                                      \
            PRINT(DEFAULT_ERR,                                                            \
                  ": Assertion `" MAGENTA(BOLD) #COND RED(BOLD) "' failed." NORMAL "\n"); \
            PRINTME;                                                                      \
            abort();                                                                      \
        }                                                                                 \
    } while (0) /* standard trick to make it a statement */

/* Like assert but controlled by another flag
 * to tune testing with or without expensive asserts.
 */
#ifndef DISABLE_ASSERTX
#define assertx(STATEMENT)  assert(STATEMENT)
#define DODEBUGX(STATEMENT) STATEMENT
#else
#define assertx(STATEMENT)
#define DODEBUGX(STATEMENT)
#endif

#else /* NDEBUG */

#define PRINT(OUT, S)
#define PRINT_INT(OUT, N)
#define PRINT_FILE(OUT)
#define DODEBUG(STATEMENT)
#define PRINT_INFO(INFO)
#define PRINT_CINFO(COLOR, INFO)
#define ASSERT(COND, PRINTME)
#define PRINT_COLOR(OUT, COLOR)
#define assertx(STATEMENT)
#define DODEBUGX(STATEMENT)

#endif /* NDEBUG */

/* On Sun/Solaris, rand() is bugged,
 * thank you Sun! rand() returns a
 * random number only on 16 bits,
 * which is quite laughable.
 * It is fine to use rand() if you
 * are satisfied with a 16 bits number
 * though. This #define may be wrong
 * on Intel running Solaris, but it
 * isn't sane to do so anyway.
 */
#if INTEL_ARCH

#define RAND() rand()

/* include inttypes.h for this */
#define RAND64() ((((uint64_t)rand()) << 32) | (((uint64_t)rand())))

#else

#define RAND() ((rand() ^ (rand() << 16)) & 0x7fffffff)

/* include inttypes.h for this */
#define RAND64()                                                                            \
    ((((uint64_t)rand()) << 48) | (((uint64_t)rand()) << 32) | (((uint64_t)rand()) << 16) | \
     (((uint64_t)rand())))

#endif /* INTEL_ARCH */

#if UINTPTR_MAX == 0xFFFFFFFF
#define RAND_SIZE() RAND()
#elif UINTPTR_MAX == 0xFFFFFFFFFFFFFFFFu
#define RAND_SIZE() RAND64()
#else
#error unknown pointer size
#endif

#endif /* INCLUDE_DEBUG_MACROS_H */
