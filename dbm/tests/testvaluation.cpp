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

void test(int size)
{
    IntValuation iv(size);
    DoubleValuation dv(size);

    for(int i = 0; i < size; ++i)
    {
        ASSERT(iv[i] == 0, cerr << iv[i] << endl);
        ASSERT(dv[i] == 0.0, cerr << dv[i] << endl);
    }

    (iv += 3) -= 2;
    (dv += 3.1) -= 2;

    if (size > 0)
    {
        assert(iv[0] == 0.0);
        assert(dv[0] == 0.0);
    }

    for(int i = 1; i < size; ++i)
    {
        ASSERT(iv[i] == 1, cerr << iv[i] << endl);
        ASSERT(dv[i] == 1.1, cerr << dv[i] << endl);
    }

    cout << iv << endl << dv << endl;
}

int main(int argc, char *argv[])
{
    int n, seed;

    if (argc < 2)
    {
        cerr << "Usage: " << argv[0]
             << " size [seed]\n";
        return 1;
    }

    n = atoi(argv[1]);
    seed = argc > 2 ? atoi(argv[2]) : time(NULL);
    srand(seed);

    /* Print the seed for the random generator
     * to be able to repeat a failed test.
     */
    cout << "Test with seed=" << seed << endl;

    for(int i = 0 ; i <= n ; ++i)
        test(i);

    cout << "Passed\n";
    return 0;
}
