/* -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/*********************************************************************
 *
 * Filename : testbitstring.c (base/tests)
 *
 * Test bitstring.c.
 *
 * This file is a part of the UPPAAL toolkit.
 * Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
 * All right reserved.
 *
 * $Id: testbitstring.c,v 1.5 2005/04/22 15:20:08 adavid Exp $
 *
 *********************************************************************/

/* Tests are always for debugging. */

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "base/bitstring.h"
#include "debug/macros.h"

/* safe count of bits in a:
 * shift to the right and count the
 * right most bit.
 */
static uint32_t checkBits(uint32_t a)
{
    uint32_t count = 0;
    while (a) {
        count += a & 1;
        a >>= 1;
    }
    return count;
}

/* safe count of bits
 */
static uint32_t checkBitsN(const uint32_t* b, uint32_t n)
{
    uint32_t count = 0;
    while (n) {
        count += checkBits(*b);
        b++;
        --n;
    }
    return count;
}

/** invalidate table indices
 */
static void test_invalidate(uint32_t* table, uint32_t n)
{
    while (n) {
        *table++ = 0xffffffff;
        --n;
    }
}

/** test table generation
 */
static void test_bits2indexTable(uint32_t size)
{
    uint32_t intSize = bits2intsize(size);
    uint32_t bitString[intSize];
    uint32_t tableSize = intSize * 32;
    uint32_t table[tableSize];
    uint32_t k;

    printf("Testing bits2indexTable(%u)\n", size);

    for (k = 0; k < 1000; ++k) {
        uint32_t i, counter = 0;

        test_invalidate(table, tableSize);
        debug_generateBits(bitString, intSize, size, ZERO);
        base_bits2indexTable(bitString, intSize, table);

        for (i = 0; i < tableSize; ++i) {
            /* all valid entries at position i
             * should match a bit at position i
             * and the index should be the current
             * size so far
             */
            if (table[i] == 0xffffffff) {
                assert(!base_getOneBit(bitString, i));
            } else {
                assert(base_getOneBit(bitString, i));
                assert(table[i] == counter);
                counter++;
            }
        }
        assert(counter == size);
    }
}

static void tests(int bitSize)
{
    uint32_t k, a;
    uint32_t intSize = bits2intsize(bitSize);
    uint32_t* bits = (uint32_t*)malloc(intSize * sizeof(uint32_t));
    uint32_t* copy = (uint32_t*)malloc(intSize * sizeof(uint32_t));

    /* countBits
     */
    for (k = 0; k < 100; ++k) {
        a = rand();
        assert(checkBits(a) == base_countBits(a));
    }

    /* countBitsN
     */
    for (k = 0; k < 100; ++k) {
        debug_randomize(bits, intSize);
        assert(checkBitsN(bits, intSize) == base_countBitsN(bits, intSize));
    }

    /* resetBits
     */
    base_resetBits(bits, intSize);
    assert(checkBitsN(bits, intSize) == 0);
    base_resetBits(copy, intSize);
    assert(checkBitsN(copy, intSize) == 0);

    assert(base_areEqual(bits, copy, intSize));
    assert(checkBitsN(bits, intSize) == a);

    free(copy);
    free(bits);
}

int main(int argc, char* argv[])
{
    int i;
    srand(time(NULL));

    for (i = 1; i < 100; ++i) {
        tests(i);
        test_bits2indexTable(i);
    }

    printf("Passed\n");
    return 0;
}
