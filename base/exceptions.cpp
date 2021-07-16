// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-
////////////////////////////////////////////////////////////////////
//
// This file is a part of the UPPAAL toolkit.
// Copyright (c) 1995 - 2006, Uppsala University and Aalborg University.
// All right reserved.
//
///////////////////////////////////////////////////////////////////

#include <cstdarg>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "base/exceptions.h"

const char* UppaalException::what() const noexcept { return _what; }

SystemException::SystemException(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(_what, 256, fmt, ap);
    va_end(ap);
}

InvalidOptionsException::InvalidOptionsException(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(_what, 256, fmt, ap);
    va_end(ap);
}

RuntimeException::RuntimeException(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(_what, 256, fmt, ap);
    va_end(ap);
}

SuccessorException::SuccessorException(const char* s, const char* c, const char* message):
    RuntimeException(message), state(strdup(s)), channel(strdup(c))
{}

SuccessorException::~SuccessorException()
{
    free((void*)state);
    free((void*)channel);
}
