// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-
////////////////////////////////////////////////////////////////////
//
// This file is a part of the UPPAAL toolkit.
// Copyright (c) 1995 - 2006, Uppsala University and Aalborg University.
// All right reserved.
//
///////////////////////////////////////////////////////////////////

#include <math.h>
#include "base/doubles.h"

double base_addEpsilon(double value, double epsilon)
{
    if (value == -HUGE_VAL || value == HUGE_VAL) {
        return value;
    }
    double result;
    while ((result = value + epsilon) <= value) {
        epsilon *= 2;
    }
    return result;
}

double base_subtractEpsilon(double value, double epsilon)
{
    if (value == -HUGE_VAL || value == HUGE_VAL) {
        return value;
    }
    double result;
    while ((result = value - epsilon) >= value) {
        epsilon *= 2;
    }
    return result;
}
