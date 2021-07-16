// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-
////////////////////////////////////////////////////////////////////
//
// Filename : testarray.cpp (base/tests)
//
// Test random access array_t
//
// This file is a part of the UPPAAL toolkit.
// Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
// All right reserved.
//
// $Id: testarray.cpp,v 1.2 2004/02/27 16:36:52 adavid Exp $
//
///////////////////////////////////////////////////////////////////

#ifdef NDEBUG
#undef NDEBUG
#endif

#include "base/array_t.h"
#include "debug/new.h"

#include <iostream>
#include <cstdlib>
#include <ctime>

using namespace std;

static void test(uint32_t size)
{
    base::array_t<uint32_t> arr(rand() % (size + 1));
    size <<= 1;
    // random write
    for (auto i = 1u; i < size; ++i) {
        uint32_t idx = rand() % (size + 1);
        arr.set(idx, idx);
    }
    // print
    if (size < 10) {
        std::cout << arr << std::endl;
    }
    // read all
    for (auto i = 0u; i < size; ++i) {
        assert(arr.get(i) == 0 || arr.get(i) == i);
    }
    // test []
    size = arr.size();
    for (auto i = 0u; i < size; ++i) {
        arr[i]++;
        assert(arr[i] == 1 || arr[i] == i + 1);
    }
}

class Foo
{
public:
    Foo(int a): fii(a) {}
    int fii;
};

static void testFoo(int crash)
{
    Foo foo(4);
    base::pointer_t<Foo> p(crash > 1 ? nullptr : &foo, 1);
    auto n = crash == 1 ? 2 : 1;
    if (crash == 2)
        p->fii++;
    for (auto i = 0; i < n; ++i) {
        p[i].fii++;
    }
    (*p).fii++;
    cout << p->fii << endl;
}

int main(int argc, char* argv[])
{
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " size crash(0..3) [seed]\n";
        return 1;
    }

    auto n = std::atoi(argv[1]);
    auto seed = argc > 3 ? atoi(argv[3]) : time(nullptr);
    cout << "Testing with seed=" << seed << endl;
    srand(seed);

    for (auto i = 0; i < n; ++i) {
        test(i);
    }
    testFoo(std::atoi(argv[2]));

    cout << "Passed\n";
    return 0;
}
