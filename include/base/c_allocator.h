/* -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/*********************************************************************
 *
 * Filename : c_allocator.h (base)
 *
 * This file is a part of the UPPAAL toolkit.
 * Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
 * All right reserved.
 *
 * $Id: c_allocator.h,v 1.2 2004/06/14 07:36:53 adavid Exp $
 *
 *********************************************************************/

#ifndef INCLUDE_BASE_C_ALLOCATOR_H
#define INCLUDE_BASE_C_ALLOCATOR_H

#include "base/inttypes.h"
#include <stddef.h>

/**
 * @file
 * Definition of the allocator function type
 * and declaration of a default function based
 * on malloc.
 * In different places where C libraries need
 * memory management, we want to stay independent
 * from particular needs. A generic allocator function
 * is declared for this purpose.
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Allocator function.
 * @param intSize: size to allocate in ints
 * @param data: custom data for the allocator
 * (ignored for malloc, for example, important
 *  for a custom allocator)
 * @return int32_t[intSize]
 */
typedef int32_t* (*allocator_f)(size_t intSize, void* data);

/** Deallocator function.
 * @param mem: memory to deallocate
 * @param intSize: size to deallocate in ints
 * @param data: custom data for the allocator
 * @pre intSize must correspond to the allocated
 * size, mem != NULL.
 */
typedef void (*deallocator_f)(void* mem, size_t intSize, void* data);

/** Allocator = allocator function + custom data.
 */
typedef struct
{
    void* allocData;
    allocator_f allocFunction;
    deallocator_f deallocFunction;
} allocator_t;

/** Default allocator function based on malloc.
 * @param intSize: size to allocate in ints
 * @param unused: not used, only to comply with allocator_f
 * @return int32_t[intSize] allocated by malloc
 */
int32_t* base_malloc(size_t intSize, void* unused);

/** Default deallocator function based on free.
 * @param mem: memory to deallocate
 * @param unused1,unused2: unused parameters, only to comply
 * with deallocator_f.
 */
void base_free(void* mem, size_t unused1, void* unused2);

/** Default allocator type based on malloc.
 */
extern allocator_t base_mallocator;

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_BASE_C_ALLOCATOR_H */
