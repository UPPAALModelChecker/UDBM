/* -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/*********************************************************************
 * Implements hash functions.
 *
 * This file is a part of the UPPAAL toolkit.
 * Copyright (c) 2012 - 2019, Aalborg University.
 * Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
 * All right reserved.
 *
 * Not reviewed.
 *
 **********************************************************************/

#include "hash/compute.h"
#include "base/memory.hpp"

#ifdef MURMUR2_HASH1

// Murmur hash2. hash1+2: original, hash1: modified.

uint32_t hash_computeU8(const uint8_t* data8, size_t len, uint32_t initHash)
{
    // 'm' and 'r' are mixing constants generated offline.
    // They're not really 'magic', they just happen to work well.

    const uintptr_t m = (sizeof(uintptr_t) == 8) ? 0xc6a4a7935bd1e995LL : 0x5bd1e995L;

#ifdef MURMUR2_HASH2
    const uintptr_t r = (sizeof(uintptr_t) == 8) ? 47 : 24;
#endif /* MURMUR2_HASH2 */

    uintptr_t h = (sizeof(uintptr_t) == 8) ? initHash ^ len * m : initHash ^ len;

    const uintptr_t* data = (uintptr_t*)data8;

    // Main hashing loop.

    while (len >= sizeof(uintptr_t)) {
        uintptr_t k = *data++;
        len -= sizeof(uintptr_t);

#ifdef MURMUR2_HASH2
        // This is useless if the whole hash is to be used
        // but the point is to use lower bits as index so
        // this mixing does have an effect to distribute the
        // variation along the bits, most importantly the
        // lower bits.

        k *= m;
        k ^= k >> r;
        k *= m;
#endif /* MURMUR2_HASH2 */
        h *= m;
        h ^= k;
    }

    if constexpr (sizeof(intptr_t) == 8) {  // 64bit architecture
        switch (len) {
        case 7:
            h *= m;
            h ^= mem_get<uint64_t>(data) & 0x00FF'FFFF'FFFF'FFFFull;
            break;
        case 6:
            h *= m;
            h ^= mem_get<uint64_t>(data) & 0x0000'FFFF'FFFF'FFFFull;
            break;
        case 5:
            h *= m;
            h ^= mem_get<uint64_t>(data) & 0x0000'00FF'FFFF'FFFFull;
            break;
        case 4:
            h *= m;
            h ^= mem_get<uint32_t>(data);
            break;
        case 3:
            h *= m;
            h ^= mem_get<uint32_t>(data) & 0x00FF'FFFFu;
            break;
        case 2:
            h *= m;
            h ^= mem_get<uint16_t>(data);
            break;
        case 1: h *= m; h ^= mem_get<uint8_t>(data);
        }

        // Not fundamentally necessary but
        // mixes the bits around, which is
        // good for getting indices later.
        h ^= h >> 45;
        h *= m;
        h ^= h >> 47;
    } else if constexpr (sizeof(intptr_t) == 4) {  // 32bit architecture
        switch (len) {
        case 3:
            h *= m;
            h ^= mem_get<uint32_t>(data) & 0x00FF'FFFFu;
            break;
        case 2:
            h *= m;
            h ^= mem_get<uint16_t>(data);
            break;
        case 1: h *= m; h ^= mem_get<uint8_t>(data);
        }

        // Not fundamentally necessary but
        // mixes the bits around, which is
        // good for getting indices later.

        h ^= h >> 13;
        h *= m;
        h ^= h >> 15;
    } else {
        assert(false);  // not 64bit and not 32bit architecture?!
    }

    return h;
}

#else /* not MURMUR2_HASH1 */

/**********************************************************************
 * Algorithm of the hash functions implemented here:
 * read values 3 by 3, mix them and continue.
 * Return one of the mixed values as the result.
 **********************************************************************/

/** Hash for uint32_t[].
 * @param length: number of uint32_t
 * @param initval: initial value to start the hash computation.
 * @return the hash.
 */
uint32_t hash_computeU32(const uint32_t* data, size_t length, uint32_t initval)
{
    register uint32_t a, b, c, len;
    assert(data);

    /* Set up the internal state */
    a = 0x9e3779b9; /* the golden ratio; an arbitrary value   */
    b = a + length; /* combine with reasonable amount of bits */
    c = initval;    /* the previous hash value                */
    len = length;

    /* handle most of the key, partial unroll of the loop:
     * unroll to keep the processor ALU not disturbed by jumps.
     */
    while (len >= 9) {
        a += data[0];
        b += data[1];
        c += data[2];
        HASH_MIX(a, b, c);

        a += data[3];
        b += data[4];
        c += data[5];
        HASH_MIX(a, b, c);

        a += data[6];
        b += data[7];
        c += data[8];
        HASH_MIX(a, b, c);

        data += 9;
        len -= 9;
    }

    /* handle the last uint32_t's
     */
    switch (len) {
    case 8: b += data[7];
    case 7: a += data[6]; HASH_MIX(a, b, c);

    case 6: c += data[5];
    case 5: b += data[4];
    case 4: a += data[3]; HASH_MIX(a, b, c);

    case 3: c += data[2];
    case 2: b += data[1];
    case 1: a += data[0]; HASH_MIX(a, b, c);

    case 0: /* Finished */
        ;   /* skip     */
    }

    /* report the result */
    return c;
}

/** Hash for uint16_t[].
 * @param length: number of uint16_t
 * @param initval: initial value to start the hash computation.
 * @return the hash.
 */
uint32_t hash_computeU16(const uint16_t* data16, size_t length, uint32_t initval)
{
    register uint32_t a, b, c, len, *data;
    assert(data16);

    a = 0x9e3779b9;
    b = a + length;
    c = initval;
    len = length;
    data = (uint32_t*)data16; /* to read uint32_t */

    /* no unroll to keep the end switch case reasonable
     */
    while (len >= 6) {
        a += data[0];
        b += data[1];
        c += data[2];
        HASH_MIX(a, b, c);
        data += 3; /* read uint32_t: 3 uint32_t = 6 uint16_t */
        len -= 6;  /* len of uint16_t */
    }

    /* handle the last 2 uint32_t's and the last uint16_t = 5 uint16_t
     */
    switch (len) {
    case 5: /* 2 uint32 + 1 uint16 */ c += mem_get<uint16_t>(data + 2);

    case 4: /* 2 uint32 */
        a += data[0];
        b += data[1];
        break;

    case 3: /* 1 uint32 + 1 uint16 */ b += mem_get<uint16_t>(data + 1);

    case 2: /* 1 uint32 */ a += data[0]; break;

    case 1: /* 1 uint16 */ a += mem_get<uint16_t>(data); break;

    case 0: /* Finished */ return c;
    }

    HASH_MIX(a, b, c);
    return c;
}

/** Hash for uint8_t[].
 * @param length: number of uint16_t
 * @param initval: initial value to start the hash computation.
 * @return the hash.
 */
uint32_t hash_computeU8(const uint8_t* data8, size_t length, uint32_t initval)
{
    register uint32_t a, b, c, len, *data;
    assert(data8);

    a = 0x9e3779b9;
    b = a + length;
    c = initval;
    len = length;
    data = (uint32_t*)data8; /* to read uint32_t */

    /* no unroll to keep the switch at a reasonable length
     */
    while (len >= 12) {
        a += data[0];
        b += data[1];
        c += data[2];
        HASH_MIX(a, b, c);
        data += 3; /* 3 uint32_t = 12 uint8_t */
        len -= 12; /* len in uint8_t */
    }

    /* handle the last (11) bytes
     */
    switch (len) {
    case 11: /* 2 uint32 + 1 uint16 + 1 uint8 */
        a += mem_get<uint8_t>(((uint16_t*)data) + 5);
        HASH_MIX(a, b, c);

    case 10: /* 2 uint32 + 1 uint16 */
        a += data[0];
        b += data[1];
        c += mem_get<uint16_t>(data + 2);
        break;

    case 9: /* 2 uint32 + 1 uint8 */ c += mem_get<uint8_t>(data + 2);

    case 8: /* 2 uint32 */
        a += data[0];
        b += data[1];
        break;

    case 7: /* 1 uint32 + 1 uint16 + 1 uint8 */ c += mem_get<uint8_t>(((uint16_t*)data) + 3);

    case 6: /* 1 uint32 + 1 uint16 */
        a += data[0];
        b += mem_get<uint16_t>(data + 1);
        break;

    case 5: /* 1 uint32 + 1 uint8 */ b += mem_get<uint8_t>(data + 1);

    case 4: /* 1 uint32 */ a += data[0]; break;

    case 3: /* 1 uint16 + 1 uint8 */ b += mem_get<uint8_t>(((uint16_t*)data) + 1);

    case 2: /* 1 uint16 */ a += mem_get<uint16_t>(data); break;

    case 1: /* 1 uint8 */ a += mem_get<uint8_t>(data); break;

    case 0: /* Finished */ return c;
    }

    HASH_MIX(a, b, c);
    return c;
}

#endif /* not MURMUR2_HASH1 */
