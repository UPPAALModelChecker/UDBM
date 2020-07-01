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
// Filename : testitemtables.cpp (hash/tests)
//
// Test of ItemTableSingle.
//
// This file is a part of the UPPAAL toolkit.
// Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
// All right reserved.
//
// $Id: #
//
///////////////////////////////////////////////////////////////////

// Tests are always for debugging.

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <time.h>
#include <iostream>
#include "hash/itemtables.h"
#include "base/DataAllocator.h"
#include "debug/utils.h"

using namespace std;
using namespace base;
using namespace hash;

// spinning bar
#define PROGRESS() debug_spin(stderr)

static
void test(int size)
{
    DataAllocator alloc;
    for(int k = 0; k < 20; ++k)
    {
        PROGRESS();
        ItemTableSingle<int> tableN(4, rand()%2 == 0);
        ItemTableSingle<int> tableA(4, rand()%2 == 0);
        for(int i = 0; i < size; ++i)
        {
            
            assert(!tableN.hasItem(i, (uint32_t) ~i)); // ~i = hash
            assert(!tableA.hasItem(i, (uint32_t) ~i));
            tableN.addNewItem(i, (uint32_t) ~i);
            tableA.addItem<DataAllocator>(i, (uint32_t) ~i, &alloc);
            assert(tableN.hasItem(i, (uint32_t) ~i));
            assert(tableA.hasItem(i, (uint32_t) ~i));
        }
        alloc.reset();
        tableN.resetDelete();
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

    /* Print the seed for the random generator
     * to be able to repeat a failed test.
     */
    cout << "Test with seed=" << seed << endl;

    test(n);

    cout << "Passed\n";
    return 0;
}
