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

#include "hash/PointerTable.h"
#include "base/intutils.h"
#include <assert.h>

namespace hash
{
#define TOMBSTONE ((const void*) ~0)

    PointerTable::PointerTable()
        : table(new const void*[2]),
          tableSize(2),
          nbPointers(0)
    {
        table[0] = NULL;
        table[1] = NULL;
    }

    PointerTable::~PointerTable()
    {
        delete [] table;
    }

    void PointerTable::clear()
    {
        base_resetBest(table, tableSize*(sizeof(void*)/sizeof(int)));
        nbPointers = 0;
    }

    bool PointerTable::has(const void* ptr) const
    {
        size_t index = getIndex(ptr);
        size_t start = index;
        while(table[index] && table[index] != ptr)
        {
            index = (index + 1) & (tableSize-1);
            if (index == start)
            {
                return false;
            }
        }
        return table[index] == ptr;
    }

    bool PointerTable::add(const void* ptr)
    {
        size_t index = getIndex(ptr);
        size_t retries = 0;
        assert(index < tableSize);
        assert(ptr && ptr != TOMBSTONE);
        while(table[index] && table[index] != TOMBSTONE && table[index] != ptr)
        {
            index = (index + 1) & (tableSize - 1);
            assert(index < tableSize);
            ++retries;
        }
        if (table[index] == ptr)
        {
            return false;
        }
        table[index] = ptr;
        ++nbPointers;
        if (retries > 7 || nbPointers > (tableSize >> 1))
        {
            rehash();
        }
        return true;
    }

    bool PointerTable::del(const void* ptr)
    {
        size_t index = getIndex(ptr);
        size_t start = index;
        assert(index < tableSize);
        assert(ptr && ptr != TOMBSTONE);
        while(table[index] && table[index] != ptr)
        {
            index = (index + 1) & (tableSize - 1);
            assert(index < tableSize);
            if (index == start)
            {
                return false;
            }
        }
        if (table[index] == ptr)
        {
            table[index] = TOMBSTONE;
            --nbPointers;
            return true;
        }
        return false;
    }

    void PointerTable::rehash()
    {
        size_t oldSize = tableSize;
        const void** oldTable = table;
        tableSize <<= 1;
        table = new const void*[tableSize];
        base_resetBest(table, tableSize*(sizeof(void*)/sizeof(int)));
        for(size_t i = 0; i < oldSize; ++i)
        {
            if (oldTable[i] && oldTable[i] != TOMBSTONE)
            {
                size_t j = getIndex(oldTable[i]);
                while(table[j])
                {
                    j = (j+1) & (tableSize-1);
                }
                table[j] = oldTable[i];
            }
        }
        delete [] oldTable;
    }

    bool PointerTable::operator == (const PointerTable& arg) const
    {
        if (size() != arg.size())
        {
            return false;
        }

        for(size_t i = 0; i < tableSize; ++i)
        {
            if (table[i] && table[i] != TOMBSTONE && !arg.has(table[i]))
            {
                return false;
            }
        }

        return true;
    }
}
