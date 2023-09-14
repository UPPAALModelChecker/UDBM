/* -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/*********************************************************************
 *
 * Filename : mingraph_cache.c
 *
 * This file is a part of the UPPAAL toolkit.
 * Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
 * All right reserved.
 *
 * $Id: mingraph_cache.cpp,v 1.4 2005/07/22 12:55:54 adavid Exp $
 *
 *********************************************************************/

#ifdef ENABLE_MINGRAPH_CACHE

#include "mingraph_cache.h"

#include <base/intutils.h>
#include <base/stats.h>
#ifndef NDEBUG
#include <base/bitstring.h>
#endif
#include <debug/macros.h>

#include <iostream>

/* Default size for the cache, can redefined to anything > 0 */
/* Choose your favorite prime among http://primes.utm.edu/lists/small/1000.txt. */
#ifndef MINGRAPH_CACHE_SIZE
#define MINGRAPH_CACHE_SIZE 683
#endif

/* Simple cache entry */
typedef struct
{
    uint32_t hashValue;
    cindex_t dim;
    size_t cnt;
    uint32_t data[]; /* dbm[dim*dim] followed by bitMatrix */
} cache_t;

/* Simple cache implementation */
class Cache
{
public:
    Cache() { std::fill(entries, entires + MINGRAPH_CACHE_SIZE, nullptr); }

    ~Cache()
    {
        // cleanup properly even if it is just one instance
        for (uint32_t i = 0; i < MINGRAPH_CACHE_SIZE; ++i) {
            delete[](uint32_t*) entries[i];
        }
    }

    cache_t* get(uint32_t hashValue) { return entries[hashValue % MINGRAPH_CACHE_SIZE]; }

    void set(cache_t* entry) { entries[entry->hashValue % MINGRAPH_CACHE_SIZE] = entry; }

private:
    cache_t* entries[MINGRAPH_CACHE_SIZE];
};

static Cache cache;

size_t mingraph_getCachedResult(const raw_t* dbm, cindex_t dim, uint32_t* bitMatrix, uint32_t hashValue)
{
    cache_t* entry = cache.get(hashValue);
    uint32_t dim2;
    if (entry && entry->hashValue == hashValue && entry->dim == dim &&
        base_areEqual(entry->data, dbm, dim2 = dim * dim)) /* hit! */
    {
        RECORD_NSTAT(MAGENTA(BOLD) "DBM: Mingraph cache", "hit");
        std::copy(&entry->data[dim2], &entry->data[dim2] + bits2intsize(dim2), bitMatrix); /* write result */
        assert(base_countBitsN(bitMatrix, bits2intsize(dim2)) == entry->cnt);
        return entry->cnt;
    } else /* miss! */
    {
        RECORD_NSTAT(MAGENTA(BOLD) "DBM: Mingraph cache", "miss");
        return 0xffffffff;
    }
}

void mingraph_putCachedResult(const raw_t* dbm, cindex_t dim, const uint32_t* bitMatrix, uint32_t hashValue, size_t cnt)
{
    uint32_t dim2 = dim * dim;
    cache_t* entry = cache.get(hashValue);
    if (!entry || entry->dim != dim) {
        delete[](uint32_t*) entry;
        entry = (cache_t*)new uint32_t[intsizeof(cache_t) + dim2 + bits2intsize(dim2)];
    }
    entry->hashValue = hashValue;
    entry->dim = dim;
    entry->cnt = cnt;
    std::copy(dbm, dbm + dim2, entry->data);
    std::copy(&entry->data[dim2], bitMatrix, bits2intsize(dim2));
    cache.set(entry);

    assert(base_countBitsN(&entry->data[dim2], bits2intsize(dim2)) == entry->cnt);
}

#endif /* ENABLE_MINGRAPH_CACHE */
