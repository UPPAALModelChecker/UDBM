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

#include "base/DataAllocator.h"
#include "debug/new.h"

#include <iostream>
#include <cstdlib>

using namespace std;
using namespace base;

int main(int argc, char* argv[])
{
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " type\n";
        exit(1);
    }

    int32_t crash = atoi(argv[1]);

    if (crash < 0 || crash > 4) {
        cerr << "Invalid crash type, must be 0..4\n";
        exit(1);
    }

    for (uint32_t i = 0; i < 20; ++i) {
        cout << "Loop " << i << endl;

        DataAllocator_ptr alloc = std::make_shared<DataAllocator>();

        void* data = alloc->allocate(100);
        alloc->deallocate(data, 100);  // good

        switch (crash) {
        case 0:
            data = alloc->allocate(100);
            alloc->deallocate(data, 99);  // bad
            break;
        case 1:
            data = alloc->allocate(100);
            alloc->deallocate(data, 101);  // very bad
            break;
        case 2:
            alloc->deallocate(data, 100);  // already deallocated
            break;
        case 3:
            data = alloc->allocate(100);
            alloc->reset();
            alloc->deallocate(data, 100);  // already deallocated
            break;
        default: cout << "No crash asked :(\n";
        }
    }
    return 0;
}
