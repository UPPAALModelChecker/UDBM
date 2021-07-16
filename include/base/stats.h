// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-
////////////////////////////////////////////////////////////////////
//
// Filename : stats.h
//
// General statistics meant for experiments but not in the release
// version of UPPAAL.
//
// This file is a part of the UPPAAL toolkit.
// Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
// All right reserved.
//
// $Id: $
//
///////////////////////////////////////////////////////////////////

#ifndef INCLUDE_BASE_STATS_H
#define INCLUDE_BASE_STATS_H

#ifdef SHOW_STATS

#include "hash/tables.h"

namespace base
{
    class Stats
    {
    public:
        Stats() {}
        ~Stats();

        void count(const char* statName, const char* subStat = NULL);

        struct stat_t
        {
            stat_t(const char* statName, stat_t* nxt): name(statName), counter(1), next(nxt) {}
            ~stat_t() { delete next; }

            const char* name;
            int64_t counter;
            stat_t* next;
        };

        struct entry_t : public uhash::TableSingle<entry_t>::Bucket_t
        {
            entry_t(const char* name, stat_t* next = NULL): stat(name, next) {}

            stat_t stat;  // Main stat, with list of sub-stats.
        };

    private:
        uhash::TableSingle<entry_t> table;
    };

    extern Stats stats;
}  // namespace base

// Typical way of using stats:

#define RECORD_STAT()        base::stats.count(__PRETTY_FUNCTION__, NULL)
#define RECORD_SUBSTAT(NAME) base::stats.count(__PRETTY_FUNCTION__, NAME)
#define RECORD_NSTAT(ROOT, NAME)   \
    base::stats.count(ROOT, NULL); \
    base::stats.count(ROOT, NAME)

#else

#define RECORD_STAT()
#define RECORD_SUBSTAT(NAME)
#define RECORD_NSTAT(ROOT, NAME)

#endif  // SHOW_STATS

#endif  // INCLUDE_BASE_STATS_H
