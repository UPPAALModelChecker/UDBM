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

#include <iomanip>
#include <limits>
#include <sstream>
#include <vector>
#include <cassert>

namespace dbm
{
    /** Valuation: similar to std::valarray, except it treats reference and dynamic values (clocks) specially. */
    template <class S>
    class Valuation
    {
    private:
        size_t staticSize;
        size_t dynamicSize;
        std::vector<S> values;

    public:
        /** Constructor
         * @param size: size of the S valuation vector (number of dimensions).
         * @param dyn: number of dynamic values.
         */
        Valuation(size_t size, size_t dyn = 0): staticSize{size}, dynamicSize{dyn}, values(size + dyn, S{}) {}

        /** Add an amount to all the elements of this vector EXCEPT #0 (reference clock).
         * @param value: amount to add.
         */
        Valuation& operator+=(S value)
        {
            if (value != 0 && !values.empty()) {
                for (auto i = std::next(this->begin()); i != this->end(); ++i)
                    *i += value;
            }
            return *this;
        }
        /** Subtract an amount from all the elements EXCEPT #0.
         * @param value: amount to subtract.
         */
        Valuation& operator-=(S value) { return *this += -value; }

        /** Reset all values (except #0) to specific value */
        void reset(S value = {})
        {
            if (!values.empty())
                std::fill(std::next(this->begin()), this->end(), value);
        }
        const S& last() const
        {
            assert(!values.empty());
            return values.back();
        }

        const S& lastStatic() const
        {
            assert(staticSize > 0);
            return (*this)[staticSize - 1];
        }

        /** @return the delay between this point and the
         * argument point. This has any sense iff
         * argument point = this point + some delay.
         */
        S getDelayTo(const Valuation<S>& arg) const
        {
            if (values.size() <= 1)
                return 0;                   // Only ref clock.
            S delay = arg[1] - (*this)[1];  // Get delay.
#ifndef NDEBUG                              // Check consistency.
            for (size_t i = 1, n = values.size(); i < n; ++i)
                assert(arg[i] - (*this)[i] == delay);
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
                (*this)[staticSize + i] = S{};
            }
        }

        /** Extend the number dynamic values by the specified amount. */
        bool extend(size_t n, S value = {})
        {
            dynamicSize += n;
            values.resize(staticSize + dynamicSize, value);
            return true;
        }

        size_t size() const { return values.size(); }
        auto begin() const { return values.cbegin(); }
        auto end() const { return values.cend(); }
        auto begin() { return values.begin(); }
        auto end() { return values.end(); }

        const S* data() const { return values.data(); }
        S* data() { return values.data(); }

        /** wrap and check */
        S& operator[](size_t at)
        {
            assert(at < values.size());
            return values[at];
        }

        /** wrap and check read-only */
        const S& operator[](size_t at) const
        {
            assert(at < values.size());
            return values[at];
        }
    };

    template <typename S>
    std::ostream& operator<<(std::ostream& os, const Valuation<S>& val)
    {
        os << '(' << val.size() << ")[";
        for (auto& v : val)
            os << ' ' << v;
        return os << " ]";
    }

    using IntValuation = Valuation<int32_t>;
    using DoubleValuation = Valuation<double>;
}  // namespace dbm

#endif  // INCLUDE_DBM_VALUATION_H
