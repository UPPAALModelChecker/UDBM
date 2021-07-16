// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-
////////////////////////////////////////////////////////////////////
//
// This file is a part of the UPPAAL toolkit.
// Copyright (c) 1995 - 2006, Uppsala University and Aalborg University.
// All right reserved.
//
///////////////////////////////////////////////////////////////////

#ifndef INCLUDE_BASE_EXCEPTIONS_H
#define INCLUDE_BASE_EXCEPTIONS_H

#include <exception>

// Common base class.

class UppaalException : public std::exception
{
protected:
    char _what[256];

public:
    const char* what() const noexcept override;
};

// Specialized exceptions. Unfortunately
// it is difficult to factorize the code
// in the constructor due to the varying
// number of arguments.

class SystemException : public UppaalException
{
public:
    SystemException(const char* fmt, ...);
};

class InvalidOptionsException : public UppaalException
{
public:
    InvalidOptionsException(const char* fmt, ...);
};

class RuntimeException : public UppaalException
{
public:
    RuntimeException(const char* fmt, ...);
};

class SuccessorException : public RuntimeException
{
public:
    const char* state;
    const char* channel;
    SuccessorException(const char* state, const char* channel, const char* message);
    virtual ~SuccessorException();
};

#endif  // INCLUDE_BASE_EXCEPTIONS_H
