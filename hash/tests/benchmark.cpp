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
// Filename : benchmark.cpp
//
// This file is a part of the UPPAAL toolkit.
// Copyright (c) 1995 - 2004, Uppsala University and Aalborg University.
// All right reserved.
//
// $Id: benchmark.cpp,v 1.3 2004/09/08 09:02:57 johanb Exp $
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

#include <hash/compute.h>
#include <hash/StreamHasher.h>
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

uint32_t test_original_hasher(uint32_t **d, size_t length, size_t count)
{
    uint32_t a = 0;
    for (int k = 0; k < SWEEPS; ++k) {
        for (unsigned int i = 0; i < count; ++i) {
            a ^= hash_computeU32(d[i], length, 0);
        }
    }
    return a;
}

uint32_t test_stream_hasher(uint32_t **d, size_t length, size_t count)
{
    uint32_t a = 0;
    for (int k = 0; k < SWEEPS; ++k) {
        for (unsigned int i = 0; i < count; ++i) {
            hash::StreamHasher hash(0);
            const uint32_t *now = d[i];
            for (unsigned int j = 0; j < length; ++j) {
                hash.addValue(*now);
                ++now;
            }
            a ^= hash.hash();
        }
    }
    return a;
}

uint32_t test_h3_hash_function(uint32_t **d, size_t length, size_t count)
{
    uint32_t a = 0;
    hash::H3HashFunction hash(0,1);
    for (int k = 0; k < SWEEPS; ++k) {
        for (unsigned int i = 0; i < count; ++i) {
            hash.hash(d[i], length, &a);
        }
    }
    return a;
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " length count [seed]" << endl;
        return 0;
    }

    size_t length = std::atoi(argv[1]);
    size_t count = std::atoi(argv[2]);
    int seed = (argc > 3 ? std::atoi(argv[3]) : time(NULL));
    
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

    struct rusage before, after;

    getrusage(RUSAGE_SELF, &before); 

    test_original_hasher(data, length, count);

    getrusage(RUSAGE_SELF, &after); 

    uint32_t original = (after.ru_utime.tv_sec * 1000 + after.ru_utime.tv_usec / 1000) - (before.ru_utime.tv_sec * 1000 + before.ru_utime.tv_usec / 1000);

    getrusage(RUSAGE_SELF, &before); 

    test_stream_hasher(data, length, count);

    getrusage(RUSAGE_SELF, &after); 

    uint32_t stream = (after.ru_utime.tv_sec * 1000 + after.ru_utime.tv_usec / 1000) - (before.ru_utime.tv_sec * 1000 + before.ru_utime.tv_usec / 1000);

    getrusage(RUSAGE_SELF, &before); 

    test_h3_hash_function(data, length, count);

    getrusage(RUSAGE_SELF, &after); 

    uint32_t h3 = (after.ru_utime.tv_sec * 1000 + after.ru_utime.tv_usec / 1000) - (before.ru_utime.tv_sec * 1000 + before.ru_utime.tv_usec / 1000);

    cerr << "RESULT: original = " << original 
         << ", stream = " << stream
         << ", h3 = " << h3 
         << endl;

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
 
