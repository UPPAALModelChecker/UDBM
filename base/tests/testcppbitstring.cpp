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
// Filename : test.cpp
//
// This is a template file for tests.
//
// This file is a part of the UPPAAL toolkit.
// Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
// All right reserved.
//
// $Id: testcppbitstring.cpp,v 1.1 2005/04/22 15:20:08 adavid Exp $
//
///////////////////////////////////////////////////////////////////

// Tests are always for debugging.

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <time.h>
#include <stdlib.h>
#include <iostream>
#include "base/bitstring.h"
#include "debug/macros.h"

using namespace std;
using namespace base;

void test(int size)
{
    BitString b1(size), b2(size), b3(b2);
    assert(b1.size() == b2.size() && b2.size() == b3.size());
    size = b1.size(); // rounded up
    assert(b1.count() == 0 && b2.count() == 0 && b3.count() == 0);
    assert(b1.isEmpty() && b2.isEmpty() && b3.isEmpty());
    for(int i = 0; i < size; ++i)
    {
        b1[i] |= Bit::One;
        b2 += i;
    }
    b3 = Bit::One;
    assert(b1.count() == (size_t) size &&
           b2.count() == (size_t) size &&
           b3.count() == (size_t) size);
    ASSERT(b1 == b2,
           debug_cppPrintDiffBitstrings(cerr, b1(), b2(),
                                        b1.intSize()));
    ASSERT(b2 == b3,
           debug_cppPrintDiffBitstrings(cerr, b2(), b3(),
                                        b2.intSize()));
    assert(size == 0 || (!b1.isEmpty() &&
                         !b2.isEmpty() && !b3.isEmpty()));
    for(int i = 0; i < size; ++i)
    {
        b1[i] ^= Bit::One;
        b2 ^= i;
    }
    b3 = Bit::Zero;
    assert(b1.isEmpty() && b1 == b2 && b2 == b3);
    b1 = Bit::One;
    b2 = b1;
    b3 |= b1;
    b3 &= b1;
    for(int i = 0; i < size; ++i)
    {
        b1 -= i;
    }
    assert(b1.isEmpty() && b2 == b3);
    b2 ^= b3;
    assert(b2.isEmpty());
    ASSERT(b3.neg().isEmpty(), cerr << b3 << endl);
    for(int i = 0; i < size; ++i)
    {
        bit_t b = (rand() & 1) ? ONE : ZERO;
        b1[i] = b;
        b2.assignBit(i, b);
        b3.addBit(i, b);
    }
    assert(b1 == b2 && b2 == b3);
    for(int i = 0; i < size; ++i)
    {
        b1[i] ^= b2[i];
        b2.xorBit(i, b3[i]);
        b3.subBit(i, b3.getBit(i));
    }
    assert(b1.isEmpty() && b1 == b2 && b2 == b3);
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

    for(int i = 0 ; i < n ; ++i)
        test(i);

    cout << "Passed\n";
    return 0;
}

