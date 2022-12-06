// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-
////////////////////////////////////////////////////////////////////
//
// Unit tests for IntValuation and DoubleValuation
//
// This file is a part of the UPPAAL toolkit.
// Copyright (c) 2011 - 2022, Aalborg University.
// Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
// All rights reserved.
//
///////////////////////////////////////////////////////////////////

#include "dbm/valuation.h"

#include <iostream>

#include <doctest/doctest.h>

using namespace std;
using namespace dbm;

static void test(size_t size)
{
    auto iv = valuation_int{size};
    auto dv = valuation_fp{size};

    for (size_t i = 0; i < size; ++i) {
        CHECK(iv[i] == 0);
        CHECK(dv[i] == 0.0);
    }

    (iv += 3) -= 2;
    (dv += 3.1) -= 2;

    if (size > 0) {
        CHECK(iv[0] == 0.0);
        CHECK(dv[0] == 0.0);
    }

    for (size_t i = 1; i < size; ++i) {
        CHECK(iv[i] == 1);
        CHECK(dv[i] == 1.1);
    }

    cout << iv << endl << dv << endl;
}

TEST_CASE("Valuation")
{
    for (size_t i = 0; i <= 20; ++i)
        test(i);
}
