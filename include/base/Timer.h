// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-
////////////////////////////////////////////////////////////////////
//
// Filename : Timer.h (base)
//
// This file is a part of the UPPAAL toolkit.
// Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
// All right reserved.
//
// v 1.3 reviewed.
// $Id: Timer.h,v 1.6 2004/04/02 22:50:43 behrmann Exp $
//
///////////////////////////////////////////////////////////////////

#ifndef INCLUDE_BASE_TIMER_H
#define INCLUDE_BASE_TIMER_H

#include <iosfwd>
#include <chrono>

namespace base
{
    /** A simple timer to measure CPU time.
     */
    class Timer
    {
    public:
        explicit Timer(bool running = true);

        /**
         * Access to CPU time. Returns the CPU time consumed since the
         * last call of this method or since initialization of the
         * object if the method has not been called before.
         *
         * @returns CPU time in seconds.
         */
        double getElapsed();

        void pause();
        void start();

        class AutoStartStop
        {
        public:
            explicit AutoStartStop(Timer&);
            ~AutoStartStop();

        private:
            Timer& timer;
        };

    private:
        std::chrono::nanoseconds nanoseconds{0};
        bool paused = false;
        std::chrono::system_clock::time_point timer;
    };
}  // namespace base

/**
 * Output operator for Timer class. Writes the consumed CPU time to \a
 * out. Calls \c getElapsed() internally, so the timer is reset.  Up
 * to 3 decimals are printed.
 */
std::ostream& operator<<(std::ostream& out, base::Timer& t);

#endif  // INCLUDE_BASE_TIMER_H
