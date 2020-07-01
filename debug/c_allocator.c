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

/* -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/*********************************************************************
 *
 * Filename : c_allocator.c (debug)
 *
 * This file is a part of the UPPAAL toolkit.
 * Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
 * All right reserved.
 *
 * $Id: c_allocator.c,v 1.4 2005/04/22 15:20:09 adavid Exp $
 *
 *********************************************************************/

#include <stdlib.h>
#include <assert.h>
#include "debug/c_allocator.h"
#include "debug/macros.h"


/** Easy hash of a pointer:
 * remove 2 lower bits (==0) modulo
 * size of allocation table.
 */
#define HASH(PTR) (((PTR) >> 2) & (debug_ALLOCATION_TABLE-1))


/* Allocate memory for debug_allocation_t + asked size + 2 magic numbers.
 */
int32_t* debug_malloc(size_t intSize, void *debugAllocations)
{
    int32_t *data;
    debug_allocation_t **table = (debug_allocation_t**) debugAllocations;

    /* Allocate an entry + size asked + 2 (for checking) */
    debug_allocation_t *entry = (debug_allocation_t*)
        malloc(sizeof(debug_allocation_t) + (intSize+2)*sizeof(int32_t));

    assert(debugAllocations);

    /* Allocation */
    data = (int32_t*) &entry[1];

    /* Write magic numbers */
    data[0] = (intptr_t) &data[1];
    data[intSize+1] = data[0];

    /* Register newly allocated data
     */
    table = &table[HASH(data[0])];
    entry->next = *table;
    entry->ptr = &data[1];
    entry->size = intSize;
    *table = entry;

    return &data[1];
}


/* Look for entry of allocated memory, check for
 * magic numbers, and free.
 */
void debug_free(void *ptr, size_t intSize, void *debugAllocations)
{
    debug_allocation_t **table = (debug_allocation_t**) debugAllocations;
    void **magic = (void**) ptr;

    assert(debugAllocations);

    if (!ptr) /* don't free NULL */
    {
        fprintf(stderr, "Free aborted: tried to free NULL!\n");
        return;
    }

    /* look for entry to free */
    table = &table[HASH((uintptr_t)ptr)];
    while(*table)
    {
        if ((*table)->ptr == ptr)
        {
            debug_allocation_t *next = (*table)->next;
            /* check magic numbers */
            if (magic[-1] != ptr || magic[(*table)->size] != ptr)
            {
                fprintf(stderr, RED(BOLD) "Memory was corrupted!" NORMAL "\n");
            }
            if (intSize != (*table)->size)
            {
                fprintf(stderr, RED(BOLD) "Wrong deallocated size!" NORMAL "\n");
            }
            free(*table);
            *table = next;
            return;
        }
        table = &(*table)->next;
    }

    /* fatal error: try to free unregistered memory */
    fprintf(stderr,
            "Free aborted: data at 0x%p not allocated!\n",
            ptr);
}


/* Any remaining entry is a memory leak.
 */
void debug_destroyAllocator(allocator_t *debugAllocator)
{
    uint32_t nbLeaks = 0, i = 0, leakSize = 0;
    debug_allocation_t **table;

    assert(debugAllocator);
    table = (debug_allocation_t**) debugAllocator->allocData;
    assert(table);
    
    /* go through all entries and print the leaks */
    do {
        debug_allocation_t *entry = table[i];
        while(entry)
        {
            /* << 2: int to byte */
            uint32_t size = entry->size << 2;
#ifdef VERBOSE
            if (nbLeaks)
            {
                fprintf(stderr, "+%d", size);
            }
            else
            {
                fprintf(stderr, RED(BOLD) "Leak of %d", size);
            }
#endif
            nbLeaks++;
            leakSize += size;
            entry = entry->next;
        }
    } while(++i < debug_ALLOCATION_TABLE);
    
    if (nbLeaks)
    {
#ifdef VERBOSE
        if (nbLeaks > 1)
        {
            fprintf(stderr, " = %d bytes" NORMAL "\n", leakSize);
        }
        else
        {
            fprintf(stderr, " bytes" NORMAL "\n");
        }
#else
        fprintf(stderr,
                RED(BOLD) "Leak of %d bytes in %d allocation%s" NORMAL "\n",
                leakSize, nbLeaks, nbLeaks > 1 ? "s." : ".");
#endif
    }
#ifdef VERBOSE
    else
    {
        fprintf(stderr,
                GREEN(BOLD) "No leak detected (allocator_t)." NORMAL "\n");
    }
#endif

    /* free the table */
    free(debugAllocator->allocData);
    debugAllocator->allocData = NULL;
}
