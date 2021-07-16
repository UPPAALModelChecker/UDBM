// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-
////////////////////////////////////////////////////////////////////
//
// Filename : Timer.cpp
//
// This file is a part of the UPPAAL toolkit.
// Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
// All right reserved.
//
// v 1.2 reviewed.
// $Id: Timer.cpp,v 1.4 2004/08/17 14:53:31 behrmann Exp $
//
///////////////////////////////////////////////////////////////////

#include "base/Timer.h"

#include <iostream>
#include <cassert>

namespace base
{
    Timer::AutoStartStop::AutoStartStop(Timer& t): timer(t) { timer.start(); }

    Timer::AutoStartStop::~AutoStartStop() { timer.pause(); }

    Timer::Timer(bool running): paused(!running)
    {
        timer = std::chrono::high_resolution_clock::now();
    }

    void Timer::pause()
    {
        assert(!paused);
        if (!paused) {
            paused = true;
            auto end = std::chrono::high_resolution_clock::now();
            nanoseconds += end - timer;
        }
    }

    void Timer::start()
    {
        assert(paused);
        paused = false;
        timer = std::chrono::high_resolution_clock::now();
    }

    double Timer::getElapsed()
    {
        auto time = nanoseconds;
        if (!paused) {
            auto end = std::chrono::high_resolution_clock::now();
            time += end - timer;
        }
        return std::chrono::duration_cast<std::chrono::milliseconds>(time).count() / 1000.0;
    }

}  // namespace base

using namespace std;

ostream& operator<<(ostream& out, base::Timer& t)
{
    // save old & set new crappy C++ iostuff
    ios_base::fmtflags oldFlags = out.flags();
    streamsize oldPrecision = out.precision(3);
    out.setf(ios_base::fixed, ios_base::floatfield);

    // print!
    out << t.getElapsed() << 's';

    // restore crappy C++ iostuff
    out.flags(oldFlags);
    out.precision(oldPrecision);

    return out;
}
