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
 * Filename : testcompute.c (hash/tests)
 *
 * Test compute.h
 *
 * This file is a part of the UPPAAL toolkit.
 * Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
 * All right reserved.
 *
 * Not reviewed.
 * $Id: testcompute.c,v 1.4 2004/02/27 16:36:53 adavid Exp $
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "hash/compute.h"

static
const char teststr[] = "Dummy test string.... not too short, not too long.. well hopefully!";

#ifdef NDEBUG
#undef NDEBUG
#endif

static
void test(int i)
{
    /* no stack allocation to avoid potential stack overflow
     */
    int32_t *dat32 = (int32_t*)malloc(i*sizeof(int32_t));
    int16_t *dat16 = (int16_t*)malloc(i*sizeof(int16_t));
    int8_t  *dat8  = (int8_t*) malloc(i*sizeof(int8_t));
    int j;
    uint32_t h32, hh32, h16, hh16, h8, hh8;
    
    for(j = 0 ; j < i ; ++j)
    {
        dat32[j] = rand();
        dat16[j] = rand();
        dat8[j]  = rand();
    }

    h32 = h16 = h8 = 0;
    for(j = 1 ; j < i ; ++j)
    {
        hh32 = hash_computeI32(dat32, j, 0);
        hh16 = hash_computeI16(dat16, j, 0);
        hh8 = hash_computeI8(dat8, j, 0);
        assert(h32 != hh32);
        assert(h16 != hh16);
        assert(h8 != hh8);
        h32 = hh32;
        h16 = hh16;
        h8 = hh8;
    }

    free(dat8);
    free(dat16);
    free(dat32);
}

int main(int argc, char **argv)
{
    int i;
    int len = strlen(teststr);
    uint32_t h1,h2,h3,h4;

    for(i = 0 ; i <= len ; ++i)
    {
        h1 = hash_computeI8((int8_t*)teststr, i, 0);
        h2 = hash_computeI8((int8_t*)teststr, i, 1);
        assert(h1 != h2);
        printf("0x%x  != 0x%x\n",h1,h2);
    }
    h3 = hash_computeStr(teststr, 0);
    h4 = hash_computeStr(teststr, 1);
    assert(h1 == h3);
    assert(h2 == h4);

    printf("Testing...\n");
    for(i = 0 ; i < 1024 ; ++i)
        test(i);

    printf("Passed\n");
    return 0;
}





