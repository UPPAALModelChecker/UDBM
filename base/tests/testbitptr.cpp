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
// Filename : testbitptr.cpp (base/tests)
//
// Test bitptr.h
//
// This file is a part of the UPPAAL toolkit.
// Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
// All right reserved.
//
// $Id: testbitptr.cpp,v 1.2 2004/02/27 16:36:52 adavid Exp $
//
///////////////////////////////////////////////////////////////////

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <iostream>
#include "base/bitptr.h"

using namespace std;
using namespace base;

static
void test(uint32_t i)
{
    uint32_t m = i&3;
    uint32_t n = m ^ 3;
    bitptr_t<uint32_t> a;
    bitptr_t<uint32_t> b(&m);
    bitptr_t<uint32_t> c(m);
    bitptr_t<uint32_t> d(&m, m);
    bitptr_t<uint32_t> e(d, m);
    bitptr_t<uint32_t> f(&m, e);
    
    assert(a.getBits() == 0);
    assert(b.getBits() == 0);
    assert(c.getBits() == m);
    assert(d.getBits() == m);
    assert(e.getBits() == m);
    assert(f.getBits() == m);
    assert(a.getPtr() == NULL);
    assert(b.getPtr() == &m);
    assert(c.getPtr() == NULL);
    assert(d.getPtr() == &m);
    assert(e.getPtr() == d.getPtr());
    assert(f.getPtr() == &m);

    c.setPtr(&n);
    a.delBits(3);
    b.delBits(3);
    c.delBits(3);

    assert(a.getBits() == 0);
    assert(b.getBits() == 0);
    assert(c.getBits() == 0);
    assert(a.getPtr() == NULL);
    assert(b.getPtr() == &m);
    assert(c.getPtr() == &n);
   
    a.setPtr(&n);
    a.addBits(m);
    b.addBits(m);
    d.addBits(m);

    assert(a.getBits() == m);
    assert(b.getBits() == m);
    assert(d.getBits() == m);
    assert(a.getPtr() == &n);
    assert(b.getPtr() == &m);
    assert(d.getPtr() == &m);
}

int main(int, char **)
{
    for(uint32_t i = 0 ; i < 20 ; ++i) test(i);

    cout << "Passed\n";
    return 0;
}
