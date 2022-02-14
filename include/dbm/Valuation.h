// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-
////////////////////////////////////////////////////////////////////
//
// Filename : Valuation.h
//
// IntValuation & DoubleValuation
//
// This file is a part of the UPPAAL toolkit.
// Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
// All right reserved.
//
// $Id: Valuation.h,v 1.2 2005/04/25 16:38:27 behrmann Exp $
//
///////////////////////////////////////////////////////////////////

#ifndef INCLUDE_DBM_VALUATION_H
#define INCLUDE_DBM_VALUATION_H

#include "dbm/ClockAccessor.h"
#include "base/pointer.h"

#include <iomanip>
#include <limits>
#include <sstream>

namespace dbm
{
    /** Valuation: like a fixed vector of scalars S with
     * some basic operations on it.
     * We don't use array_t because we don't want to
     * resize but we need basic memory alloc/dealloc.
     */
    template <class S>
    class Valuation : public base::pointer_t<S>
    {
    private:
        size_t dynamicSize;
        size_t staticSize;
        // S* dynamic;
    public:
        /** Constructor
         * @param size: size of the S valuation vector,
         * or dimension.
         */
        Valuation(size_t size, size_t dyn = 0):
            base::pointer_t<S>(new S[size + dyn], dyn + size), dynamicSize(dyn), staticSize(size)
        {
            base::pointer_t<S>::reset();
        }
        /** Copy constructor
         * @param original: vector to copy
         */
        Valuation(const Valuation<S>& original):
            base::pointer_t<S>(new S[original.staticSize + original.dynamicSize],
                               original.staticSize + original.dynamicSize),
            dynamicSize(original.dynamicSize), staticSize(original.staticSize)
        {
            this->copyFrom(original);
        }

        ~Valuation() { delete[] this->data; }

        /**
         * Copy the elements of the original vector.
         * It is dangerous not to have this operator.
         * @pre original.size() == size().
         */
        Valuation<S>& operator=(const Valuation<S>& original)
        {
            base::pointer_t<S>::copyFrom(original);
            return *this;
        }

        /** Add value to all the elements of this vector
         * EXCEPT element 0 (reference clock).
         * @param value: element to add.
         */
        Valuation<S>& operator+=(S value)
        {
            if (value != 0) {
                for (S* i = this->begin() + 1; i < this->end(); ++i) {
                    *i += value;
                }
            }
            return *this;
        }
        void reset() { std::fill(this->begin(), this->end(), 0); }
        S last() const
        {
            assert(base::pointer_t<S>::size() > 0);
            return (*this)[base::pointer_t<S>::size() - 1];
        }

        S lastStatic() const
        {
            assert(staticSize > 0);
            return (*this)[staticSize - 1];
        }

        /** Subtract value to all the elements of this vector.
         * @param value: element to subtract.
         */
        Valuation<S>& operator-=(S value) { return *this += -value; }

        /** @return the delay between this point and the
         * argument point. This has any sense iff
         * argument point = this point + some delay.
         */
        S getDelayTo(const Valuation<S>& arg) const
        {
            if (base::pointer_t<S>::size() <= 1) {
                return 0;  // Only ref clock.
            }
            S delay = arg[1] - (*this)[1];  // Get delay.
#ifndef NDEBUG                              // Check consistency.
            size_t n = base::pointer_t<S>::size();
            for (size_t i = 1; i < n; ++i) {
                assert(arg[i] - (*this)[i] == delay);
            }
#endif
            return delay;
        }

        std::ostream& print(std::ostream& os, const ClockAccessor& a) const
        {
            os << "[ " << std::setprecision(std::numeric_limits<S>::digits10 + 1);
            cindex_t id = 0;
            for (auto& v : *this) {
                auto name = a.getClockName(id++);
                if (name[0] != '#')
                    os << name << "=" << v << " ";
            }
            return os << "]";
        }

        /** String representation of the valuation in a more
         * user friendly manner than just a vector of numbers.
         */
        std::string toString(const ClockAccessor& a) const
        {
            std::ostringstream os;
            print(os, a);
            return os.str();
        }

        void copyFrom(const Valuation<S>& src)
        {
            assert(src.staticSize == staticSize);
            assert(src.dynamicSize <= dynamicSize);
            // Copy the dynamic parts in common
            std::copy(src.begin(), src.end(), this->begin());
            for (size_t i = src.dynamicSize; i < dynamicSize; ++i) {
                (*this)[staticSize + i] = 0;
            }
        }

        bool extend(size_t n)
        {
            S* buf = new S[n + dynamicSize + staticSize];

            std::copy(base::pointer_t<S>::begin(), base::pointer_t<S>::end(), buf);
            delete[] base::pointer_t<S>::data;
            // dynamicSize += n;
            std::fill_n(buf + staticSize + dynamicSize, n, 0);
            dynamicSize += n;
            this->setData(buf, dynamicSize + staticSize);
            return true;
        }
    };

    using IntValuation = Valuation<int32_t>;
    using DoubleValuation = Valuation<double>;

    /** Overload += for automatic cast. */
    /*    static inline
        DoubleValuation& operator += (DoubleValuation& d, int32_t v) {
            return d += (double)v;
        }

        static inline
        DoubleValuation& operator += (DoubleValuation& d, double v) {
            return d += v;
        }*/
}  // namespace dbm

#endif  // INCLUDE_DBM_VALUATION_H
