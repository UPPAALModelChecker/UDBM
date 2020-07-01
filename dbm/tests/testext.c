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
 * Filename : testext.c (dbm/tests)
 *
 * Test of ext.c
 *
 * This file is a part of the UPPAAL toolkit.
 * Copyright (c) 1995 - 2000, Uppsala University and Aalborg University.
 * All right reserved.
 *
 * $Id: testext.c,v 1.9 2005/05/11 19:08:14 adavid Exp $
 *
 **********************************************************************/

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <stdlib.h>
#include <time.h>
#include "base/bitstring.h"
#include "debug/macros.h"
#include "dbm/dbm.h"
#include "dbm/print.h"
#include "dbm/gen.h"

/* make the code more readable
 */
#define DBM1(I,J) dbm1[(I)*dim1+(J)]
#define DBM2(I,J) dbm2[(I)*dim2+(J)]

/* progress spinning bar
 */
#define PROGRESS() debug_spin(stderr)

static
raw_t *allocDBM(uint32_t dim)
{
    return (raw_t*) malloc(dim*dim*sizeof(raw_t));
}


/** invalidate table indices
 */
static
void test_invalidate(cindex_t *table, uint32_t n)
{
    while(n)
    {
        *table++ = ~0;
        --n;
    }
}

/** Print a redirection table and ignore invalid indices.
 * Use in case of problems.
 */
#if 0
static
void test_printTable(const uint32_t *table, uint32_t tableSize)
{
    while(tableSize)
    {
        if (*table == 0xffffffff)
        {
            printf(" ?");
        }
        else
        {
            printf(" %u", *table);
        }
        table++;
        --tableSize;
    }
    printf("\n");
}
#endif

/** debug print ; use in case of problem */
#if 0
static
void test_debugPrint(const raw_t *dbm1, uint32_t dim1,
                     const raw_t *dbm2, uint32_t dim2,
                     const uint32_t *table1,
                     const uint32_t *table2, uint32_t tableSize,
                     uint32_t i, uint32_t j)
{
    printf("Table1: ");
    test_printTable(table1, tableSize);
    dbm_print(stdout, dbm1, dim1);

    printf("Table2: ");
    test_printTable(table2, tableSize);
    dbm_print(stdout, dbm2, dim2);

    if (i || j)
    {
        printf("\nproblem at %u,%u -> %u, %u\n",
               i,j,
               table2[i], table2[j]);
    }
}
#endif

/** test shrinkExpand
 */
static
void test_shrinkExpand(uint32_t dim)
{
    uint32_t intSize = bits2intsize(dim);
    uint32_t tableSize = intSize*32;
    cindex_t srcTable[tableSize];
    cindex_t dstTable[tableSize];
    cindex_t checkTable[tableSize];
    uint32_t bitString1[intSize];
    uint32_t bitString2[intSize];
    raw_t *dbm1 = allocDBM(dim);
    raw_t *dbm2 = allocDBM(dim);
    uint32_t k;

    printf("Testing shrinkExpand(%u)\n", dim);

    for(k = 0; k < 10000; ++k)
    {
        cindex_t dim1 = rand()%dim + 1;
        cindex_t dim2 = rand()%dim + 1;
        cindex_t i,j;

        PROGRESS();

        test_invalidate(srcTable, tableSize);
        test_invalidate(dstTable, tableSize);
        test_invalidate(checkTable, tableSize);

        /* generate 2 bit tables */
        debug_generateBits(bitString1, intSize,
                           dim1, ONE);
        debug_generateBits(bitString2, intSize,
                           dim2, ONE);
        assert(base_countBitsN(bitString1, intSize) == dim1);
        assert(base_countBitsN(bitString2, intSize) == dim2);
        
        /* respect precondition
         */
        if (!base_areBitsEqual(bitString1, bitString2, intSize))
        {
            /* generate source DBM */
            dbm_generate(dbm1, dim1, 50000);
            
            /* unpack */
            base_bits2indexTable(bitString1, intSize, srcTable);
            base_bits2indexTable(bitString2, intSize, checkTable);

            /* shrinkExpand */
            assert(dim2 ==
                   dbm_shrinkExpand(dbm1, dbm2,
                                    dim1,
                                    bitString1,
                                    bitString2,
                                    intSize,
                                    dstTable));

            assert(dbm_isValid(dbm2, dim2));

            /* check table */
            assert(base_areEqual(dstTable, checkTable, intsizeof(cindex_t[dim])));
            
            for(i = 0; i < dim; ++i)
            {
                for(j = 0; j < dim; ++j)
                {
                    /* valid index condition == ~table[i]
                     * because of the invalidate value
                     */
                    if ((~dstTable[i]) && (~dstTable[j]))
                    {
                        if ((~srcTable[i]) && (~srcTable[j]))
                        {
                            assert(DBM1(srcTable[i], srcTable[j]) ==
                                   DBM2(dstTable[i], dstTable[j]));
                        }
                        else
                        {
                            if (i == 0 || i == j)
                            {
                                assert(DBM2(dstTable[i], dstTable[j]) ==
                                       dbm_LE_ZERO); /* new clock */
                            }
                            else
                            {
                                assert(DBM2(dstTable[i], dstTable[j]) ==
                                       (j == 0 ? dbm_LS_INFINITY :
                                        DBM2(dstTable[i], dstTable[0])));
                            }
                        }
                    }
                }
            }
        }

        /* print something to check
         
        if (k == 0 && dim1 < 10 && dim2 < 10)
        {
            test_debugPrint(dbm1, dim1,
                            dbm2, dim2,
                            srcTable, dstTable, tableSize,
                            0, 0);
        }
        */
    }

    free(dbm2);
    free(dbm1);
}


int main(int argc, char *argv[])
{
    uint32_t i, n, seed;

    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s size [seed]\n",
                argv[0]);
        return 1;
    }

    n = atoi(argv[1]);
    seed = argc > 2 ? atoi(argv[2]) : time(NULL);
    printf("Testing with seed=%u\n", seed);
    srand(seed);

    for(i = 1; i <= n; ++i)
    {
        test_shrinkExpand(i);
    }

    printf("Passed\n");
    return 0;
}
