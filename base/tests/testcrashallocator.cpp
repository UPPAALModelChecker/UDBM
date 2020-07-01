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
// Filename : testcrashallocator.cpp
//
// Test for DataAllocator ; test debugging features when a data
// allocator is badly used.
//
// This file is a part of the UPPAAL toolkit.
// Copyright (c) 1995 - 2000, Uppsala University and Aalborg University.
// All right reserved.
//
// $Id: testcrashallocator.cpp,v 1.2 2003/12/19 14:49:33 adavid Exp $
//
////////////////////////////////////////////////////////////////////

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <iostream>
#include <stdlib.h>
#include "base/DataAllocator.h"
#include "debug/new.h"

using namespace std;
using namespace base;

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        cerr << "Usage: " << argv[0] << " type\n";
        exit(1);
    }

    int32_t crash = atoi(argv[1]);

    if (crash < 0 || crash > 4)
    {
        cerr << "Invalid crash type, must be 0..4\n";
        exit(1);
    }

    for(uint32_t i = 0 ; i < 20 ; ++i)
    {
        cout << "Loop " << i << endl;

        DataAllocator *alloc = new DataAllocator;

        void *data = alloc->allocate(100);
        alloc->deallocate(data, 100); // good
        
        switch(crash)
        {
        case 0:
            data = alloc->allocate(100);
            alloc->deallocate(data, 99); // bad
            break;
        case 1:
            data = alloc->allocate(100);
            alloc->deallocate(data, 101); // very bad
            break;
        case 2:
            alloc->deallocate(data, 100); // already deallocated
            break;
        case 3:
            data = alloc->allocate(100);
            alloc->reset();
            alloc->deallocate(data, 100); // already deallocated
            break;
        default:
            cout << "No crash asked :(\n";
        }

        delete alloc;
    }

    return 0;
}

