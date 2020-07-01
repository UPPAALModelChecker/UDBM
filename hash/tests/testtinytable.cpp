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
// Filename : testtinytable.cpp
//
// Test of TinyTable.
//
// This file is a part of the UPPAAL toolkit.
// Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
// All right reserved.
//
// $Id: testtinytable.cpp,v 1.1 2005/04/22 15:20:09 adavid Exp $
//
///////////////////////////////////////////////////////////////////

// Tests are always for debugging.

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <stdlib.h>
#include <time.h>
#include <iostream>
#include "debug/utils.h"
#include "hash/TinyTable.h"

using namespace std;
using namespace hash;

void test(int size)
{
    size_t tableSize = TinyTable::intsizeFor(size);
    uint32_t table[tableSize];
    TinyTable ttable(table, tableSize);

    uint32_t items[size];
    debug_randomize(items, size);

    // ensure unique & !=0 elements
    for(int i = 0; i < size; ++i)
    {
        items[i] = (items[i] & 0xffff) | ((i+1) << 16);
    }

    // new
    for(int i = 0; i < size; ++i)
    {
        assert(ttable.add(items[i]));
    }

    // not new
    for(int i = 0; i < size; ++i)
    {
        assert(!ttable.add(items[i]));
    }

    // reset
    base_resetSmall(table, tableSize);

    // new
    for(int i = 0; i < size; ++i)
    {
        assert(ttable.add(i+1));
    }

    // not new
    for(int i = 0; i < size; ++i)
    {
        assert(!ttable.add(i+1));
    }
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

    if (n > 0xffff)
    {
        cerr << "Test supports only size <= " << 0xffff << endl;
        return 1;
    }

    /* Print the seed for the random generator
     * to be able to repeat a failed test.
     */
    cout << "Test with seed=" << seed << endl;

    for(int i = 1 ; i < n ; ++i) test(i);

    cout << "Passed\n";
    return 0;
}
