/* -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/*********************************************************************
 *
 * Filename : test_compute.c (hash/tests)
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

#ifdef NDEBUG
#undef NDEBUG
#endif

#include "hash/compute.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

#include <vector>
#include <cstdlib>

static const char teststr[] = "Dummy test string.... not too short, not too long.. well hopefully!";

static void test(int i)
{
    /* no stack allocation to avoid potential stack overflow
     */
    auto dat32 = std::vector<int32_t>(i);
    auto dat16 = std::vector<int16_t>(i);
    auto dat8 = std::vector<int8_t>(i);
    int j;
    uint32_t h32, hh32, h16, hh16, h8, hh8;

    for (j = 0; j < i; ++j) {
        dat32[j] = rand();
        dat16[j] = rand();
        dat8[j] = rand();
    }

    h32 = h16 = h8 = 0;
    for (j = 1; j < i; ++j) {
        hh32 = hash_computeI32(dat32.data(), j, 0);
        hh16 = hash_computeI16(dat16.data(), j, 0);
        hh8 = hash_computeI8(dat8.data(), j, 0);
        CHECK(h32 != hh32);
        CHECK(h16 != hh16);
        CHECK(h8 != hh8);
        h32 = hh32;
        h16 = hh16;
        h8 = hh8;
    }
}

TEST_CASE("hash")
{
    int i;
    int len = strlen(teststr);
    uint32_t h1, h2, h3, h4;

    for (i = 0; i <= len; ++i) {
        h1 = hash_computeI8((int8_t*)teststr, i, 0);
        h2 = hash_computeI8((int8_t*)teststr, i, 1);
        CHECK(h1 != h2);
        // printf("0x%x != 0x%x\n",h1,h2);
    }
    h3 = hash_computeStr(teststr, 0);
    h4 = hash_computeStr(teststr, 1);
    CHECK(h1 == h3);
    CHECK(h2 == h4);

    printf("Testing...\n");
    for (i = 0; i < 1024; ++i)
        test(i);
}
