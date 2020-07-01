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

////////////////////////////////////////////////////////////////////
//
// Filename : h3scale.cpp
//
// This file is a part of the UPPAAL toolkit.
// Copyright (c) 1995 - 2004, Uppsala University and Aalborg University.
// All right reserved.
//
// $Id: h3scale.cpp,v 1.3 2004/09/08 09:02:57 johanb Exp $
//
///////////////////////////////////////////////////////////////////

/** Compare different hash functions.
 * This test program compares the different hash functions defined in the hash
 * module with respect to speed.
 */

#include <iostream>
#include <cstdlib>
#include <cstddef>

#include <base/MZRan13.h>
#include <base/inttypes.h>

#include <hash/H3HashFunction.h>

/* For getrusage */
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>


using std::cerr;
using std::endl;
using std::size_t;

using base::MZRan13;

#define SWEEPS 100

/** Test the H3 stream hasher 
 * This function creates an H3 hash function and computes a hash value for each
 * row of a given matrix. The actual value is not remembered by the function.
 * @param d: A matrix containing the data.
 * @param length: 
 * @param width:
 * @param count:
 */
void test_h3_stream_hasher(uint32_t **d, size_t length,
                           size_t width, size_t count)
{
    hash::H3HashFunction hash(0,width);
    uint32_t val[width];
    for (int k = 0; k < SWEEPS; ++k) {
        for (unsigned int i = 0; i < count; ++i) {            
            hash.hash(d[i], length, val);
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc < 4) {
        cerr << "Usage: " << argv[0] << " hwidth length count [seed]" << endl;
        return EXIT_FAILURE;
    }

    size_t hwidth = std::atoi(argv[1]);
    size_t length = std::atoi(argv[2]);
    size_t count = std::atoi(argv[3]);
    int seed = (argc > 4 ? std::atoi(argv[4]) : time(NULL));
    
    MZRan13 random(seed);

    cerr << "Starting benchmark: random seed = " << seed << endl;

    /* Generate benchmark data. */
    uint32_t **data = new uint32_t* [count];
    uint32_t *now = new uint32_t [count * length];

    for (size_t i = 0 ; i < count; ++i) {
        data[i] = now;
        now += length;
    }

    now = data[0];

    for (int i = length * count; i != 0; --i) {
        *now = random();
        ++now;
    }

    cerr << "RESULT:";

    for (size_t i=1; i <= hwidth; ++i) {

       struct rusage before, after;

       getrusage(RUSAGE_SELF, &before); 
       
       test_h3_stream_hasher(data, length, i, count);
       
       getrusage(RUSAGE_SELF, &after); 
       
       uint32_t h3_stream = (after.ru_utime.tv_sec * 1000 + after.ru_utime.tv_usec / 1000) - (before.ru_utime.tv_sec * 1000 + before.ru_utime.tv_usec / 1000);

       cerr << " " << h3_stream;
    }

    cerr << endl;

    delete [] data[0];
    delete [] data;

    return EXIT_SUCCESS;
}


// Local Variables:
// mode: C++
// c-file-style: "stroustrup"
// c-basic-offset: 4
// fill-column: 79
// End:
 
