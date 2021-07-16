/* -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/*********************************************************************
 * Functions for safe and direct memory access.
 *
 * This file is a part of the UPPAAL toolkit.
 * Copyright (c) 2012 - 2019, Aalborg University.
 * Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
 * All right reserved.
 *
 **********************************************************************/

#ifndef INCLUDE_BASE_MEMORY_HPP
#define INCLUDE_BASE_MEMORY_HPP

#include <type_traits>  // conditional_t
#include <cstring>      // memcpy
#include <cinttypes>    // uint8_t

/**
 * A few notes on low-level memory manipulation:
 * 1) Arbitrary pointer type-casting is UB and poor alignment triggers UB sanitizer.
 * 2) Some architectures may not support instructions when the data type is not properly aligned.
 *    The supporting ones (e.g. Intel) may perform slower on non-aligned data.
 *    Therefore avoid performing direct operations and use it for storage (compression, encoding
 * etc). 3) The safe way is to perform byte-wise operations (which are many and potentially slower).
 * 4) Templated operations are most likely to be inlined and optimized.
 * 5) GCC and Clang recognize memcpy and -O2 use appropriate move instructions.
 * 6) arguments, internal variables, return values are mapped to registers
 * 7) there are only "move reg, mem" and "move mem,reg" instructions thus the code below is optimal.
 */

/**
 * Reads the specified type at arbitrary memory position.
 * @param address the location of the memory
 * @return a copy of memory content
 */
template <typename Data>
Data mem_get(const void* address)
{
    Data res;
    std::memcpy(&res, address, sizeof(Data));
    return res;  // C++17 guarantees Return-Value-Optimization
}

/**
 * Read arbitrary type value from the specified memory location (disregarding the alignment).
 * @param value the data location
 * @param mem the memory place
 */
template <typename Data>
void mem_get(Data& value, const void* address)
{
    std::memcpy(&value, address, sizeof(Data));  // -O2: "mov rax,[xx]; mov [yy],rax;"
}

/**
 * Copies arbitrary type value into specified memory location (disregarding the alignment).
 * @param address the memory place
 * @param value the data (passed-by-value, likely via register, enabling single instruction
 * optimaztion)
 */
template <typename Data>
void mem_set(void* address, const Data value)
{
    static_assert(sizeof(Data) <= 16, "only up to 16byte=128bit data types are expected");
    std::memcpy(address, &value, sizeof(Data));  // -O2: "movq [xx],xmm0;"
}

template <typename T>
using uint_equivalent_t = std::conditional_t<
    sizeof(T) == 1, uint8_t,
    std::conditional_t<sizeof(T) == 2, uint16_t,
                       std::conditional_t<sizeof(T) == 4, uint32_t,
                                          std::conditional_t<sizeof(T) == 8, uint64_t, void>>>>;

/**
 * Computes in-place bit-wise OR on the specified memory with the given mask.
 * @param address the place in memory
 * @param mask the binary mask of arbitrary type
 */
template <typename Mask>
void mem_or(void* address, const Mask mask)
{
    static_assert(sizeof(Mask) * 8 <= 64, "only up to 64bit types are supported");
    using Data = uint_equivalent_t<Mask>;
    Data d = mem_get<Data>(address) | mem_get<Data>(&mask);  // bitwise ops result in int(!)
    mem_set<Data>(address, d);
}

/**
 * Computes in-place bit-wise AND on the specified memory with the given mask.
 * @param mem the place in memory
 * @param mask the binary mask of arbitrary type
 */
template <typename Mask>
void mem_and(void* address, const Mask mask)
{
    static_assert(sizeof(Mask) * 8 <= 64, "only up to 64bit types are supported");
    using Data = uint_equivalent_t<Mask>;
    Data d = mem_get<Data>(address) & mem_get<Data>(&mask);  // bitwise ops result in int(!)
    mem_set<Data>(address, d);
}

#endif /* INCLUDE_BASE_MEMORY_HPP */
