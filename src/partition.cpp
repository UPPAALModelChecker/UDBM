// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-
////////////////////////////////////////////////////////////////////
//
// Filename : partition.cpp
//
// This file is a part of the UPPAAL toolkit.
// Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
// All right reserved.
//
// $Id: partition.cpp,v 1.13 2005/10/17 17:11:13 adavid Exp $
//
///////////////////////////////////////////////////////////////////

#include "dbm/partition.h"

#ifndef NDEBUG
#include "dbm/print.h"
#endif

#ifndef REDUCE
// static inline dbm::fed_t& REDUCE(dbm::fed_t& fed) { return fed.convexReduce(); }
// static inline dbm::fed_t& REDUCE(dbm::fed_t& fed) { return fed.partitionReduce(); }
// static inline dbm::fed_t& REDUCE(dbm::fed_t& fed) { return fed.convexReduce().expensiveReduce(); }
// static inline dbm::fed_t& REDUCE(dbm::fed_t& fed) { return fed.expensiveConvexReduce(); }
static inline dbm::fed_t& REDUCE(dbm::fed_t& fed) { return fed.mergeReduce(); }
// static inline dbm::fed_t& REDUCE(dbm::fed_t& fed) { return fed.reduce(); }
// static inline dbm::fed_t& REDUCE(dbm::fed_t& fed) { return fed.expensiveReduce(); }
// static inline dbm::fed_t& REDUCE(dbm::fed_t& fed) { return fed.reduce().mergeReduce(); }
// static inline dbm::fed_t& REDUCE(dbm::fed_t& fed) { return fed.noReduce(); }
#endif
#ifndef BIGREDUCE
static inline void BIGREDUCE(dbm::fed_t& fed) { fed.expensiveConvexReduce(); }
#endif

// static inline void REDUCE_SKIP(dbm::fed_t& fed, size_t s) { REDUCE(fed); }
static inline void REDUCE_SKIP(dbm::fed_t& fed, size_t s) { fed.mergeReduce(s); }
// static inline void REDUCE_SKIP(dbm::fed_t& fed, size_t s) { fed.mergeReduce(0,2); }

namespace dbm
{
    void partition_t::intern()
    {
        assert(fedTable);

        if (isPtr()) {
            entry_t** start = fedTable->getTable();
            entry_t** end = start + fedTable->getSize();
            for (; start < end; ++start) {
                if (*start != nullptr)
                    (*start)->fed.intern();
            }
        }
    }

    void partition_t::add(uintptr_t id, fed_t& fed)
    {
        assert(fedTable);

        if (!isPtr()) {
            fedTable = fedtable_t::create(edim(), eflag());
            // Now it is mutable.
        } else {
            checkMutable();
        }
        if (fedTable->add(id, fed)) {
            fedTable = fedTable->larger();
        }
    }

    partition_t::fedtable_t* partition_t::fedtable_t::create(cindex_t dim, bool noP)
    {
        // call placement constructor
        return new (new char[sizeof(partition_t::fedtable_t) + INIT_SIZE * sizeof(entry_t*)])
            partition_t::fedtable_t(dim, noP);
    }

    void partition_t::fedtable_t::remove()
    {
        assert(refCounter == 0);

#ifndef NDEBUG
        if (!noPartition) {
            const entry_t* const* start = getBeginTable();
            const entry_t* const* end = getEndTable();
            for (const entry_t* const* i = start; i != end; ++i)
                if (*i != nullptr) {
                    for (const entry_t* const* j = start; j != end; ++j)
                        if (*j != nullptr && i != j) {
                            assert((*i)->fed.getDimension() <= 1 || (((*i)->fed & (*j)->fed).isEmpty()));
                        }
                }
        }
#endif

        uint32_t n = mask + 1;
        for (uint32_t i = 0; i < n; ++i) {
            delete table[i];  // delete NULL is fine
        }

        all.nil();
        // allocate as char*, deallocate as char*
        delete[]((char*)this);
    }

    fed_t partition_t::fedtable_t::get(uint32_t id) const
    {
        uint32_t index = id & mask;
        while (table[index] != nullptr && table[index]->id != id) {
            index = (index + 1) & mask;
        }
        return table[index] != nullptr ? table[index]->fed : fed_t(getDimension());
    }

    bool partition_t::fedtable_t::add(uintptr_t id, fed_t& fedi)
    {
        // Partition invariant if !noPartition, otherwise keep partition only for id == 0.
        fed_t newi = fedi;

        if (!(noPartition && id != 0)) {
            newi -= all;
        }
        /*
        else
        {
            uint32_t id0 = 0;
            while(table[id0] && table[id0]->id != 0)
            {
                id0 = (id0 + 1) & mask;
            }
            if (table[id0]) newi -= table[id0]->fed;
        }
        */

        if (newi.isEmpty()) {
            fedi.setEmpty();
            return false;
        }

        // Add new federation to all (cheapest way).
        if (REDUCE(newi).size() < fedi.size()) {
            size_t s = all.size();
            if (s > 0) {
                fed_t copy = newi;
                REDUCE_SKIP(all.appendEnd(copy), s);
            } else {
                all = newi;  // newi already reduce.
            }
            fedi.setEmpty();
        } else {
            size_t s = all.size();
            REDUCE_SKIP(all.appendEnd(fedi), s);
        }

        // Find right entry for id.
        auto index = id & mask;
        while (table[index] != nullptr && table[index]->id != id) {
            index = (index + 1) & mask;
        }

        if (table[index] != nullptr)  // New entry;
        {
            table[index] = new entry_t(id, newi);
            nbEntries++;
            newi.nil();
        } else  // Add to existing entry.
        {
            size_t s = table[index]->fed.size();
            REDUCE_SKIP(table[index]->fed.appendEnd(newi), s);
        }
        table[index]->fed.intern();

        // Return = needs rehash?
        return 4 * nbEntries > 3 * (mask + 1);
    }

    // Move the entries to a larger table.
    partition_t::fedtable_t* partition_t::fedtable_t::larger()
    {
        assert(isMutable());

        uint32_t oldN = mask + 1;
        uint32_t newMask = (oldN << 1) - 1;
        auto* mem = new char[sizeof(partition_t::fedtable_t) + (newMask + 1) * sizeof(entry_t*)];
        auto* ftab = new (mem) partition_t::fedtable_t{getDimension(), nbEntries, newMask, noPartition};
        std::fill(ftab->table, ftab->table + (newMask + 1), nullptr);

        ftab->all = all;
        for (uint32_t i = 0; i < oldN; ++i)
            if (table[i] != nullptr) {
                auto j = table[i]->id & newMask;
                while (ftab->table[j] != nullptr) {
                    j = (j + 1) & newMask;
                }
                ftab->table[j] = table[i];
            }

        all.nil();
        delete[]((char*)this);
        return ftab;
    }

    size_t partition_t::fedtable_t::getNumberOfDBMs() const
    {
        size_t nb = 0;
        const entry_t* const* start = table;
        const entry_t* const* end = start + getSize();
        for (; start < end; ++start) {
            if (*start != nullptr)
                nb += (*start)->fed.size();
        }
        return nb;
    }

    // Deep copy of fedtable_t
    partition_t::fedtable_t* partition_t::fedtable_t::copy()
    {
        uint32_t n = mask + 1;
        auto* mem = new char[sizeof(partition_t::fedtable_t) + n * sizeof(entry_t*)];
        auto* ftab = new (mem) partition_t::fedtable_t{getDimension(), nbEntries, mask, noPartition};

        ftab->all = all;
        for (uint32_t i = 0; i < n; ++i) {
            ftab->table[i] = table[i] != nullptr ? new entry_t(table[i]->id, table[i]->fed) : nullptr;
        }

        assert(refCounter > 1);
        refCounter--;

        return ftab;
    }

}  // namespace dbm
