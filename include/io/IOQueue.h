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
// Filename : IOQueue.h (io)
//
// Priority queue for IO requests and IO request definition.
//
// This file is a part of the UPPAAL toolkit.
// Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
// All right reserved.
//
// $Id: IOQueue.h,v 1.2 2004/02/11 11:17:08 adavid Exp $
//
///////////////////////////////////////////////////////////////////

#ifndef INCLUDE_IO_IOQUEUE_H
#define INCLUDE_IO_IOQUEUE_H

#include "base/PriorityQueue.h"
#include "base/bitptr.h"

namespace io
{
    /** Base request struct to define requests.
     */
    struct requestdata_t
    {
        uint64_t offset;
    };


    /** Abstract io request:
     * pointer to the offset of the operation
     * (on a file) with some bit information
     * (meaning unknown here).
     *
     * NOTE: do not inherit from iorequest_t
     * because it is copied in the IOQueue.
     */
    struct iorequest_t : public base::bitptr_t<requestdata_t>
    {
        /** Constructors = wrappers.
         */
        iorequest_t(requestdata_t *ptr)
            : base::bitptr_t<requestdata_t>(ptr) {}
        iorequest_t(requestdata_t *ptr, uint32_t bits)
            : base::bitptr_t<requestdata_t>(ptr, bits) {}
        

        /** Wrapper: @return offset of the io request
         */
        uint64_t getOffset() const
        {
            assert(getPtr());
            return getPtr()->offset;
        }

        /* Comparison operators are used only
         * to order requests in the priority queue.
         */

        /** Operator < defined on the offset
         */
        bool operator < (const iorequest_t& req) const
        {
            return getOffset() < req.getOffset();
        }

    };


    /** An IO queue is a priority queue
     * where the ordering is defined by the
     * offsets of the operations to perform.
     */
    class IOQueue : public base::PriorityQueue<iorequest_t>
    {
    public:

        /** Pop a request from the priority queue
         * @param req: pointer to the request to write
         * @return true if successful, false otherwise
         */
        bool tryPop(iorequest_t *req)
        {
            assert(req);
            if (empty()) return false;
            *req = top();
            pop();
            return true;
        }

    };

} // namespace io

#endif // INCLUDE_IO_IOQUEUE_H
