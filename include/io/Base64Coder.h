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

#ifndef INCLUDE_IO_BASE64CODER_H
#define INCLUDE_IO_BASE64CODER_H

#include <string>
#include <exception>

namespace io
{
    class Base64Coder
    {
    public:
        static std::string encode(const std::string&);
        static std::string decode(const std::string&) throw(std::exception);

        class IllegalLengthException : public std::exception
        {
        public:
            virtual const char *what() const throw ();
        };

        class IllegalCharacterException : public std::exception
        {
        public:
            virtual const char *what() const throw ();
        };

    private:
        static Base64Coder instance;

        Base64Coder();
        char mapTo64[64];
        char mapFrom64[128];
    };
}

#endif // INCLUDE_IO_BASE64CODER_H
