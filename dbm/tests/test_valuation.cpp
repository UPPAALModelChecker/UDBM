// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-
////////////////////////////////////////////////////////////////////
//
// Filename : testvaluation.cpp
//
// Basic tests of IntValuation and DoubleValuation
//
// This file is a part of the UPPAAL toolkit.
// Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
// All right reserved.
//
// $Id: testvaluation.cpp,v 1.1 2005/04/22 15:20:09 adavid Exp $
//
///////////////////////////////////////////////////////////////////

// Tests are always for debugging.

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <time.h>
#include <iostream>
#include "debug/macros.h"
#include "dbm/Valuation.h"

using namespace std;
using namespace dbm;

static void test(int size)
{
    IntValuation iv(size);
    DoubleValuation dv(size);

    for (int i = 0; i < size; ++i) {
        ASSERT(iv[i] == 0, cerr << iv[i] << endl);
        ASSERT(dv[i] == 0.0, cerr << dv[i] << endl);
    }

    (iv += 3) -= 2;
    (dv += 3.1) -= 2;

    if (size > 0) {
        assert(iv[0] == 0.0);
        assert(dv[0] == 0.0);
    }

    for (int i = 1; i < size; ++i) {
        ASSERT(iv[i] == 1, cerr << iv[i] << endl);
        ASSERT(dv[i] == 1.1, cerr << dv[i] << endl);
    }

    cout << iv << endl << dv << endl;
}

int main(int argc, char* argv[])
{
    int n, seed;

    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " size [seed]\n";
        return 1;
    }

    n = atoi(argv[1]);
    seed = argc > 2 ? atoi(argv[2]) : time(NULL);
    srand(seed);

    /* Print the seed for the random generator
     * to be able to repeat a failed test.
     */
    cout << "Test with seed=" << seed << endl;

    for (int i = 0; i <= n; ++i)
        test(i);

    cout << "Passed\n";
    return 0;
}
