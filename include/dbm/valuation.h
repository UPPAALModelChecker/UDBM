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

#include <iosfwd>
#include <limits>
#include <vector>
#include <cassert>

namespace dbm
{
    /** Valuation: similar to std::valarray, except it treats reference and dynamic values (clocks) specially. */
    template <typename S>
    class valuation_t
    {
    private:
        size_t static_size;
        size_t dynamic_size;
        std::vector<S> values;

    public:
        /** Constructor
         * @param size: size of the S valuation vector (number of dimensions).
         * @param dyn: number of dynamic values.
         */
        valuation_t(size_t size, size_t dyn = 0): static_size{size}, dynamic_size{dyn}, values(size + dyn) {}
        valuation_t(const std::initializer_list<S>& list): static_size{list.size()}, dynamic_size{0}, values{list} {}

        valuation_t(const valuation_t& other) { *this = other; }
        valuation_t(valuation_t&&) noexcept = default;

        valuation_t& operator=(const valuation_t<S>& src)
        {
            assert(src.static_size == static_size);
            assert(src.dynamic_size <= dynamic_size);
            std::copy(src.begin(), src.end(), begin_mutable());                           // copy the common values
            std::fill(std::next(begin_mutable(), src.dynamic_size), end_mutable(), S{});  // reset remaining
            return *this;
        }

        /** Add an amount to all the elements of this vector EXCEPT #0 (reference clock).
         * @param value: amount to add.
         */
        valuation_t& operator+=(S value)
        {
            if (value != 0 && values.size() > 1)
                for (auto i = std::next(begin_mutable()); i != end_mutable(); ++i)
                    *i += value;
            return *this;
        }
        /** Subtract an amount from all the elements EXCEPT #0.
         * @param value: amount to subtract.
         */
        valuation_t& operator-=(S value) { return *this += -value; }

        /** Reset all values (except #0) to specific value */
        void reset(S value = {})
        {
            if (values.size() > 1)
                std::fill(std::next(begin_mutable()), end_mutable(), value);
        }
        const S& back() const
        {
            assert(!values.empty());
            return values.back();
        }

        S& back()
        {
            assert(!values.empty());
            return values.back();
        }

        const S& back_static() const
        {
            assert(static_size > 0);
            return (*this)[static_size - 1];
        }

        /** @return the delay between this point and the
         * argument point. This has any sense iff
         * argument point = this point + some delay.
         */
        S get_delay_to(const valuation_t<S>& arg) const
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

        std::ostream& print(std::ostream& os, const ClockAccessor& a) const;

        /** String representation of the valuation in a more
         * user friendly manner than just a vector of numbers.
         */
        std::string str(const ClockAccessor& a) const;

        /** Extend the number dynamic values by the specified amount. */
        bool extend(size_t n, S value = {})
        {
            dynamic_size += n;
            values.resize(static_size + dynamic_size, value);
            return true;
        }

        size_t size() const { return values.size(); }
        auto begin() const { return values.cbegin(); }
        auto end() const { return values.cend(); }
        auto begin_mutable() { return values.begin(); }
        auto end_mutable() { return values.end(); }

        operator const std::vector<S>&() const { return values; }
        operator const S*() const { return values.data(); }
        auto& get_mutable() { return values; }

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
    valuation_t(const std::initializer_list<S>&) -> valuation_t<S>;

    template <typename S>
    std::ostream& operator<<(std::ostream& os, const valuation_t<S>& val);

    using valuation_int = valuation_t<int32_t>;
    using valuation_fp = valuation_t<double>;
}  // namespace dbm

#endif  // INCLUDE_DBM_VALUATION_H
