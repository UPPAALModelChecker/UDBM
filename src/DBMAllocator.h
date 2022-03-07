// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-
////////////////////////////////////////////////////////////////////
//
// Filename : DBMAllocator.h
//
// This file is a part of the UPPAAL toolkit.
// Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
// All right reserved.
//
// $Id: DBMAllocator.h,v 1.4 2005/07/22 12:55:54 adavid Exp $$
//
///////////////////////////////////////////////////////////////////

#ifndef DBM_DBMALLOCATOR_H
#define DBM_DBMALLOCATOR_H

#include "dbm/config.h"
#include "dbm/fed.h"
#include "base/array_t.h"

#include <forward_list>
#include <vector>

/** @file
 * Allocation of classes related to fed_t, dbm_t, etc.
 * Declaration of the DBM allocator and DBM table.
 */
namespace dbm
{
#ifndef ENABLE_DBM_NEW
    ///< Allocator for DBMs, only if we don't use new for them.
    class DBMAllocator
    {
    public:
        ///< Default constructor
        DBMAllocator(): freeList(16), dbm1x1(1)
        {
            raw_t lezero = dbm_LE_ZERO;
            dbm1x1.newCopy(&lezero, 1);
            dbm1x1.intern();
        }

        ///< free memory of the deallocated idbm_t
        void cleanUp();

        /** Allocate memory to instantiate an idbm_t
         * @param dim: dimension of the DBM
         */
        void* alloc(cindex_t dim)
        {
            if (freeList.size() > dim) {
                auto& list = freeList[dim];
                if (!list.empty()) {
                    auto* dbm = list.front();
                    list.pop_front();
                    return dbm;
                }
            }
#ifdef ENABLE_STORE_MINGRAPH
            return new int32_t[intSizeOf(idbm_t) + dim * dim + bits2intsize(dim * dim)];
#else
            return new int32_t[intSizeOf(idbm_t) + dim * dim];
#endif
        }

        /** Deallocate an idbm_t
         * @param dbm: dbm to deallocate
         */
        void dealloc(idbm_t* dbm)
        {
            assert(dbm);
            cindex_t dim = dbm->getDimension();
            if (freeList.size() <= dim)
                freeList.resize(dim + 1);
            freeList[dim].push_front(dbm);
        }

        ~DBMAllocator()
        {
            // cleanup properly even if it is just one instance
            dbm1x1.nil();
            cleanUp();
        }

        ///< @return a constant DBM 1x1 (copies will make it non-mutable).
        dbm_t& dbm1()
        {
            assert(!dbm1x1.isEmpty());
            return dbm1x1;
        }

        static DBMAllocator& instance();

    private:
        std::vector<std::forward_list<idbm_t*>> freeList;
        dbm_t dbm1x1;
        static DBMAllocator dbm_allocator;
    };

    ///< Allocator instance
#else
    inline dbm_t dbm1()
    {
        dbm_t dbm1x1(1);
        raw_t lezero = dbm_LE_ZERO;
        dbm1x1.newCopy(&lezero, 1);
        dbm1x1.intern();
        return dbm1x1;
    }
#endif  // ENABLE_DBM_NEW
}  // namespace dbm

#endif  // DBM_FED_ALLOC_H
