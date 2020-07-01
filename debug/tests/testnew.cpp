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
// Filename : testnew.cpp
//
// Test new/delete monitoring.
//
// This file is a part of the UPPAAL toolkit.
// Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
// All right reserved.
//
// $Id: testnew.cpp,v 1.2 2004/02/27 16:36:52 adavid Exp $
//
///////////////////////////////////////////////////////////////////

#ifdef NDEBUG
#undef NDEBUG
#endif

// Should be tested with and without -DNO_NEW_MONITOR

#include <iostream>
#include <stdlib.h>
#include <new>

//#define NNEW_INFO
//#define NDELETE_INFO

#include "debug/macros.h"
#include "debug/new.h"

class CFoo
{
public:
    CFoo() :foo(0)
        { std::cout << "\tCFoo - constructor only\n"; }
    int foo;
};

class DFoo
{
public:
    DFoo() : foo(new int)
        { std::cout << "\t\tDFoo - constructor\n"; }
    ~DFoo() {
        delete foo;
        std::cout << "\t\tDFoo - destructor\n";
    }
    int *foo;
};

static
void testok()
{
    PRINT_INFO("");
    int *a = new int;
    int *b = new int[5];
    CFoo *c = new CFoo;
    CFoo *d = new CFoo[2];
    DFoo *e = new DFoo;
    DFoo *f = new DFoo[3];

#ifdef NNEW_INFO
    DFoo *g = new (b) DFoo[2];
#endif

    delete a;
    delete [] b;
    delete c;
    delete [] d;
    delete e;
    delete [] f;

#ifdef NNEW_INFO
    g[0].~DFoo();
    g[1].~DFoo();
#endif
}

static
void testleak()
{
    PRINT_INFO("");
    int *a = new int;
    int *b = new int[5];
    CFoo *c = new CFoo;
    CFoo *d = new CFoo[2];
    DFoo *e = new DFoo;
    DFoo *f = new DFoo[3];

#ifdef NNEW_INFO
    DFoo *g = new (b) DFoo[2];
#endif

    if (rand() & 1) { delete a; }
    if (rand() & 1) { delete [] b; }
    if (rand() & 1) { delete c; }
    if (rand() & 1) { delete [] d; }
    if (rand() & 1) { delete e; }
    if (rand() & 1) { delete [] f; }
    
#ifdef NNEW_INFO
    g[0].~DFoo();
    g[1].~DFoo();
#endif
}

static
void testcrash()
{
    PRINT_INFO("");
    int *a = new int;
    int *b = new int[5];
    CFoo *c = new CFoo;
    CFoo *d = new CFoo[2];
    DFoo *e = new DFoo;
    DFoo *f = new DFoo[3];

#ifdef NNEW_INFO
    DFoo *g = new (b) DFoo[2];
#endif

    if (rand() & 1) { delete (a + (rand() & 1)); }
    if (rand() & 1) { delete [] (b + (rand() & 1)); }
    if (rand() & 1) { delete (c + (rand() & 1)); }
    if (rand() & 1) { delete [] (d + (rand() & 1)); }
    if (rand() & 1) { delete (e + (rand() & 1)); }
    if (rand() & 1) { delete [] (f + (rand() & 1)); }

#ifdef NNEW_INFO
    g[0].~DFoo();
    g[1].~DFoo();
#endif
}

#define SUFFER_SIZE 1000000

static
void testsuffer()
{
    PRINT_INFO("");
    int **table = new int*[SUFFER_SIZE];
    int i;
    std::cout << "Allocating...\n";
    for(i = 0 ; i < SUFFER_SIZE ; ++i)
        table[i] = new int[rand()%10];
    std::cout << "Deallocating...\n";
    for(i = 0 ; i < SUFFER_SIZE ; ++i)
    {
        delete [] table[i];
    }
    delete [] table;
}

int main(int argc, char *argv[])
{
    int i;
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0]
                  << " 0..3\n";
        return 1;
    }

    switch(atoi(argv[1]))
    {
    case 3:  for(i = 0 ; i < 3; ++i) testsuffer(); break;
    case 2:  for(i = 0 ; i < 3; ++i) testcrash(); break;
    case 1:  for(i = 0 ; i < 3; ++i) testleak(); break;
    default: for(i = 0; i < 3; ++i) testok(); break;
    }

    return 0;
}
