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
// This file is a part of the UPPAAL toolkit.
// Copyright (c) 1995 - 2006, Uppsala University and Aalborg University.
// All right reserved.
//
///////////////////////////////////////////////////////////////////

#include <assert.h>
#include "io/Base64Coder.h"

namespace io
{
    const char* Base64Coder::IllegalLengthException::what() const throw()
    {
        return "$Length_is_not_a_multiple_of_4.";
    }

    const char* Base64Coder::IllegalCharacterException::what() const throw()
    {
        return "$Illegal_base64-encoded_character.";
    }

    // The static instance is here.
    Base64Coder Base64Coder::instance;

    Base64Coder::Base64Coder()
    {
        // Initialize map 6-bit to 64-bit.
        int i = 0;
        for(char c = 'A'; c <= 'Z'; c++)
        {
            mapTo64[i++] = c;
        }
        for(char c = 'a'; c <= 'z'; c++)
        {
            mapTo64[i++] = c;
        }
        for(char c = '0'; c <= '9'; c++)
        {
            mapTo64[i++] = c;
        }
        mapTo64[i++] = '+';
        mapTo64[i++] = '/';
    
        // Initialize map 64-bit to 6-bit.
        for(i = 0; i < 128; i++)
        {
            mapFrom64[i] = -1;
        }
        for(i = 0; i < 64; i++)
        {
            mapFrom64[(int) mapTo64[i]] = (char) i;
        }
    }

    std::string Base64Coder::encode(const std::string& in)
    {
        const char *map = Base64Coder::instance.mapTo64;
        size_t iLen = in.length();
        size_t oDataLen = (iLen*4+2)/3; // output length without padding
        size_t oLen = ((iLen+2)/3)*4+1; // output length including padding
        char out[oLen];
        size_t op = 0;
        for(size_t ip = 0; ip < iLen; )
        {
            int i0 = in[ip++] & 0xff;
            int i1 = ip < iLen ? (in[ip++] & 0xff) : 0;
            int i2 = ip < iLen ? (in[ip++] & 0xff) : 0;
            assert(op < oLen);
            out[op++] = map[i0 >> 2];
            assert(op < oLen);
            out[op++] = map[((i0 & 3) << 4) | (i1 >> 4)];
            assert(op < oLen);
            out[op] = op < oDataLen ? map[((i1 & 0xf) << 2) | (i2 >> 6)] : '=';
            op++;
            assert(op < oLen);
            out[op] = op < oDataLen ? map[i2 & 0x3f] : '=';
            op++;
        }
        assert(op < oLen);
        out[op] = '\0';
        return out;
    }

    std::string Base64Coder::decode(const std::string& in) throw(std::exception)
    {
        const char *map = Base64Coder::instance.mapFrom64;
        size_t iLen = in.length();
        if (iLen % 4 != 0)
        {
            throw IllegalLengthException();
        }
        while(iLen > 0 && in[iLen-1] == '=')
        {
            iLen--; // Skip padding.
        }
        size_t oLen = (iLen*3) / 4;
        char out[oLen];
        size_t op = 0;
        for(size_t ip = 0; ip < iLen; )
        {
            int i0 = in[ip++];
            int i1 = in[ip++];
            int i2 = ip < iLen ? in[ip++] : 'A';
            int i3 = ip < iLen ? in[ip++] : 'A';
            if (i0 > 127 || i1 > 127 || i2 > 127 || i3 > 127)
            {
                throw IllegalCharacterException();
            }
            int b0 = map[i0];
            int b1 = map[i1];
            int b2 = map[i2];
            int b3 = map[i3];
            if (b0 < 0 || b1 < 0 || b2 < 0 || b3 < 0)
            {
                throw IllegalCharacterException();
            }
            assert(op < oLen);
            out[op++] = (char) ((b0 << 2) | (b1 >> 4));
            if (op < oLen)
            {
                out[op++] = (char) (((b1 & 0xf) << 4) | (b2 >> 2));
            }
            if (op < oLen)
            {
                out[op++] = (char) (((b2 & 3) << 6) | b3);
            }
        }
        return std::string(out, oLen);
    }
}

