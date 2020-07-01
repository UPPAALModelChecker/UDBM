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
// Filename : new.cpp
//
// Implementation of new/delete using the malloc/free to avoid to
// mess-up with the real new/delete calls. We monitor the calls.
//
// This file is a part of the UPPAAL toolkit.
// Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
// All right reserved.
//
// $Id: new.cpp,v 1.8 2005/07/22 12:55:54 adavid Exp $
//
///////////////////////////////////////////////////////////////////

#if !defined(_WIN32) && defined(ENABLE_MONITOR)

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

#include "base/linkable.h"
#include "base/intutils.h"
#include "debug/macros.h"
#include "debug/new.h"
#include "debug/malloc.h"

// no mess please
#ifdef new
#undef new
#endif
#ifdef delete
#undef delete
#endif
#ifdef malloc
#undef malloc
#endif
#ifdef free
#undef free
#endif

// TODO: multithread support, if it's possible

// Private hash table to store allocated memory.
// 2 formats are available depending on the level
// of verbosity, ie, debug information available.
// These 2 structures are MemBucket_t and ExtMemBucket_t
// for the extended version. EXTENSION_BIT marks that
// the extended version is used. The bit is stored in 
// the size field of MemBucket_t. The extension contains
// position information on where the memory was allocated.

#define INIT_SIZE (1 << 10)
#define EXTENSION_BIT 0x80000000
#define NOSIZE        0x7fffffff
#define BUCKETSIZE(B) ((B)->size & ~EXTENSION_BIT)

typedef unsigned int uint;

namespace debug
{
    /** Source position information.
     * It is not always available,
     * depending on which new is called.
     */
    struct PositionInfo_t
    {
        const char *filename; /**< which filename */
        int line;             /**< line number    */
        const char *function; /**< which function */


        /** @return a hash for this
         * position. Used when gathering
         * statistics on leaks.
         */
        uintptr_t hash() const
        {
            return
                (uintptr_t) filename ^
                line ^
                (uintptr_t) function;
        }

        /** @return true if 2 positions
         * are equal. NULL are possible
         * arguments.
         * Note: filename and
         * function are const char* given
         * by the compiler, which means,
         * they are unique pointers.
         */
        static bool areEqual(const PositionInfo_t *p1,
                             const PositionInfo_t *p2)
        {
            if (p1 && p2)
            {
                return
                    p1->filename == p2->filename &&
                    p1->line     == p2->line &&
                    p1->function == p2->function;
            }
            else
            {
                return p1 == p2;
            }
        }
    };
    
    /** To gather statistics on leaks we
     * use a simple hash table of STATS_SIZE
     * that is allocated in the stack.
     * STATS_SIZE must be a small power of 2.
     */
#define STATS_SIZE 256
#define STATS_MASK ((STATS_SIZE)-1)


    /** The simple hash table to gather
     * statistics on leaks.
     * NOTE: this tables stores pointers
     * to PositionInfo_t, so it assumes
     * it can access them in addLeak and
     * printStats. It is fine to delete
     * these PositionInfo_t before or
     * after ~StatsTable.
     */
    class StatsTable
    {
    public:
        StatsTable();
        ~StatsTable();

        /** Add a leak.
         * @param size: size of the leak (in bytes)
         * @param where: where it was allocated (may
         * be NULL if information not available).
         */
        void addLeak(size_t size,
                     const PositionInfo_t *where);

        /** Print statistics.
         * @param os: where to print.
         */
        void printStats(std::ostream &os) const;

    private:

        /** The bucket used to store leaks.
         */
        struct leak_t : public base::SingleLinkable<leak_t>
        {
            size_t size;            /**< size of the leak       */
            const PositionInfo_t *where; /**< where it was allocated */
            size_t nb;              /**< number of allocations  */
        };

        leak_t* table[STATS_SIZE]; /**< entries of the hash table */
    };

    /** The hash table that stores all allocated
     * memory with new (and removes entries deleted
     * with delete).
     */
    class NewPointerTable
    {
    public:
        NewPointerTable();
        ~NewPointerTable();
        
        /** Remember an allocated memory zone
         * of some size = store in the hash table.
         * @param ptr: allocated memory.
         * @param size: size (bytes) of the allocation.
         */
        void remember(void *ptr, size_t size);

        /** Forget an allocated memory zone ==
         * remove from hash table.
         * @param ptr: allocated memory.
         * @param nosize: check for NOSIZE or normal size
         */
        void forget(void *ptr, bool nosize);
        
        /** Remember an allocated memory zone
         * of some size = store in the hash table;
         * with allocation source information.
         * @param ptr: allocated memory.
         * @param size: size (bytes) of the allocation.
         * @param filename: which filename 'new' is called.
         * @param line: line position.
         * @param function: in which function.
         */
        void remember(void *ptr, size_t size,
                      const char *filename, int line,
                      const char *function);

        /** Prepare a delete = store position information
         * before calling a delete. This is a trick to
         * get position information with delete since
         * we cannot overload delete in such a way to
         * return such information.
         * @param filename: which filename 'delete' is called.
         * @param line: line position.
         * @param function: in which function
         */
        void prepareDelete(const char *filename, int line,
                           const char *function);
        
    private:
        
        /** Print allocation statistics (not the leaks).
         * @param isLeak: format the printout as
         * leak if isLeak is true, or simply as allocated
         * memory if isLeak is false.
         */
        void printStats(bool isLeak) const;
        
        /** Computes a hash value from a pointer:
         * since pointers are 32 bit aligned, pointer >> 2
         * is a good associated hash value.
         */
        static uintptr_t hashPtr(const void* ptr)
        {
            return ((uintptr_t)ptr) >> 2;
        }
        
        /** Increment number of buckets, may rehash.
         */
        void inc()
        {
            if (++nbBuckets > (mask >> 1)) rehash();
        }

        /** Decrement number of buckets.
         * @pre there is at least a bucket in the table.
         */
        void dec()
        {
            assert(nbBuckets > 0);
            nbBuckets--;
        }

        /** Rehash the hash table.
         */
        void rehash();
        
        /** Basic allocation information (bucket in
         * the hash table): pointer and size
         * allocated.
         * NOTE: size has an extension bit
         * (EXTENSION_BIT) set if there is more
         * information, ie, if the bucket is an
         * ExtMemBucket_t.
         */
        struct MemBucket_t :
            public base::SingleLinkable<MemBucket_t>
        {
            const void *ptr;  /**< allocated memory */
            size_t size; /**< allocated size | extension bit */
        };
        
        /** Extended allocation information: contains
         * the position of the allocation.
         */
        struct ExtMemBucket_t : public MemBucket_t
        {
            PositionInfo_t pos; /**< where memory was allocated */
        };
        
        PositionInfo_t pos;  /**< temporary position used by prepareDelete */
        int peakAlloc;       /**< peak allocation == largest cumulated allocation */
        uint minAlloc;       /**< smallest allocation == smallest unit */
        uint maxAlloc;       /**< largest allocation == largest unit */
        int totalAlloc;      /**< current total allocation */
        int overhead;        /**< current overhead used by NewPointerTable */
        int maxOverhead;     /**< maximal overhead ever used by NewPointerTable */
        uint nbBuckets;      /**< current number of buckets */
        uint mask;           /**< mask to access the hash table = size-1 where size=2^n */
        MemBucket_t **table; /**< hash table of size mask+1 */
    };

    /* Constructor: reset the hash table
     */
    StatsTable::StatsTable()
    {
        base_resetLarge(table, STATS_SIZE*(sizeof(void*)/sizeof(int)));
    }

    /* Destructor: deallocate all the buckets.
     */
    StatsTable::~StatsTable()
    {
        size_t n = STATS_SIZE;
        leak_t **leakTable = table;
        do {
            leak_t *leaki = *leakTable++;
            while(leaki)
            {
                leak_t *next = leaki->getNext();
                free(leaki);
                leaki = next;
            }
        } while(--n);
    }

    /* Use a simple hash based on the size
     * allocated to access the hash table.
     * Add a leak_t in the table if this
     * kind of leak (same file, line, function, size)
     * is not found.
     */
    void StatsTable::addLeak(size_t size,
                            const PositionInfo_t *where)
    {
        uintptr_t hashValue = size ^ (where ? where->hash() : 0);
        leak_t **entry = &table[hashValue & STATS_MASK];
        leak_t *leak = *entry;
        // look for this leak
        while(leak)
        {
            if (leak->size == size &&
                PositionInfo_t::areEqual(leak->where, where))
            {
                leak->nb++; // one more of this type of leak
                return;
            }
            leak = leak->getNext();
        }
        // new type of leak
        leak = (leak_t*) malloc(sizeof(leak_t));
        leak->link(entry);
        leak->size = size;
        leak->where = where;
        leak->nb = 1;
    }

    /* Stats of the leak table:
     * go through all buckets and print
     * information.
     */
    void StatsTable::printStats(std::ostream &os) const
    {
        size_t n = STATS_SIZE;
        // The compiler gave the right type...
        leak_t* const* leakTable = table;
        do {
            const leak_t *leaki = *leakTable++;
            while(leaki)
            {
                os << RED(BOLD) "\t"
                   << leaki->nb;
                if (leaki->size != NOSIZE)
                {
                    os << " x " << leaki->size << " bytes";
                }
                else
                {
                    os << (leaki->nb > 1 ? " pointers" : " pointer");
                }
                if (leaki->where)
                {
                    os << " from "
                       << debug_shortSource(leaki->where->filename)
                       << ':' << leaki->where->function
                       << '(' << leaki->where->line
                       << ')';
                }

                os << NORMAL "\n";
                leaki = leaki->getNext();
            }
        } while(--n);
    }

    /* Constructor for table of allocated memory
     * by new: reset the hash table and the statistics.
     */
    NewPointerTable::NewPointerTable()
        : peakAlloc(0), minAlloc(0xffffffff), maxAlloc(0),
          totalAlloc(0), nbBuckets(0), mask(INIT_SIZE-1)
    {
        table = (MemBucket_t**) malloc(INIT_SIZE*sizeof(MemBucket_t*));
        if (!table)
        {
            std::cerr << "Fatal: could not allocate table for monitor!\n";
            throw std::bad_alloc();
        }
        base_resetLarge(table, INIT_SIZE*(sizeof(void*)/sizeof(int)));
        overhead = INIT_SIZE*sizeof(MemBucket_t*) + sizeof(NewPointerTable);
        maxOverhead = overhead;
        pos.filename = NULL;
        pos.line = 0;
        pos.function = NULL;
    }
    
    /** Print memory usage:
     * @param caption: caption for a given statistic
     * @param mem: memory usage statistic
     */
    static
    void new_print(const char *caption, uint mem)
    {
        std::cerr << caption << mem << 'B';
        if (mem >= 1024)
        {
            std::cerr << "\t= ";
            debug_printMemory(stderr, mem);
        }
    }

    /* Print stats of NewPointerTable.
     */
    void NewPointerTable::printStats(bool leak) const
    {
        uint min =
            (minAlloc == 0xffffffff) ? 0 : minAlloc;
        
        std::cerr << CYAN(THIN) "Allocated memory stats:";
        new_print("\n\tPeak     = ", peakAlloc);
        new_print("\n\tSmallest = ", min);
        new_print("\n\tLargest  = ", maxAlloc);
        new_print("\n\tOverhead = ", maxOverhead);
        std::cerr << NORMAL "\n";
        
        if (leak)
        {
            if (totalAlloc != 0)
            {
                std::cerr << RED(BOLD) "Memory leak of "
                          << totalAlloc
                          << " bytes!\nDetails:" NORMAL "\n";
            }
        }
        else
        {
            std::cerr << CYAN(THIN) "Allocated memory: "
                      << totalAlloc 
                      << " bytes" NORMAL "\n";
        }
    }

    /* 1) Print allocation statistics
     * 2) gather leak statistics
     * 3) print leak statistics
     * 4) deallocate (if any) the memory allocation
     *    records
     * 5) deallocate hash table
     */    
    NewPointerTable::~NewPointerTable()
    {
        if (table)
        {
            StatsTable stats;
            printStats(true);
            
            MemBucket_t **entry = table;
            uint n = mask + 1;
            do {
                MemBucket_t *bucket = *entry++;
                while(bucket)
                {
                    if (bucket->size & EXTENSION_BIT)
                    {
                        ExtMemBucket_t *ebucket = static_cast<ExtMemBucket_t*>(bucket);
                        stats.addLeak(BUCKETSIZE(ebucket), &ebucket->pos);
                    }
                    else
                    {
                        stats.addLeak(bucket->size, NULL);
                    }
                    bucket = bucket->getNext();
                }
            } while(--n);
            
            // need the PositionInfo_t to print the stats
            stats.printStats(std::cerr);

            entry = table;
            n = mask + 1;
            do {
                MemBucket_t *bucket = *entry++;
                while(bucket)
                {
                    MemBucket_t *next = bucket->getNext();
                    free(bucket);
                    bucket = next;
                }
            } while(--n);

            free(table);
            table = NULL;
            overhead = sizeof(NewPointerTable);
        }
    }

    /* Standard rehash based on power of 2.
     * rehash() doubles the hash table size.
     */
    void NewPointerTable::rehash()
    {
        uint oldSize = mask + 1;
        uint newSize = oldSize << 1; // double size
        MemBucket_t **oldBuckets = table;
        MemBucket_t **newBuckets =
            (MemBucket_t**) malloc(newSize*sizeof(MemBucket_t*));
        
        if (!newBuckets)
        {
            DODEBUG(std::cerr << "Rehash aborted: out of memory\n");
            return;
        }
        
        overhead += newSize*sizeof(MemBucket_t*);
        mask = newSize - 1;
        table = newBuckets;
        base_resetLarge(newBuckets + oldSize, oldSize*(sizeof(void*)/sizeof(int)));
        
        uint i = 0;
        do {
            MemBucket_t *bucketi = oldBuckets[i];
            newBuckets[i] = NULL;
            while(bucketi)
            {
                MemBucket_t *next = bucketi->getNext();
                MemBucket_t **newEntry =
                    newBuckets + i + (hashPtr(bucketi->ptr) & oldSize);
                bucketi->link(newEntry);
                bucketi = next;
            }
        } while(++i < oldSize);
        
        free(oldBuckets);
        overhead -= oldSize*sizeof(MemBucket_t*);
        if (overhead > maxOverhead)
        {
            maxOverhead = overhead;
        }
    }
    
    /* Add a new bucket to the hash table.
     */
    void NewPointerTable::remember(void *ptr, size_t size)
    {
        if (table)
        {
            MemBucket_t **entry = &table[hashPtr(ptr) & mask];
            MemBucket_t *bucket = *entry;
            bool warning = false;
            
            // never called with size == NOSIZE
            assert(size != NOSIZE);

            // check collision list, for correctness
            // will find a match if malloc is bugged, ie,
            // returns twice the same pointer, which is
            // unlikely.

            while(bucket)
            {
                if (bucket->ptr == ptr)
                {
                    if (BUCKETSIZE(bucket) != NOSIZE)
                    {
                        fprintf(stderr,
                                MAGENTA(BOLD)
                                "Error: pointer 0x%x already registered!"
                                NORMAL "\n",
                                (uintptr_t)ptr);
                        return;
                    }
                    else if (!warning)
                    {
                        fprintf(stderr,
                                MAGENTA(BOLD)
                                "Warning: user pointer 0x%x already registered!"
                                NORMAL "\n",
                                (uintptr_t)ptr);
                        warning = true;
                    }
                }
                bucket = bucket->getNext();
            }
            
            /* New bucket.
             */
            bucket = (MemBucket_t*) malloc(sizeof(MemBucket_t));
            if (!bucket)
            {
                std::cerr << "Fatal: could not allocate bucket for monitor!\n";
                throw std::bad_alloc();
            }
            overhead += sizeof(MemBucket_t);
            if (overhead > maxOverhead)
            {
                maxOverhead = overhead;
            }
            bucket->link(entry);
            bucket->ptr = ptr;
            bucket->size = size;
            
            /* Allocation stats.
             */
            if (size != NOSIZE)
            {
                totalAlloc += size;
                if (peakAlloc < totalAlloc)
                {
                    peakAlloc = totalAlloc;
                }
                if (minAlloc > size)
                {
                    minAlloc = size;
                }
                if (maxAlloc < size)
                {
                    maxAlloc = size;
                }
            }
            inc(); // one more bucket
        }
    }
    
    /* Add a new extended bucket to the hash table.
     * Similar to previous 'remember'.
     */
    void NewPointerTable::remember(void *ptr, size_t size,
                                   const char *filename, int line, const char *function)
    {
        if (table)
        {
            MemBucket_t **entry = &table[hashPtr(ptr) & mask];
            MemBucket_t *bucket = *entry;
            bool warning1 = false, warning2 = false;

            while(bucket)
            {
                if (bucket->ptr == ptr)
                {
                    if (size == NOSIZE)
                    {
                        if (BUCKETSIZE(bucket) == NOSIZE)
                        {
                            if (!warning1)
                            {
                                fprintf(stderr,
                                        MAGENTA(BOLD)
                                        "Warning: registering again user pointer 0x%x! [%s",
                                        (uintptr_t)ptr,
                                        filename ? debug_shortSource(filename) : "");
                                if (function)
                                {
                                    fprintf(stderr, ":%s", function);
                                }
                                fprintf(stderr, "(%d)]" NORMAL "\n", line);
                                warning1 = true;
                            }
                        }
                        //else fprintf(stderr, "OK to register a user pointer already allocated\n");
                    }
                    else // size != NOSIZE
                    {
                        if (BUCKETSIZE(bucket) == NOSIZE)
                        {
                            if (!warning2)
                            {
                                fprintf(stderr,
                                        MAGENTA(BOLD)
                                        "Warning: pointer 0x%x already registered as user pointer! [%s",
                                        (uintptr_t)ptr,
                                        filename ? debug_shortSource(filename) : "");
                                if (function)
                                {
                                    fprintf(stderr, ":%s", function);
                                }
                                fprintf(stderr, "(%d)]" NORMAL "\n", line);
                                warning2 = true;
                            }
                        }
                        else
                        {
                            fprintf(stderr,
                                    MAGENTA(BOLD)
                                    "Error: pointer 0x%x already registered! [%s",
                                    (uintptr_t)ptr,
                                    filename ? debug_shortSource(filename) : "");
                            if (function)
                            {
                                fprintf(stderr, ":%s", function);
                            }
                            fprintf(stderr, "(%d)]" NORMAL "\n", line);
                            return;
                        }
                    }
                }
                bucket = bucket->getNext();
            }
            
            /* Add new extended bucket
             */
            ExtMemBucket_t *ebucket =
                (ExtMemBucket_t*) malloc(sizeof(ExtMemBucket_t));
            if (!ebucket)
            {
                std::cerr << "Fatal: could not allocate bucket for monitor!\n";
                throw std::bad_alloc();
            }
            overhead += sizeof(ExtMemBucket_t);
            if (overhead > maxOverhead)
            {
                maxOverhead = overhead;
            }
            ebucket->link(entry);
            ebucket->ptr = ptr;
            ebucket->size = size | EXTENSION_BIT;
            ebucket->pos.filename = filename;
            ebucket->pos.line = line;
            ebucket->pos.function = function;
            
            /* Allocation stats
             */
            if (size != NOSIZE)
            {
                totalAlloc += size;
                if (peakAlloc < totalAlloc)
                {
                    peakAlloc = totalAlloc;
                }
                if (minAlloc > size)
                {
                    minAlloc = size;
                }
                if (maxAlloc < size)
                {
                    maxAlloc = size;
                }
            }
            inc(); // one more bucket
        }
    }
    
    /* Look for the corresponding bucket
     * that records this allocation and remove it.
     * If not found, print error, meaning that one
     * tries to delete non allocated memory.
     */
    void NewPointerTable::forget(void *ptr, bool nosize)
    {
        if (table && ptr)
        {
            MemBucket_t **toBucket = &table[hashPtr(ptr) & mask];
            MemBucket_t *bucket = *toBucket;

            while(bucket)
            {
                assert(BUCKETSIZE(bucket) != NOSIZE || (bucket->size & EXTENSION_BIT));

                if (bucket->ptr == ptr &&
                    ((nosize && BUCKETSIZE(bucket) == NOSIZE) ||
                     (!nosize && BUCKETSIZE(bucket) != NOSIZE)))
                {
                    if (!nosize)
                    {
                        totalAlloc -= BUCKETSIZE(bucket);
                    }
                    bucket->unlink(toBucket);
                    overhead -= (bucket->size & EXTENSION_BIT) ?
                        sizeof(ExtMemBucket_t) :
                        sizeof(MemBucket_t);
                    free(bucket);
                    dec(); // one fewer bucket
                    return;
                }
                toBucket = bucket->getAtNext();
                bucket = bucket->getNext();
            }

            /* Error: bucket not found == ptr was not
             * allocated -> print msg.
             */
            if (pos.filename)
            {
                fprintf(stderr, RED(BOLD) "%s", debug_shortSource(pos.filename));
                if (pos.function)
                {
                    fprintf(stderr, ":%s", pos.function);
                }
                fprintf(stderr,
                        "(%d):\n%s 0x%x%s"
                        NORMAL "\n",
                        pos.line,
                        nosize ? "Unknown pointer" : "Pointer",
                        (uintptr_t)ptr,
                        nosize ? "!" : " not registered! Segmentation fault soon!");
            }
            else
            {
                fprintf(stderr,
                        RED(BOLD)
                        "%s 0x%x%s"
                        NORMAL "\n",
                        nosize ? "Unknown pointer" : "Pointer",
                        (uintptr_t) ptr,
                        nosize ? "!" : " not registered! Segmentation fault soon!");
            }

            if (!nosize) printStats(false); // before crashing...
        }
        
        // invalidate for next call
        pos.filename = NULL;
        pos.function = NULL;
    }
    
    /* Remember position for upcoming delete.
     */
    void NewPointerTable::prepareDelete(const char *filename, int line, const char *function)
    {
        pos.filename = filename;
        pos.line = line;
        pos.function = function;
    }

} // namespace debug

// Monitor instance

static debug::NewPointerTable new_table;

//////////////////////////////////////////////////////////////////////////
//
// Overrides the standard new/delete operators. This works by C++ magic,
// at link time these are used instead of the standard ones.
// Implement new/delete with malloc/free because we cannot use the original
// new/delete any more. We implement them!
// Implementation is straight-forward: malloc for new and free for delete
// with an associated remember/forget to monitor pointers.
// 
// There is one trick however with delete: we cannot use special information
// with the delete command so we have to call a prepare function first
// that stores position and file, then the delete itself. A macro does
// this for all delete. This is not possible for new (a = new bla) and
// the limitation for no placement constructor comes from here. The limitation
// for delete being inside brackets comes from here too. You should check
// all delete with a grep to be sure they'll all right.
// 
//////////////////////////////////////////////////////////////////////////

void *operator new(size_t size) throw (std::bad_alloc)
{
    void *ptr = malloc(size);
    if (!ptr)
    {
        throw std::bad_alloc();
    }
    new_table.remember(ptr, size);
    return ptr;
}

void *operator new[](size_t size) throw (std::bad_alloc)
{
    void *ptr = malloc(size);
    if (!ptr)
    {
        throw std::bad_alloc();
    }
    new_table.remember(ptr, size);
    return ptr;
}

void operator delete(void *ptr) throw()
{
    new_table.forget(ptr, false);
    if (ptr) free(ptr);
}

void operator delete[](void *ptr) throw()
{
    new_table.forget(ptr, false);
    if (ptr) free(ptr);
}

void *operator new(size_t size, const std::nothrow_t&) throw()
{
    void *ptr = malloc(size);
    if (ptr)
    {
        new_table.remember(ptr, size);
    }
    else
    {
        std::cerr << "NULL pointer crash soon!\n";
    }
    return ptr;
}

void *operator new[](size_t size, const std::nothrow_t&) throw()
{
    void *ptr = malloc(size);
    if (ptr)
    {
        new_table.remember(ptr, size);
    }
    else
    {
        std::cerr << "NULL pointer crash soon!\n";
    }
    return ptr;
}

void operator delete(void *ptr, const std::nothrow_t&) throw()
{
    new_table.forget(ptr, false);
    if (ptr) free(ptr);
}

void operator delete[](void *ptr, const std::nothrow_t&) throw()
{
    new_table.forget(ptr, false);
    if (ptr) free(ptr);
}

void *operator new(size_t size,
                   const char *filename, int line, const char *function)
    throw (std::bad_alloc)
{
    void *ptr = malloc(size);
    if (ptr)
    {
        new_table.remember(ptr, size, filename, line, function);
    }
    else
    {
        std::cerr << "NULL pointer crash soon!\n";
    }
    return ptr;
}

void *operator new[](size_t size,
                     const char *filename, int line, const char *function)
    throw (std::bad_alloc)
{
    void *ptr = malloc(size);
    if (ptr)
    {
        new_table.remember(ptr, size, filename, line, function);
    }
    else
    {
        std::cerr << "NULL pointer crash soon!\n";
    }
    return ptr;
}

void debug_prepareDelete(const char *filename, int line, const char *function)
{
    new_table.prepareDelete(filename, line, function);
}

static debug::PositionInfo_t localPosition = { filename:NULL, line:0, function:NULL};

void debug_pushPosition(const char *filename, int line, const char *function)
{
    localPosition.filename = filename;
    localPosition.line = line;
    localPosition.function = function;
}

void debug_pop()
{
    debug_prepareDelete(localPosition.filename, localPosition.line, localPosition.function);
}

void *debug_monitoredMalloc(size_t size, const char *filename, int line, const char *function)
{
    void *ptr = malloc(size);
    if (ptr)
    {
        new_table.remember(ptr, size, filename, line, function);
    }
    else
    {
        std::cerr << "Warning: malloc returns NULL...\n";
    }
    //fprintf(stderr, "Register 0x%x\n", (uintptr_t)ptr);
    return ptr;
}

void debug_monitoredFree(void *ptr, const char *filename, int line, const char *function)
{
    new_table.prepareDelete(filename, line, function);
    new_table.forget(ptr, false);
    if (ptr)
    {
        free(ptr);
    }
    else
    {
        std::cerr << RED(BOLD) "Error: free(NULL) was called: expect a segfault without -DENABLE_MONITOR!" NORMAL "\n";
    }
    //fprintf(stderr, "Unregister 0x%x\n", (uintptr_t)ptr);
}

void* debug_rememberPointer(void *ptr, const char *filename, int line, const char *function)
{
    //fprintf(stderr, "Remember 0x%x\n", (uintptr_t) ptr);
    new_table.remember(ptr, NOSIZE, filename, line, function);
    return ptr;
}

void debug_forgetPointer(void *ptr, const char *filename, int line, const char *function)
{
    //fprintf(stderr, "Forget 0x%x\n", (uintptr_t) ptr);
    new_table.prepareDelete(filename, line, function);
    new_table.forget(ptr, true);
}

void debug_forgetPtr(void *ptr)
{
    //fprintf(stderr, "Forget 0x%x\n", (uintptr_t) ptr);
    new_table.forget(ptr, true);
}

#endif
