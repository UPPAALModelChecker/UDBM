// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-
////////////////////////////////////////////////////////////////////
//
// Test Timer (and base_getTime).
//
// This file is a part of the UPPAAL toolkit.
// Copyright (c) 2020 - 2021, Aalborg University.
// Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
// All right reserved.
//
///////////////////////////////////////////////////////////////////

#include "base/Timer.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>
#include <iostream>
#include <thread>
#include <random>

using namespace std;
using namespace base;

static int64_t compute(int64_t size)
{
    static auto gen = std::mt19937{std::random_device{}()};
    auto dist = std::uniform_int_distribution<int64_t>{0, 100};
    // do something very clever
    int64_t result = 0;
    size *= 10000;
    while (size-- > 0)
        result += dist(gen);
    return result;
}

TEST_CASE("compare timers after computation")
{
    int64_t n = 100;
    double total_time = 0;
    Timer t0;
    int64_t results = 0;
    for (int i = 0; i < n; ++i) {
        Timer t1, t2;
        results += compute(i);
        double elapsed = t2.getElapsed();
        total_time += elapsed;
        CHECK(t1.getElapsed() >= elapsed);
        // cout << "Test: " << t1 << " >= " << elapsed << "s\n";
    }
    CHECK(t0.getElapsed() >= total_time);
    CHECK(results > 0);
    CHECK(results < n * (n - 1) / 2 * 1'000'000);  // just to avoid optimization
    //    cout << "Passed in " << t1 << " >= " << total << "s\n";
}

TEST_CASE("simplest time measurement (fails if nanoseconds are not initialized")
{
    using namespace std::chrono_literals;
    auto t = Timer{true};
    std::this_thread::sleep_for(67ms);
    auto delay = t.getElapsed();
    CHECK(delay >= 0.067);
    CHECK(delay < 0.1);
}

TEST_CASE("measure time with default start")
{
    using namespace std::chrono_literals;
    auto t = Timer{true};
    std::this_thread::sleep_for(67ms);
    auto delay = t.getElapsed();
    CHECK(delay >= 0.067);
    CHECK(delay < 0.1);
    t.pause();
    std::this_thread::sleep_for(67ms);
    t.start();
    std::this_thread::sleep_for(67ms);
    delay = t.getElapsed();
    CHECK(delay >= 2 * 0.067);
    CHECK(delay < 0.15);
}

TEST_CASE("measure time with default pause")
{
    using namespace std::chrono_literals;
    auto t = Timer{false};
    t.start();
    std::this_thread::sleep_for(67ms);
    t.pause();
    auto delay = t.getElapsed();
    CHECK(delay >= 0.067);
    CHECK(delay < 0.1);
    std::this_thread::sleep_for(67ms);
    t.start();
    std::this_thread::sleep_for(67ms);
    t.pause();
    delay = t.getElapsed();
    CHECK(delay >= 2 * 0.067);
    CHECK(delay < 0.15);
}
