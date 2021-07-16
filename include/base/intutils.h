/* -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/*********************************************************************
 *
 * Filename : intutils.h (base)
 *
 * Utility functions for integers.
 *
 * This file is a part of the UPPAAL toolkit.
 * Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
 * All right reserved.
 *
 * $Id: intutils.h,v 1.13 2005/04/22 15:20:10 adavid Exp $
 *
 **********************************************************************/

#ifndef INCLUDE_BASE_INTUTILS_H
#define INCLUDE_BASE_INTUTILS_H

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "base/inttypes.h"

/**
 * Check if an unsigned integer is a power of two
 * @param v: the value to check
 * @return true if power of two, else false
 */
static inline bool base_isPowerOfTwo(size_t v) { return v && !(v & (v - 1)); }

/** Check if memory differs with a given value.
 * @param mem: memory to check
 * @param intSize: size in int to fill
 * @param intValue: value to check
 * @return 0 if mem[0..intSize-1] == intValue,
 * or !=0 otherwise.
 */
uint32_t base_diff(const void* mem, size_t intSize, uint32_t intValue);

/** Fill memory with a given value.
 * @param from: memory to fill
 * @param to: ptr after last element to fill
 * @param intval: value to use to fill
 * @post from[i] == inval for 0 <= i < (to - from).
 */
void base_fill(int32_t* from, int32_t* to, uint32_t intval);

/** Test equality. (optimistic implementation)
 * @param data1,data2: data to be compared.
 * @param intSize: size in int to compare.
 * @return true if the contents are the same.
 * @pre data1 and data2 are of size int32_t[intSize]
 * null pointers are accepted as argument
 */
bool base_areEqual(const void* data1, const void* data2, size_t intSize);

/** @return (x < 0 ? ~x : x) without a jump!
 * @param x: int to test.
 */
static inline int32_t base_absNot(int32_t x)
{
    /* Algo: see base_abs
     */
    uint32_t sign = ((uint32_t)x) >> 31;
    return (x ^ -sign);
}

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_BASE_INTUTILS_H */
