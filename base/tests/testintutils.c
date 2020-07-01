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
 * Filename : testintutils.c (base/tests)
 *
 * Test intutils.c
 *
 * This file is a part of the UPPAAL toolkit.
 * Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
 * All right reserved.
 *
 * v 1.1 reviewed.
 * $Id: testintutils.c,v 1.5 2004/02/27 16:36:52 adavid Exp $
 *
 **********************************************************************/

/* We are debugging.
 */
#ifdef NDEBUG
#undef NDEBUG
#endif

#include <assert.h>
#include <stdlib.h>
#include <time.h>
#include "base/intutils.h"
#include "debug/utils.h"
#include "debug/macros.h"

/* test memory is == 0
 */
static
BOOL isNull(uint32_t *mem, uint32_t intSize)
{
    while(intSize)
    {
        if (*mem) return FALSE;
        ++mem;
        --intSize;
    }
    return TRUE;
}


/* Test base_sort
 */
static
void test_sort(uint32_t size)
{
    uint32_t *val = (uint32_t*) malloc(size*sizeof(uint32_t));
    uint32_t i,j;

    printf("test_sort(%u)\n", size);

    for(i = 0; i <= size; ++i)
    {
        printf("%u of %u\r", i, size);
        fflush(stdout);
        debug_randomize(val, i);
        base_sort(val, i);
        for(j = 1; j < i; ++j)
        {
            assert(val[j-1] <= val[j]);
        }
    }
    printf("\n");

    free(val);
}

/* Test base_abs
 */
static
void test_abs(int32_t size)
{
    printf("test_abs(%d)\n", size);

    while(size > 0)
    {
        int32_t a = RAND() - RAND();
        assert(base_abs(a) == (a < 0 ? -a : a));
        assert(base_abs(size) == size);
        assert(base_abs(-size) == size);
        --size;
    }
    assert(base_abs(0) == 0);
}


/* Test reset:
 * - alloc
 * - randomize
 * - reset
 * - test
 */
static
void test_reset(uint32_t size)
{
    uint32_t *data = (uint32_t*) malloc((size+2)*sizeof(uint32_t));
    uint32_t magic = (uint32_t) data;

    printf("test_reset(%u)\n", size);

    /* mark before and after: don't corrupt memory
     */
    data[0] = magic;
    data[size+1] = ~magic;

    debug_randomize(data+1, size);
    base_resetSmall(data+1, size);
    assert(isNull(data+1, size));
    assert(data[0] == magic && data[size+1] == ~magic);

    debug_randomize(data+1, size);
    base_resetLarge(data+1, size);
    assert(isNull(data+1, size));
    assert(data[0] == magic && data[size+1] == ~magic);

    free(data);
}


/* Test copy and equal:
 * - alloc
 * - randomize
 * - copy
 * - test
 */
static
void test_copy(uint32_t size)
{
    uint32_t *src = (uint32_t*) malloc(size*sizeof(uint32_t));
    uint32_t *dst = (uint32_t*) malloc((size+2)*sizeof(uint32_t));
    uint32_t magic = (uint32_t) src;
    uint32_t i;

    printf("test_copy(%u)\n", size);

    /* mark before and after: don't corrupt memory
     */
    dst[0] = magic;
    dst[size+1] = ~magic;

    debug_randomize(src, size);
    base_copySmall(dst+1, src, size);
    assert(dst[0] == magic && dst[size+1] == ~magic);
    for(i = 0; i < size; ++i)
    {
        assert(dst[i+1] == src[i]);
    }

    for(i = 0 ; i < size; ++i)
    {
        assert(base_areEqual(dst+1, src, size));
        dst[i+1] = ~dst[i+1];
        assert(!base_areEqual(dst+1, src, size));
        dst[i+1] = ~dst[i+1];
    }

    debug_randomize(src, size);
    base_copyLarge(dst+1, src, size);
    assert(dst[0] == magic && dst[size+1] == ~magic);
    for(i = 0; i < size; ++i)
    {
        assert(dst[i+1] == src[i]);
    }

    free(dst);
    free(src);
}


int main(int argc, char *argv[])
{
    uint32_t size;
    srand(time(NULL));

    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s size\n", argv[0]);
        return 1;
    }
    size = atoi(argv[1]);

    test_reset(size);
    test_copy(size); /* and equal */
    test_abs((int32_t) size);
    test_sort(size);

    printf("Passed\n");
    return 0;
}


