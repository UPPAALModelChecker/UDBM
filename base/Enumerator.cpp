// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-
////////////////////////////////////////////////////////////////////
//
// Filename : Enumerator.cpp (base)
//
// This file is a part of the UPPAAL toolkit.
// Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
// All right reserved.
//
// $Id: Enumerator.cpp,v 1.1 2004/01/30 20:54:35 adavid Exp $
//
///////////////////////////////////////////////////////////////////

#include "base/Enumerator.h"

namespace base
{
    // return NULL when no bucket left
    SingleLinkable_t* LinkableEnumerator::getNextLinkable()
    {
        SingleLinkable_t* item = current;
        if (item)  // got a bucket, prepare for next
        {
            current = current->next;
        } else if (size) {
            // find 1st non null
            do {
                table++;
            } while (--size && !*table);

            // if valid
            if (size) {
                // while stop condition: nEntries == 0 || *table != NULL
                // if nEntries == 0, we are not here, item stays NULL
                // else *table not NULL and valid.
                assert(*table);

                item = *table;
                current = item->next;
            }
        }
        return item;
    }
}  // namespace base
