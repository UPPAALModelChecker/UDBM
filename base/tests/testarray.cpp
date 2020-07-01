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
// Filename : testarray.cpp (base/tests)
//
// Test random access array_t
//
// This file is a part of the UPPAAL toolkit.
// Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
// All right reserved.
//
// $Id: testarray.cpp,v 1.2 2004/02/27 16:36:52 adavid Exp $
//
///////////////////////////////////////////////////////////////////

#include <iostream>
#include <stdlib.h>
#include <time.h>
#include "base/array_t.h"
#include "debug/new.h"

using namespace std;

class Foo
{
public:
    Foo(int a) : fii(a) {}
    int fii;
};

static
void test(uint32_t size)
{
    base::array_t<uint32_t> arr(rand()%(size+1));
    uint32_t i;
    size <<= 1;
    // random write
    for(i = 1 ; i < size ; ++i)
    {
        uint32_t idx = rand()%(size+1);
        arr.set(idx,idx);
    }
    // print
    if (size < 10)
    {
        std::cout << arr << std::endl;
    }
    // read all
    for(i = 0 ; i < size ; ++i)
    {
        assert(arr.get(i) == 0 || arr.get(i) == i);
    }
    // test []
    size = arr.size();
    for(i = 0 ; i < size ; ++i)
    {
        arr[i]++;
        assert(arr[i] == 1 || arr[i] == i + 1);
    }
}

static
void testFoo(int crash)
{
    Foo foo(4);
    base::pointer_t<Foo> p(crash > 1 ? NULL : &foo, 1);
    int n = crash == 1 ? 2 : 1;
    int i;
    if (crash == 2) p->fii++;
    for(i = 0 ; i < n ; ++i)
    {
        p[i].fii++;
    }
    (*p).fii++;
    cout << p->fii << endl;
}

int main(int argc, char *argv[])
{
    uint32_t seed, n, i;

    if (argc < 3)
    {
        cerr << "Usage: " << argv[0]
             << " size crash(0..3) [seed]\n";
        return 1;
    }

    n = atoi(argv[1]);
    seed = argc > 3 ? atoi(argv[3]) : time(NULL);
    cout << "Testing with seed=" << seed << endl;
    srand(seed);

    for(i = 0 ; i < n ; ++i)
    {
        test(i);
    }
    testFoo(atoi(argv[2]));

    cout << "Passed\n";
    return 0;
}
