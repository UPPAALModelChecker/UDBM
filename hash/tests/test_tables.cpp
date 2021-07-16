// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-
////////////////////////////////////////////////////////////////////
//
// Filename : testtables.cpp (hash/tests)
//
// Test hash tables.
//
// This file is a part of the UPPAAL toolkit.
// Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
// All right reserved.
//
// $Id: testtables.cpp,v 1.2 2004/02/27 16:36:53 adavid Exp $
//
///////////////////////////////////////////////////////////////////

#ifdef NDEBUG
#undef NDEBUG
#endif

#include "hash/tables.h"
#include "debug/new.h"

#include <iostream>
#include <cstdlib>

using namespace std;

struct SBucket_t;
struct DBucket_t;
typedef uhash::TableSingle<SBucket_t> SParent;
typedef uhash::TableDouble<DBucket_t> DParent;

struct SBucket_t : public SParent::Bucket_t
{
    uint32_t data;
};
struct DBucket_t : public DParent::Bucket_t
{
    uint32_t data;
};

class STable : public SParent
{
public:
    STable(): SParent(2, false) {}
    ~STable() { resetDelete(); }
    bool insert(uint32_t i)
    {
        SBucket_t** root = getAtBucket(i);
        SBucket_t* bucket = *root;
        while (bucket) {
            if (bucket->info == i && bucket->data == i)
                return false;
            bucket = bucket->getNext();
        }
        bucket = new SBucket_t;
        bucket->link(root);
        bucket->info = i;
        bucket->data = i;
        incBuckets();
        if (getNbBuckets() == 100000) {
            disableRehash();
        } else if (getNbBuckets() == 300000) {
            enableRehash();
        }
        return true;
    }
};

class DTable : public DParent
{
public:
    DTable(): DParent(2, false) {}
    ~DTable() { resetDelete(); }
    bool insert(uint32_t i)
    {
        DBucket_t** root = getAtBucket(i);
        DBucket_t* bucket = *root;
        while (bucket) {
            if (bucket->info == i && bucket->data == i)
                return false;
            bucket = bucket->getNext();
        }
        bucket = new DBucket_t;
        bucket->link(root);
        bucket->info = i;
        bucket->data = i;
        incBuckets();
        if (getNbBuckets() == 100000) {
            disableRehash();
        } else if (getNbBuckets() == 300000) {
            enableRehash();
        }
        return true;
    }
};

static void test(uint32_t size)
{
    STable table1;
    DTable table2;
    uint32_t i;
    for (i = 0; i < size; ++i) {
        assert(table1.insert(i));
        assert(table2.insert(i));
    }
    table1.resetDelete();
    table2.resetDelete();
    for (i = 0; i < size; ++i) {
        assert(table1.insert(i));
        assert(table2.insert(i));
    }
    for (i = 0; i < size; ++i) {
        assert(!table1.insert(i));
        assert(!table2.insert(i));
    }
    base::Enumerator<SBucket_t> enum1 = table1.getEnumerator();
    base::Enumerator<DBucket_t> enum2 = table2.getEnumerator();
    for (i = 0; i < size; ++i) {
        assert(enum1.getNext());
        assert(enum2.getNext());
    }
    assert(!enum1.getNext());
    assert(!enum2.getNext());
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " size\n";
        return 1;
    }

    uint32_t n = atoi(argv[1]);
    // for(uint32_t i = 0 ; i < n ; ++i)
    // test(i);
    test(n);

    cout << "Passed\n";
    return 0;
}
