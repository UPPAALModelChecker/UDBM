// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-
////////////////////////////////////////////////////////////////////
//
// Filename : pointer.h (base)
//
// This file is a part of the UPPAAL toolkit.
// Copyright (c) 1995 - 2000, Uppsala University and Aalborg University.
// All right reserved.
//
// $Id: pointer.h,v 1.7 2005/04/22 15:20:10 adavid Exp $
//
///////////////////////////////////////////////////////////////////

#ifndef INCLUDE_BASE_POINTER_H
#define INCLUDE_BASE_POINTER_H

#include "base/intutils.h"
#include "hash/compute.h"

#include <iostream>
#include <memory>
#include <cassert>

namespace base
{
    /** A simple reference with maximal capacity for access checks.
     * This is a pointer to some bounded memory. array_t is defined
     * too, but pointer_t has no memory management, it is only a reference.
     * Accesses ptr[i] are checked. Main purpose is debugging + simple
     * iteration. std::vector does not provide these debugging capabilities.
     * Everything is assumed to be 32 bits aligned
     * so this wrapper is obviously not designed
     * for int8 or int16 types.
     */
    template <class T>
    class pointer_t
    {
        // check assumption on valid data types
        static_assert((sizeof(T) & 3) == 0 && sizeof(T) > 0);

    protected:
        T* data{nullptr}; /**< data pointed                       */
        size_t capa{0};   /**< size of data, important for checks */
    public:
        /** Default constructor
         */
        pointer_t() = default;

        /** Constructor:
         * @param ptr: pointer to wrap.
         * @param max: number of elements
         * pointed by ptr.
         */
        pointer_t(T* ptr, size_t max): data{ptr}, capa{max} {}

        /** Pointer equality testing.
         * It is ambiguous to define a == operator since
         * it could be interpreted as pointer or content testing.
         */
        bool isSamePointerAs(const pointer_t<T>& other) const
        {
            return begin() == other.begin() && size() == other.size();
        }

        /** wrap and check ptr[at]
         */
        T& operator[](size_t at)
        {
            assert(at < capa);
            assert(data);
            return data[at];
        }

        /** wrap and check read only ptr[at]
         */
        const T& operator[](size_t at) const
        {
            assert(at < capa);
            assert(data);
            return data[at];
        }

        /** wrap and check ptr->something
         */
        T* operator->()
        {
            assert(data);
            return data;
        }

        /** wrap and check *ptr
         */
        T& operator*()
        {
            assert(data);
            return *data;
        }

        /** size of pointed area in T nb of elements
         */
        size_t size() const { return capa; }

        /** reset all pointed elements
         */
        void reset()
        {
            assert(capa == 0 || data);
            std::fill(begin(), end(), T{});
        }

        /** @return a hash value.
         */
        uint32_t hash() const
        {
            return hash_computeU32((uint32_t*)data, intSizeOfA(T, capa), capa);
        }

        /** copy all pointed elements from a pointer.
         * @param src: compatible pointer
         * @pre sizes are the same.
         */
        void copyFrom(const pointer_t<T>& src)
        {
            assert(capa == src.capa);
            assert(capa == 0 || (data && src.data));
            std::copy(src.begin(), src.end(), data);
        }

        void setData(T* data, size_t capa)
        {
            this->data = data;
            this->capa = capa;
        }

        /** Simple iteration.
         */
        const T* begin() const
        {
            assert(capa == 0 || data);
            return data;
        }
        const T* end() const
        {
            assert(capa == 0 || data);
            return data + capa;
        }
        T* begin()
        {
            assert(capa == 0 || data);
            return data;
        }
        T* end()
        {
            assert(capa == 0 || data);
            return data + capa;
        }

        /** Reading data.
         */
        const T* operator()() const { return data; }
    };

    /// Overload ostream <<.
    template <class T>
    std::ostream& operator<<(std::ostream& os, const pointer_t<T>& p)
    {
        os << '(' << p.size() << ")[";
        for (const T* i = p.begin(); i != p.end(); ++i) {
            os << ' ' << *i;
        }
        return os << " ]";
    }

}  // namespace base

#endif
