/* -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/*********************************************************************
 *
 * Filename : inttypes.h (base)
 * C header.
 *
 * Definition of integer types. Basically a wrapper for different
 * includes + basic size computation in int units.
 *
 * This file is a part of the UPPAAL toolkit.
 * Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
 * All right reserved.
 *
 * v 1.2 reviewed.
 * $Id: inttypes.h,v 1.11 2005/04/22 15:20:10 adavid Exp $
 *
 *********************************************************************/

#ifndef INCLUDE_BASE_INTTYPES_H
#define INCLUDE_BASE_INTTYPES_H

#include "config.h"

// use .h here as this file is BOTH c and c++ (i.e. cinttypes is only c++)
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>

/** Size computation in "int" units to avoid any alignment problem.
 * intsizeof    : as sizeof but result in "int" units
 * bits2intsize : convert # of bits  to # of int (size+31)/32
 * bytes2intsize: convert # of bytes to # of int (size+3)/4
 */

#define bits2intsize(X)  (((X) + 31u) >> 5u)
#define bytes2intsize(X) (((X) + 3u) >> 2u)
#define intSizeOf(X)     bytes2intsize(sizeof(X))
#define intSizeOfA(X, N) bytes2intsize(sizeof(X) * N)

/* Define ARCH_APPLE_DARWIN on Mac OS X.
 */
#if defined(__APPLE__) && defined(__MACH__)
#define ARCH_APPLE_DARWIN
#endif

/** Type for indices (variables and clocks).
 */
typedef uint32_t cindex_t;

#endif /* INCLUDE_BASE_INTTYPES_H */
