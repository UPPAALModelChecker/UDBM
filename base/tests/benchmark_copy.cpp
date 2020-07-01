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



#include "base/intutils.h"
#include "base/Timer.h"

#include <iostream>
#include <algorithm>

#define CNT 1000000000

int main()
{
    size_t sizes[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
                       20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60, 64,
                       72, 80, 88, 96, 104, 112, 120, 128, 198, 256, 
                       384, 512, 1024, 2048, 4096, 0 };
    std::cerr.precision(4);

    // Lets see how good the compiler is to copy small blocks if the
    // amount of data is fixed at compile time.
    std::cerr << "Size: 1 (fixed)";
    
    int *a = (int*)calloc(1, sizeof(int));
    int *b = (int*)calloc(1, sizeof(int));

    size_t cnt = CNT;
    base::Timer timer;
    for (uint32_t j = 0; j < cnt; j++)
    {
        base_copySmall(b, a, 1);
    }
    std::cerr << "\tsmall: " << timer.getElapsed();
    
    for (uint32_t j = 0; j < cnt; j++)
    {
        base_copyLarge(b, a, 1);
    }
    std::cerr << "\tlarge: " << timer.getElapsed();
    
    for (uint32_t j = 0; j < cnt; j++)
    {
        base_copyBest(b, a, 1);
    }
    std::cerr << "\tbest: " << timer.getElapsed();
    
    for (uint32_t j = 0; j < cnt; j++)
    {
        memcpy(b, a, sizeof(int));
    }
    std::cerr << "\tmemcpy: " << timer.getElapsed();
    
    for (uint32_t j = 0; j < cnt; j++)
    {
        std::copy(a, a + 1, b);
    }
    std::cerr << "\tcopy: " << timer.getElapsed() << std::endl;
        
    free(b);
    free(a);

    // Now lets compare the different copy routines.
    for (uint32_t i = 0; sizes[i]; i++) 
    {
        std::cerr << "Size: " << sizes[i];

        int *a = (int*)calloc(sizes[i], sizeof(int));
        int *b = (int*)calloc(sizes[i], sizeof(int));

        size_t cnt = CNT / sizes[i];
        base::Timer timer;
        for (uint32_t j = 0; j < cnt; j++)
        {
            base_copySmall(b, a, sizes[i]);
        }
        std::cerr << "\tsmall: " << timer.getElapsed();

        for (uint32_t j = 0; j < cnt; j++)
        {
            base_copyLarge(b, a, sizes[i]);
        }
        std::cerr << "\tlarge: " << timer.getElapsed();

        for (uint32_t j = 0; j < cnt; j++)
        {
            base_copyBest(b, a, sizes[i]);
        }
        std::cerr << "\tbest: " << timer.getElapsed();

        for (uint32_t j = 0; j < cnt; j++)
        {
            memcpy(b, a, sizes[i] * sizeof(int));
        }
        std::cerr << "\tmemcpy: " << timer.getElapsed();

        for (uint32_t j = 0; j < cnt; j++)
        {
            std::copy(a, a + sizes[i], b);
        }
        std::cerr << "\tcopy: " << timer.getElapsed() << std::endl;
        

        free(b);
        free(a);
    }
}
