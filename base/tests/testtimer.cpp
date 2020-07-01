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
// Filename : testtimer.cpp (base/tests)
//
// Test Timer (and base_getTime).
//
// This file is a part of the UPPAAL toolkit.
// Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
// All right reserved.
//
// v 1.2 reviewed.
// $Id: testtimer.cpp,v 1.5 2004/02/27 16:36:52 adavid Exp $
//
///////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include "base/Timer.h"

using namespace std;
using namespace base;

static
int test(int size)
{
    // do something very clever
    int result = 0;
    size *= 10000;
    for(int i = 0 ; i < size ; ++i)
        result += rand();
    return result;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        cerr << "Usage: " << argv[0]
             << " size\n";
        return 1;
    }

    int n = atoi(argv[1]);
    double total = 0;
    Timer t1;
    int dummy = 0;

    for(int i = 0 ; i < n ; ++i)
    {
        Timer t1, t2;
        dummy += test(i);
        double elapsed = t2.getElapsed();
        total += elapsed;
        cout << "Test: " << t1 << " >= " << elapsed << "s\n";
    }

    cout << "Passed in " << t1 << " >= "
         << total << "s\n";

    return 0;
}
