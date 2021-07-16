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

#include "base/intutils.h"
#include "debug/utils.h"

#include <assert.h>
#include <stdlib.h>
#include <time.h>

/* test memory is == 0
 */
static bool isNull(int32_t* mem, uint32_t intSize)
{
    while (intSize) {
        if (*mem)
            return false;
        ++mem;
        --intSize;
    }
    return true;
}

/* Test reset:
 * - alloc
 * - randomize
 * - reset
 * - test
 */
static void test_reset(uint32_t size)
{
    int32_t* data = (int32_t*)malloc((size + 2) * sizeof(int32_t));
    int32_t magic = (int32_t)((uintptr_t)data);

    printf("test_reset(%u)\n", size);

    /* mark before and after: don't corrupt memory
     */
    data[0] = magic;
    data[size + 1] = ~magic;

    debug_randomize(data + 1, size);
    base_fill(data + 1, data + 1 + size, 0);
    assert(isNull(data + 1, size));
    assert(data[0] == magic && data[size + 1] == ~magic);

    debug_randomize(data + 1, size);
    base_fill(data + 1, data + 1 + size, 0);
    assert(isNull(data + 1, size));
    assert(data[0] == magic && data[size + 1] == ~magic);

    free(data);
}

int main(int argc, char* argv[])
{
    uint32_t size;
    srand(time(NULL));

    if (argc < 2) {
        fprintf(stderr, "Usage: %s size\n", argv[0]);
        return 1;
    }
    size = atoi(argv[1]);

    test_reset(size);

    printf("Passed\n");
    return 0;
}
