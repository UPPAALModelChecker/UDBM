// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-
////////////////////////////////////////////////////////////////////
//
// This file is a part of the UPPAAL toolkit.
// Copyright (c) 2022, Aalborg University.
// All right reserved.
//
///////////////////////////////////////////////////////////////////

#include "dbm/valuation.h"

#include <iomanip>
#include <sstream>

namespace dbm
{
    template <typename S>
    std::ostream& valuation_t<S>::print(std::ostream& os, const ClockAccessor& a) const
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

    template <typename S>
    std::string valuation_t<S>::str(const ClockAccessor& a) const
    {
        std::ostringstream os;
        print(os, a);
        return os.str();
    }

    template <typename S>
    std::ostream& operator<<(std::ostream& os, const valuation_t<S>& val)
    {
        os << '(' << val.size() << ")[";
        for (auto& v : val)
            os << ' ' << v;
        return os << " ]";
    }

    // Explicit template instantiation to provide specific implementations:
    template class valuation_t<int32_t>;
    template class valuation_t<double>;
    template std::ostream& operator<<(std::ostream& os, const valuation_t<int32_t>& val);
    template std::ostream& operator<<(std::ostream& os, const valuation_t<double>& val);
}  // namespace dbm
