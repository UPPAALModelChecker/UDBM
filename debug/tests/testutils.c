/* -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; -*-
 *
 * This file is part of the UPPAAL DBM library.
 *
 * The UPPAAL DBM library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * The UPPAAL DBM library is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with the UPPAAL DBM library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA.
 */

/* -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/*********************************************************************
 *
 * Filename : testutils.c (debug/tests)
 *
 * Test basic debug functions + debug macros.
 *
 * This file is a part of the UPPAAL toolkit.
 * Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
 * All right reserved.
 *
 * v 1.3 reviewed.
 * $Id: testutils.c,v 1.9 2005/05/11 19:08:14 adavid Exp $
 *
 **********************************************************************/

/* We are debugging.
 */
#ifdef NDEBUG
#undef NDEBUG
#endif

#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include "base/intutils.h"
#include "base/bitstring.h"
#include "debug/macros.h"

/** Dummy print for testing.
 */
static
void dummy()
{
    printf("Test assertion failed (good)!\nPassed\n");
}

/** Test basename function
 */
static
void test_basename()
{
    assert(strcmp(debug_shortSource("foo/fii/hop.c"), "fii/hop.c") == 0);
    assert(strcmp(debug_shortSource("/foo/fii/hop.c"), "fii/hop.c") == 0);
    assert(strcmp(debug_shortSource("/foo/fii/"), "fii/") == 0);
    assert(strcmp(debug_shortSource("foo/fii"), "foo/fii") == 0);
    assert(strcmp(debug_shortSource("foo"), "foo") == 0);
    assert(strcmp(debug_shortSource("tests/foo"), "tests/foo") == 0);
    assert(strcmp(debug_shortSource("mod/tests/foo"), "mod/tests/foo") == 0);
    assert(strcmp(debug_shortSource("/mod/tests/foo"), "mod/tests/foo") == 0);
    assert(strcmp(debug_shortSource("path/mod/tests/foo"), "mod/tests/foo") == 0);
}

/** Test debug/macros.h
 */
static
void test_macros()
{
    PRINT_CINFO(GREEN(BOLD), "entering");
    ASSERT(1,dummy()); /* never fails */
    ASSERT(time(NULL) & 1, dummy()); /* will fail randomly */
    DODEBUG(printf("dummy print\n"));
    PRINT_CINFO(GREEN(BOLD), "exiting");
}

/** Test randomize function
 */
static
void test_randomize(uint32_t maxSize)
{
    uint32_t size;
    for (size = 0; size <= maxSize; ++size)
    {
        int32_t *data1 = (int32_t*) malloc(size*sizeof(int32_t));
        int32_t *data2 = (int32_t*) malloc(size*sizeof(int32_t));
        uint32_t i;
        /* reset to be sure randomize has some effect */
        for (i = 0; i < size; ++i)
        {
            data1[i] = 0;
            data2[i] = 0;
        }
        debug_randomize(data1, size);
        debug_randomize(data2, size);
        for (i = 0; i < size; ++i)
        {
            assert(data1[i] != data2[i]);
        }
        free(data2);
        free(data1);
    }
}

/** Test tab
 */
static
void test_tab()
{
    uint32_t i;
    for(i = 0; i < 10; ++i)
    {
        debug_tab(stdout, i);
        printf("tab(%u)\n",i);
    }
}

/** Test print bits.
 */
static
void test_printBitstring(uint32_t maxSize)
{
    uint32_t n;
    maxSize >>= 2;
    for(n = 0; n <= maxSize; ++n)
    {
        uint32_t *data = (uint32_t*) malloc(n*sizeof(uint32_t));
        uint32_t i;
        debug_randomize(data,n);
        for(i = 0; i < n; ++i)
            printf("0x%x ", data[i]);
        printf("\n");
        debug_printBitstring(stdout, data, n);
        printf("\n");
    }
}

/** Test bit generation
 */
static
void test_generateBits(uint32_t size)
{
    uint32_t stringSize = bits2intsize(size);
    uint32_t bitString[stringSize];
    uint32_t i,max;
    debug_generateBits(bitString, stringSize, size, ZERO);
    assert(base_countBitsN(bitString, stringSize) == size);

    max = size + rand()%10;
    debug_fixGeneratedBits(bitString, stringSize, max);
    assert(base_countBitsN(bitString, stringSize) == size);
    for(i = max; i < stringSize*32; ++i)
    {
        assert(base_getOneBit(bitString, i) == 0);
    }
}


int main(int argc, char *argv[])
{
    uint32_t size, i;
    srand(time(NULL));

    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s size\n", argv[0]);
        return 1;
    }
    size = atoi(argv[1]);

    test_basename();
    test_macros();
    test_tab();
    test_printBitstring(size);
    for(i = 0; i <= size; ++i)
    {
        test_randomize(size);
        test_generateBits(size);
    }

    printf("Passed\n");
    return 0;
}


