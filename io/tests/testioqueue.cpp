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
// Filename : testioqueue.cpp (io/tests)
//
// Test IOQueue.h
//
// This file is a part of the UPPAAL toolkit.
// Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
// All right reserved.
//
// $Id: testioqueue.cpp,v 1.3 2005/05/11 19:08:15 adavid Exp $
//
///////////////////////////////////////////////////////////////////

// Tests are always for debugging.

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <time.h>
#include <iostream>
#include "io/IOQueue.h"
#include "debug/utils.h"
#include "debug/macros.h"
#include "debug/new.h"

using namespace std;
using namespace io;

/* Debug print: use in case of problems */
#if 0
static
void test_print(const requestdata_t *data, uint32_t size)
{
    uint32_t i;
    assert(size);
    cout << "[0.." << size-1 << "]:";
    for(i = 0; i < size; ++i)
    {
        cout << ' ' << data[i].offset;
    }
    cout << endl;
}
#endif

static
void test(uint32_t size)
{
    requestdata_t *input = new requestdata_t[size];
    requestdata_t *output = new requestdata_t[size];
    for(int k = 0; k < 10; ++k)
    {
        IOQueue q;
        uint32_t i, j;
        iorequest_t req(NULL);
        
        assert(size);
        debug_randomize(input, intsizeof(requestdata_t[size]));
        debug_randomize(output, intsizeof(requestdata_t[size]));
        
        // push all
        for(i = 0; i < size; ++i)
        {
            q.push(iorequest_t(&input[i]));
        }
        
        // pop all
        for(i = 0; i < size; ++i)
        {
            assert(q.tryPop(&req));
            output[i].offset = req.getOffset();
        }
        
        assert(q.empty());
        //test_print(output, size);
        
        // check order
        for(i = 0; i < size-1; ++i)
        {
            ASSERT(output[i].offset <= output[i+1].offset,
                   cerr << '[' << i << "]:" << output[i].offset
                   << " > [" << i+1 <<  "]:" << output[i+1].offset
                   << endl);
        }
        
        // ordered push partial
        j = RAND()%size;
        for(i = 0; i < j; ++i)
        {
            q.push(iorequest_t(&output[i]));
        }
        
        // pop and push
        for(i = 0; i < j; ++i)
        {
            assert(q.tryPop(&req));
            assert(output[i].offset == req.getOffset());
            if (rand()%2 == 0 && j < size)
            {
                q.push(iorequest_t(&output[j]));
                j++;
            }
        }
    }
    delete [] output;
    delete [] input;
}

int main(int argc, char *argv[])
{
    uint32_t n, seed;

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

    for(uint32_t i = 1 ; i < n ; ++i)
    {
        test(i);
    }

    cout << "Passed\n";
    return 0;
}
