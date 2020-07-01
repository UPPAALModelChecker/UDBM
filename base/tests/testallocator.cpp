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
// Filename : testallocator.cpp (base/tests)
//
// Test for DataAllocator
//
// This file is a part of the UPPAAL toolkit.
// Copyright (c) 1995 - 2000, Uppsala University and Aalborg University.
// All right reserved.
//
// $Id: testallocator.cpp,v 1.3 2004/02/27 16:36:52 adavid Exp $
//
////////////////////////////////////////////////////////////////////

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <iostream>
#include <stdlib.h>
#include <time.h>
#include "base/DataAllocator.h"
#include "debug/new.h"

using namespace std;
using namespace base;

// keep track of allocated memory
struct Cell
{
    uint32_t size;
    uint32_t *data;
};


static
void test(uint32_t size)
{
    DataAllocator alloc;
    Cell *cells = new Cell[size];

    // allocate
    uint32_t i;
    for(i = 0 ; i < size ; ++i)
    {
        cells[i].size = 30*(rand()%30)+8;
        cells[i].data = (uint32_t*) alloc.allocate(cells[i].size);

        // write
        for(uint32_t j = 0 ; j < cells[i].size ; ++j)
        {
            cells[i].data[j] = rand();
        }

        // sometimes deallocate
        if (i > 0 && rand()%5 == 0)
        {
            int index = rand()%i;
            if (cells[index].data)
            {
                alloc.deallocate(cells[index].data, cells[index].size);
                cells[index].data = 0;
            }
        }
    }

    alloc.printStats(stdout);
    
    // deallocate
    for(i = 0 ; i < size ; ++i)
    {
        if (cells[i].data)
        {
            alloc.deallocate(cells[i].data, cells[i].size);
        }
    }

    delete [] cells;
}


int main(int argc, char *argv[])
{
    uint32_t n, seed, i;

    if (argc < 2)
    {
        cerr << "Usage: " << argv[0]
             << " size [seed]\n";
        return 1;
    }

    n = atoi(argv[1]);
    seed = argc > 2 ? atoi(argv[2]) : time(NULL);
    cout << "Testing with seed=" << seed << endl;

    for(i = 0 ; i < 10 ; ++i)
    {
        test(n);
    }

    std::cerr << "Passed\n";
    return 0;
}

