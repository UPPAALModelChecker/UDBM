// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-
////////////////////////////////////////////////////////////////////
//
// Filename : DBMAllocator.cpp
//
// This file is a part of the UPPAAL toolkit.
// Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
// All right reserved.
//
// $Id: DBMAllocator.cpp,v 1.3 2005/05/24 19:13:24 adavid Exp $$
//
///////////////////////////////////////////////////////////////////

#include "DBMAllocator.h"

namespace dbm
{
    // Instances.
    DBMTable dbm_table;
    base::ItemAllocator<alloc_ifed_t> ifed_allocator(200);
    base::ItemAllocator<alloc_fdbm_t> fdbm_allocator(600);

#ifndef ENABLE_DBM_NEW
    DBMAllocator& DBMAllocator::instance() { return DBMAllocator::dbm_allocator; }

    /* Go through the list of deallocated DBMs
     * and delete them all (ie really free memory).
     */
    void DBMAllocator::cleanUp()
    {
        for (auto& list : freeList) {
            for (auto* dbm : list) {
                delete[] reinterpret_cast<int32_t*>(dbm);
            }
        }
        freeList.clear();
    }

    ///< Wrapper function
    void* dbm_new(cindex_t dim) { return DBMAllocator::instance().alloc(dim); }

    ///< Wrapper function
    void dbm_delete(idbm_t* dbm)
    {
        assert(dbm);
        DBMAllocator::instance().dealloc(dbm);
    }

    ///< Wrapper function
    void cleanUp() { DBMAllocator::instance().cleanUp(); }

    DBMAllocator DBMAllocator::dbm_allocator;
#endif  // ENABLE_DBM_NEW

}  // namespace dbm
