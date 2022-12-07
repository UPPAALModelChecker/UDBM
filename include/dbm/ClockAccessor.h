// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-
////////////////////////////////////////////////////////////////////
//
// This file is a part of the UPPAAL toolkit.
// Copyright (c) 1995 - 2006, Uppsala University and Aalborg University.
// All right reserved.
//
///////////////////////////////////////////////////////////////////

#ifndef INCLUDE_DBM_CLOCKACCESSOR_H
#define INCLUDE_DBM_CLOCKACCESSOR_H

#include "base/inttypes.h"

#include <memory>
#include <string>

namespace dbm
{
    class ClockAccessor;
    using ClockAccessor_uptr = std::unique_ptr<ClockAccessor>;
    /** Class to access clock names in an abstract way. */
    class ClockAccessor
    {
    public:
        virtual ~ClockAccessor() noexcept = default;

        /// @return the name of a given clock (index).
        /// The reference clock corresponds to index 0.
        virtual const std::string& getClockName(cindex_t index) const = 0;
    };

}  // namespace dbm

#endif  // INCLUDE_DBM_CLOCKACCESSOR_H
