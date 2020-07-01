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
// Filename : testobject.cpp (base/tests)
//
// This is a template file for tests.
//
// This file is a part of the UPPAAL toolkit.
// Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
// All right reserved.
//
// $Id: testobject.cpp,v 1.3 2004/02/27 16:36:52 adavid Exp $
//
///////////////////////////////////////////////////////////////////

// Tests are always for debugging.

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <time.h>
#include "base/Array.h"
#include "debug/new.h"

using namespace std;
using namespace base;

/** Basic class with an allocation
 * and deallocation + do something.
 */
class Test : public Object
{
public:
    Test() : mem(new int) { *mem = 0; }
    ~Test() { delete mem; }
    int get()  { return *mem; }
    void inc() { (*mem)++; }
private:
    int *mem;
};

/** Test operators and new.h will
 * check for leaks
 */
static
void test()
{
    Pointer<Test> a = new Test;
    Pointer<Test> b(new Test);
    Pointer<Test> c(b);
    assert((*a).get() == 0);
    b->inc();
    a = b;
    assert(a->get() == 1);
    assert(c->get() == 1);
}

static
void testArray()
{
    for(size_t size = 0; size < 100; ++size)
    {
        Pointer<Array<size_t> > a = Array<size_t>::create(size)->zero();
        assert(a->size() == size);
        Pointer<Array<size_t> > b = a;
        a.setMutable();
        for(size_t i = 0; i < size; ++i) assert((*b)[i] == 0);
        for(size_t* i = b->begin(); i != b->end(); ++i) *i = rand();
        for(size_t i = 0; i < size; ++i) assert((*a)[i] == 0);
        for(size_t i = 0; i < size; ++i) (*a)[i] = i;
        for(size_t i = 0; i < size; ++i) assert((*a)[i] == i);
        a.ensure(2*size);
        assert(a->size() >= 2*size);
        for(size_t* i = a->begin(); i != a->end(); ++i) *i = size;
        for(const size_t *i = a->begin(); i != a->end(); ++i) assert(*i == size);
        for(size_t i = 0; i < 2*size; ++i) assert((*a)[i] == size);
    }
}

int main(int argc, char *argv[])
{
    int seed;

    seed = argc > 1 ? atoi(argv[1]) : time(NULL);
    srand(seed);

    /* Print the seed for the random generator
     * to be able to repeat a failed test.
     */
    cout << "Test with seed=" << seed << endl;
    test();
    testArray();
    cout << "Passed\n";
    return 0;
}
