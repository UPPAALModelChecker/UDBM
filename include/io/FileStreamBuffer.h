// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-
////////////////////////////////////////////////////////////////////
//
// Filename : FileStreamBuffer.h (base)
//
// Wrap (C) FILE* to std::streambuf (very simple wrap).
//
// This file is a part of the UPPAAL toolkit.
// Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
// All right reserved.
//
// $Id: FileStreamBuffer.h,v 1.1 2004/06/14 07:36:54 adavid Exp $
//
///////////////////////////////////////////////////////////////////

#ifndef INCLUDE_IO_FILESTREAMBUFFER_H
#define INCLUDE_IO_FILESTREAMBUFFER_H

#include <iostream>
#include <cstdio>

namespace io
{
    /** Simple wrapper of FILE* to streambuf to use C-style API.
     * This class does not open or close the file, it only uses it.
     */
    class FileStreamBuffer : public std::streambuf
    {
    public:
        /** Constructor:
         * @param theFile: file stream (in C) to wrapp.
         * @pre theFile != NULL
         */
        FileStreamBuffer(FILE* theFile): wrappedFile(theFile) {}

        /** Destructor: flush.
         */
        virtual ~FileStreamBuffer() { fflush(wrappedFile); }

        /** overrides default overflow.
         */
        int overflow(int c) override { return fputc(c, wrappedFile); }

    private:
        FILE* wrappedFile;
    };
}  // namespace io

/** A macro to wrap print calls based on FILE* to
 * calls using ostream.
 * @param F: file* to wrap
 * @param OBJ: object to print
 */
#define PRINT_OSTREAM(F, OBJ)      \
    FileStreamBuffer local_fsb(F); \
    std::ostream(&local_fsb) << OBJ

#endif  // INCLUDE_IO_FILESTREAMBUFFER_H
