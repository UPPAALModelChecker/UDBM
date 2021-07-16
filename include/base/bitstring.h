/* -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/*********************************************************************
 *
 * Filename : bitstring.h (base)
 * C/C++ header.
 *
 * Basic operations on bits and strings of bits + BitString & Bit classes.
 *
 * This file is a part of the UPPAAL toolkit.
 * Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
 * All right reserved.
 *
 * v 1.1 reviewed.
 * $Id: bitstring.h,v 1.17 2005/05/14 16:57:34 behrmann Exp $
 *
 *********************************************************************/

#ifndef INCLUDE_BASE_BIT_H
#define INCLUDE_BASE_BIT_H

#include <assert.h>
#include <stddef.h>

#include "base/intutils.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Type to ensure correct arguments.
 */
typedef enum { ZERO = 0, ONE = 1 } bit_t;

/** Bit counting function.
 * @return: the number of 1s in a given int
 * @param x: the int to examine.
 */
static inline uint32_t base_countBits(uint32_t x)
{
    /* algorithm: count bits in parallel (hack) */
    x -= (x & 0xaaaaaaaa) >> 1;
    x = ((x >> 2) & 0x33333333L) + (x & 0x33333333L);
    x = ((x >> 4) + x) & 0x0f0f0f0fL;
    x = ((x >> 8) + x);
    x = ((x >> 16) + x) & 0xff;
    return x;
}

/** Bit counting function.
 * @return: the number of 1s in a given string of bits
 * @param bitString: the string of bits to examine
 * @param n: number of ints to read.
 */
static inline uint32_t base_countBitsN(const uint32_t* bitString, size_t n)
{
    uint32_t cnt;
    assert(n == 0 || bitString);
    for (cnt = 0; n != 0; --n)
        cnt += base_countBits(*bitString++);
    return cnt;
}

/** Reset of bits (to 0).
 * memset could be used but for strings of
 * bits of length 3-4 ints max, it is overkill.
 * Conversely, for larger arrays, don't use this
 * function.
 * @param bits: the string to reset
 * @param n: number of ints to write.
 */
static inline void base_resetBits(uint32_t* bits, size_t n)
{
    assert(n == 0 || bits);
    for (; n != 0; --n)
        *bits++ = 0;
}

/** Equality test to 0. Optimistic
 * implementation: works best when == 0.
 * @param bits: the string to test
 * @param n: n number of ints to test
 * @return true if all bits[0..n-1] == 0
 */
static inline bool base_areBitsReset(const uint32_t* bits, size_t n)
{
    uint32_t diff = 0; /* accumulate the difference to 0 */
    assert(n == 0 || bits);
    for (; n != 0; --n)
        diff |= *bits++;
    return (diff == 0);
}

/** Wrapper to base_areBitsResets
 */
static inline bool base_areIBitsReset(const int32_t* bits, size_t n)
{
    return base_areBitsReset((uint32_t*)bits, n);
}

/** Comparison of bits.
 * Same comment as for resetBits: better
 * to inline this simple code for small
 * arrays, ie, bit arrays. Optimistic implementation.
 * @param bits1,bits2: arrays to compare
 * @param n: number of ints to read
 * @return true if array contents are the same
 */
static inline bool base_areBitsEqual(const uint32_t* bits1, const uint32_t* bits2, size_t n)
{
    uint32_t diff = 0;
    assert(n == 0 || (bits1 && bits2));
    for (; n != 0; --n)
        diff |= *bits1++ ^ *bits2++;
    return (diff == 0);
}

/** Set bit to 1 at position i in a table of bits.
 * @param bits: the table of *int*
 * @param i: the position of the bit
 * Bit position in a table of int =
 * int position at i/32 == i >> 5
 * bit position at i%32 == i & 31
 * NOTE: we don't use setBit and resetBit
 * as names because they are too close to
 * resetBits that is already used.
 * @pre valid pointer and no out of bounds.
 */
static inline void base_setOneBit(uint32_t* bits, size_t i)
{
    assert(bits);
    bits[i >> 5] |= 1u << (i & 31);
}

/** Reset bit to 0 at position i in a table of bits.
 * @param bits: the table of *int*
 * @param i: the position of the bit
 * Bit position in a table of int =
 * int position at i/32 == i >> 5
 * bit position at i%32 == i & 31
 * NOTE: don't use setBit and resetBit
 * as names because they are too close to
 * resetBits that is already used.
 * @pre valid pointer and not out of bounds.
 */
static inline void base_resetOneBit(uint32_t* bits, size_t i)
{
    assert(bits);
    bits[i >> 5] &= ~(1u << (i & 31));
}

/** Toggle bit at position i in a table of bits.
 * @param bits: the table of *int*
 * @param i: the position of the bit
 * Bit position in a table of int =
 * int position at i/32 == i >> 5
 * bit position at i%32 == i & 31
 * @pre valid pointer and not out of bounds.
 */
static inline void base_toggleOneBit(uint32_t* bits, size_t i)
{
    assert(bits);
    bits[i >> 5] ^= 1u << (i & 31);
}

/** Test for a set bit in a bit-string.
 * Can be used in the following way. Instead of:
 * \code
 * if (base_getOneBit(a,n)) c++;
 * \endcode
 * you can write:
 * \code
 * c += base_getOneBit(a,n);
 * \endcode
 * @returns ONE if \a i'th bit is set, ZERO otherwise.
 * @pre valid pointer and not out of bounds.
 */
static inline bit_t base_getOneBit(const uint32_t* bits, size_t i)
{
    assert(bits);
    return (bit_t)((bits[i >> 5] >> (i & 31)) & 1);
}

/** Test for a set bit in a bit-string. This function is not the same
 * as \c base_getOneBit(). This function returns an integer with all
 * other bits except the \a i'th bit masked out. I.e. it returns 0 if
 * the bit is not set, and a power of 2 if it is set.
 *
 * @returns the bit as it is in the int, ie, shifted.
 * @pre valid pointer and not out of bounds.
 */
static inline uint32_t base_readOneBit(const uint32_t* bits, size_t i)
{
    assert(bits);
    return bits[i >> 5] & (1u << (i & 31));
}

/** Unpack a bit table to an index table.
 * Useful when loading a DBM and computing initial
 * redirection table. May be used for other things
 * than DBMs, e.g., variables.
 * @param table: index redirection table
 * @param bits: bit array
 * @param n: size in int of the bit array
 * @return number of bits in bits
 * @pre
 * - table is large enough: at least
 *   uint32_t[size] where size = index of the highest bit
 *   in the bitstring bits.
 * - bits is at least a uint32_t[n]
 * @post
 * - returned value == number of bits in bits
 * - for all bits set in bits at position i,
 *   then table[i] is defined and gives the
 *   proper indirection ; for other bits, table[i]
 *   is untouched.
 */
size_t base_bits2indexTable(const uint32_t* bits, size_t n, cindex_t* table);

#ifdef __cplusplus
}

#endif /* __cplusplus */

#endif /* INCLUDE_BASE_BITSTRING_H */
