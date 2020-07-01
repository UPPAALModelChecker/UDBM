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
// Filename : H3HashFunction.cpp
//
// This file is a part of the UPPAAL toolkit.
// Copyright (c) 1995 - 2004, Uppsala University and Aalborg University.
// All right reserved.
//
// $Id: H3HashFunction.cpp,v 1.3 2005/01/25 18:30:22 adavid Exp $
//
///////////////////////////////////////////////////////////////////

#include "hash/H3HashFunction.h"
#include "base/MZRan13.h"
#include "base/intutils.h"
#include <cassert>

using std::ptrdiff_t;

namespace hash 
{
    /** H3 Hash function.
     * IMPLEMENTATION NOTES:
     * In order to gain speed the hash function is partially pre-computed. 
     * Instead of storing the matrix row by row, each group of eight rows is
     * represented by a 256 entry table. Thus instead of examining each bit in
     * the key individually, it suffices to examine the key bytewise. The gain
     * in computation time is around 10 times.
     *
     * To simplify computations when generating the hash function and computing
     * the hash values, each row in the matrix is divied into 32-bit chunks.
     * Each of these chunks are then handled separately.
     * 
     * Currently the key lenght is set to the length of the hash value +
     * 32-bits. 
     */


    H3HashFunction::H3HashFunction(uint32_t f, size_t width)
        : fWidth(width)
    {        
        base::MZRan13 generator(f); // Hash function generator.

        assert(sizeof(uint32_t) / sizeof(uint8_t) == 4);
        
        // Allocate memory for the hash function.
        uint32_t *mem = new uint32_t[width * (width + 1) * 256 * 4];
#ifndef NDEBUG
        uint32_t *end = mem + (width * (width + 1) * 256 *4);
#endif

        // Set up table of row-parts.
        function = new uint32_t*[width];
        for (size_t i = 0; i < width; ++i) {
            function[i] = mem + (i * (width + 1) * 256 * 4);
        }

        // Compute hash function.
        for (size_t k = 0; k < (width * (width + 1) * 4); ++k) {

            // Generate 8 rows.
            uint32_t buff[8];
            for (size_t i = 0; i < 8; ++i) {
                do {
                    buff[i] = generator();
                } while (buff[i] == 0);
            }
            
            // Compute 256-entry table based on the generated rows.
            for (size_t i = 0; i < 256; ++i) {
                uint32_t res = 0;
                for (size_t j = 0; j < 8; ++j) {
                    if (i & (1 << j)) {
                        res ^= buff[j];
                    }
                }
                mem[i] = res;
            }
            mem += 256;
        }

        assert(mem == end);
    }

    H3HashFunction::~H3HashFunction()
    {
        delete [] function[0];
        delete [] function;
    }
    
    void H3HashFunction::hash(const uint32_t *key, size_t len, uint32_t *res)
    {
        size_t j = fWidth - 1;
        ptrdiff_t flen = (fWidth + 1) * 256 * 4;

        /** Compute the hash value for a given key.
         * To do this we view the key as a sequence of 8-bit bytes instead of
         * 32-bit words, and use this to look up a sequence key parts in our
         * pre-computed tables. 
         *
         * NOTE: This method is a hotspot when using the supertrace pw-list. 
         */

        do { 
            uint32_t *now = function[j]; 
            uint32_t *end = now + flen;
            const uint8_t *key8 = reinterpret_cast<const uint8_t *>(key);
            const uint8_t *end8 = key8 + (4 * len);

            do {
                res[j] ^= now[*key8];
                now += 256;

                if (now >= end) { now -= flen; }
            } while (++key8 != end8);        
        } while ((j--) != 0); 
    }
}
