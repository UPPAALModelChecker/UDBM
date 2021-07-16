/* -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/*********************************************************************
 *
 * Filename : c_allocator.c (base)
 *
 * This file is a part of the UPPAAL toolkit.
 * Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
 * All right reserved.
 *
 * $Id: c_allocator.c,v 1.2 2004/06/14 07:36:52 adavid Exp $
 *
 *********************************************************************/

#include <stdlib.h>
#include "base/c_allocator.h"
#include "debug/malloc.h"

/* straight-forward (m)allocation of int[intSize]
 */
int32_t* base_malloc(size_t intSize, void* data) { return (int32_t*)malloc(intSize << 2); }

/* straight-forward deallocation
 */
void base_free(void* mem, size_t unused1, void* unused2) { free(mem); }

/* default allocator instance
 */
allocator_t base_mallocator = {
    allocData : NULL,
    allocFunction : base_malloc,
    deallocFunction : base_free
};
