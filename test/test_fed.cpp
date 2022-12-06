// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-
////////////////////////////////////////////////////////////////////
//
// Filename : testfed.cpp
//
// Test of fed_t.
//
// This file is a part of the UPPAAL toolkit.
// Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
// All right reserved.
//
// $Id: testfed.cpp,v 1.21 2005/08/04 13:36:19 adavid Exp $
//
///////////////////////////////////////////////////////////////////

// Tests are always for debugging.

#include "dbm/fed.h"
#include "dbm/gen.h"
#include "dbm/print.h"
#include "debug/utils.h"

#include <algorithm>  // sort
#include <iostream>
#include <random>
#include <ctime>

#include <doctest/doctest.h>

using namespace std;
using namespace dbm;

#ifdef VERBOSE
static inline void SHOW_TEST() { (cout << __FUNCTION__ << '.').flush(); }
static inline void SKIP_TEST() { (cout << __FUNCTION__ << '?').flush(); }
static inline void NO_TEST() { (cout << __FUNCTION__ << '*').flush(); }
#else
static inline void SHOW_TEST() { (cout << '.').flush(); }
static inline void SKIP_TEST() { (cout << '?').flush(); }
static inline void NO_TEST() { (cout << '*').flush(); }
#endif

static inline void PROGRESS() { debug_spin(stderr); }

constexpr auto NB_LOOPS = 250;

// Skip too expensive tests
#define CHEAP dim <= 100 && size <= 100

static auto gen = std::mt19937{};

template <typename T>
T rand_int(T mx)
{
    if (mx < 2)
        return 0;
    auto dist = std::uniform_int_distribution<T>{0, mx - 1};
    return dist(gen);
}

/** Test that a discrete point is in a DBM list
 * @param pt: valuation point
 * @param fed: federation of DBMs.
 * @return true if pt included in dbmList, false otherwise
 */
static bool test_isPointIn(const std::vector<int32_t>& pt, const fed_t& fed)
{
    return std::any_of(fed.begin(), fed.end(),
                       [&pt](const dbm_t& dbm) { return dbm_isPointIncluded(pt.data(), dbm(), pt.size()); });
}

/** Test that a real point is in a DBM list
 * @param pt: valuation point
 * @param fed: federation of DBMs.
 * @return true if pt included in dbmList, false otherwise
 */
static bool test_isPointIn(const std::vector<double>& pt, const fed_t& fed)
{
    return std::any_of(fed.begin(), fed.end(),
                       [&pt](const dbm_t& dbm) { return dbm_isRealPointIncluded(pt.data(), dbm(), pt.size()); });
}

/** Generate a federation of  n DBMs
 * @param dim: dimension of DBMs to generate
 * @param n: number of DBMs to generate.
 * @return n allocated and generated DBMs.
 */
static fed_t test_gen(cindex_t dim, size_t n)
{
    auto result = fed_t{dim};
    if (dim > 0) {
        if (dim == 1) {
            if (n != 0)
                result.setInit();
        } else {
            for (; n != 0; --n) {
                dbm_t dbm(dim);
                dbm_generate(dbm.getDBM(), dim, 1000);
                result.add(dbm);
            }
        }
    }
    return result;
}

/** Pick one DBM from a federation of DBMs.
 * @param fed: the federation.
 * @pre fed.size() > 0, fed.getDimension() > 1, otherwise no DBM to return
 */
static const dbm_t& test_getDBM(const fed_t& fed)
{
    fed_t::const_iterator iter(fed);
    for (size_t size = rand_int(fed.size()); size != 0; --size, ++iter)
        ;
    return *iter;
}

/** Generate a real point in a federation.
 * @param fed: federation.
 * @param pt: where to write the point = double[fed.getDimension()]
 * @pre fed.size() > 0, fed.getDimension() > 1, otherwise no point to return
 */
static bool test_generateRealPoint(std::vector<double>& pt, const fed_t& fed)
{
    return fed.getDimension() > 1 && !fed.isEmpty() &&
           dbm_generateRealPoint(pt.data(), test_getDBM(fed)(), fed.getDimension());
}

/** Generate a discrete point in a federation.
 * @param fed: federation.
 * @param pt: where to write the point = int[fed.getDimension()]
 * @pre fed.size() >0, fed.getDimension() > 1, otherwise no point to return
 */
static bool test_generatePoint(std::vector<int32_t>& pt, const fed_t& fed)
{
    return fed.getDimension() > 1 && !fed.isEmpty() &&
           dbm_generatePoint(pt.data(), test_getDBM(fed)(), fed.getDimension());
}

///< Add some superset/subset DBMs to a federation.
///< @pre fed.getDimension() > 1
///< @param fed: federation to change.
///< @param nb: number of DBMs to add.
static void test_addDBMs(fed_t& fed, size_t nb)
{
    if (fed.size() != 0) {
        for (cindex_t dim = fed.getDimension(); nb != 0; --nb) {
            const raw_t* inList = test_getDBM(fed)();
            dbm_t dbm(dim);
            if ((rand_int(2) & 1) != 0) {
                dbm_generateSuperset(dbm.getDBM(), inList, dim);
            } else {
                dbm_generateSubset(dbm.getDBM(), inList, dim);
            }
            fed.add(dbm);
        }
    }
}

/** Generate arguments for an operation with a federation.
 * @param n: max size of the generated federation.
 * @param fed: base federation.
 */
static fed_t test_genArg(size_t n, const fed_t& fed)
{
    cindex_t dim = fed.getDimension();
    fed_t result(dim);
    n = rand_int(n + 1);
    if (fed.isEmpty()) {
        return test_gen(dim, n);
    }
    for (; n != 0; --n) {
        const dbm_t& inList = test_getDBM(fed);
        dbm_t dbm(dim);
        switch (rand_int(4)) {
        case 0:  // diff
            dbm_generate(dbm.getDBM(), dim, 1000);
            break;
        case 1:  // equal
            dbm = inList;
            break;
        case 2:  // subset
            dbm_generateSubset(dbm.getDBM(), inList(), dim);
            break;
        case 3:  // superset
            dbm_generateSuperset(dbm.getDBM(), inList(), dim);
            break;
        }
        result.add(dbm);
    }
    return result;
}

/** Generate constraints for a federation.
 * @param fed: federation.
 * @param n: number of constraints to generate
 * @param constraints: where to generate
 * @post by constraining fed with constraints the result is not empty (if fed is not empty)
 * @return number of generated constraints (may be < n)
 */
static size_t test_genConstraints(const fed_t& fed, size_t n, constraint_t* constraints)
{
    size_t listSize = fed.size();
    size_t nb = 0;
    cindex_t dim = fed.getDimension();

    if (listSize != 0 && n != 0 && dim > 1) {
        const raw_t* dbm = test_getDBM(fed)();  // pick a DBM
        uint32_t easy = rand_int(n + 1);        // nb of easy constraints 0..n
        uint32_t i, j, k = 0;

        do {      // generate non tightening constraints
            do {  // pick a constraint
                i = rand_int(dim);
                j = rand_int(dim);
            } while (i == j);

            if (dbm[i * dim + j] != dbm_LS_INFINITY) {
                constraints[nb].i = i;
                constraints[nb].j = j;
                constraints[nb].value = dbm[i * dim + j] + rand_int(4); /* loosen a bit */
                nb++;
            }

        } while (++k < easy);

        // generate tightening constraints
        while (nb < n) {
            // pick a constraint
            do {
                i = rand_int(dim);
                j = rand_int(dim);
            } while (i == j);

            constraints[nb].i = i;
            constraints[nb].j = j;

            if (dbm[j * dim + i] == dbm_LS_INFINITY) {
                if (dbm[i * dim + j] == dbm_LS_INFINITY) {
                    // anything will do
                    constraints[nb].value = 1 + rand_int(1000);
                } else if (i != 0 || dbm[i * dim + j] > dbm_LE_ZERO) {
                    constraints[nb].value = dbm[i * dim + j] - rand_int(10);
                    if (i == 0 && constraints[nb].value < dbm_LE_ZERO) {
                        constraints[nb].value = dbm_LE_ZERO;
                    }
                } else {
                    constraints[nb].value = dbm_LE_ZERO;
                }
            } else  // has to take dbm[j,i] into account
            {
                if (dbm[i * dim + j] == dbm_LS_INFINITY) {
                    constraints[nb].value = 2 + rand_int(500) - dbm[j * dim + i];
                } else {
                    raw_t rawRange = dbm[i * dim + j] + dbm[j * dim + i];
                    constraints[nb].value = rand_int(rawRange) + 1 - dbm[j * dim + i];
                }
            }
            ++nb;
        }
    }
    return nb;
}

template <typename It>
void shuffle(It first, It last)
{
    size_t size = std::distance(first, last);
    if (size < 2)
        return;
    for (auto i = size_t{0}; i < size - 1; ++i) {
        auto dist = std::uniform_int_distribution<size_t>{i, size - 1};
        auto idx = dist(gen);
        if (idx != i)
            std::swap(*std::next(first, i), *std::next(first, idx));
    }
}

/** Mix a DBM list: misuse of quicksort.
 * We can swap indices randomly too, but
 * this is simpler (and quite cool).
 * @param dbmList: list of DBMs
 * @pre size <= 2^16
 */
static void test_mix(fed_t& fed)
{
    size_t size = fed.size();
    CHECK(size <= (1u << 16));
    if (size > 0) {
        auto list = std::vector<fdbm_t*>(size);
        auto written = fed.write(list.data());
        CHECK(written == size);
        shuffle(list.begin(), list.end());
        fed.read(list.data(), size);
    }
    CHECK(!fed.hasEmpty());
}

// Test setZero
static void test_setZero(cindex_t dim)
{
    SHOW_TEST();
    auto fed = fed_t{dim};
    fed.setZero();
    CHECK(dim > 0);
    CHECK(fed.size() == 1);
    CHECK(!fed.isEmpty());
    fed_t::const_iterator iter = fed.begin(), e = fed.end();
    CHECK(iter != e);
    CHECK(iter->isZero());
    ++iter;
    CHECK(iter == e);
    fed.nil();
    CHECK(fed.isEmpty());
    fed.setDimension(dim);
    fed.setZero();
    iter = fed_t::const_iterator(fed);
    CHECK(iter != e);
    CHECK(iter->isZero());
    CHECK(iter->getDimension() == fed.getDimension());
    CHECK(fed.getDimension() == dim);
    ++iter;
    CHECK(iter == e);
}

// Test setInit
static void test_setInit(cindex_t dim)
{
    SHOW_TEST();
    auto fed = fed_t{dim};
    fed.setInit();
    CHECK(dim > 0);
    CHECK(fed.size() == 1);
    CHECK(!fed.isEmpty());
    fed_t::const_iterator iter = fed.begin(), e = fed.end();
    CHECK(iter != e);
    CHECK(iter->isInit());
    ++iter;
    CHECK(iter == e);
    fed.nil();
    CHECK(fed.isEmpty());
    fed.setDimension(dim);
    fed.setInit();
    iter = fed_t::const_iterator(fed);
    CHECK(iter != e);
    CHECK(iter->isInit());
    CHECK(iter->getDimension() == fed.getDimension());
    CHECK(fed.getDimension() == dim);
    ++iter;
    CHECK(iter == e);
}

// Test copy -- no real copy, since copy on write!
static void test_copy(cindex_t dim, size_t size)
{
    SHOW_TEST();
    uint32_t k;
    for (k = 0; k < NB_LOOPS; ++k) {
        PROGRESS();
        auto fed1 = fed_t{test_gen(dim, size)};
        auto fed2 = fed1;  // copy constructor
        uint32_t h = fed1.hash();
        CHECK(fed1.getDimension() == dim);
        CHECK(fed1.getDimension() == fed2.getDimension());
        CHECK(fed1.size() == fed2.size());
        CHECK(fed2.hash() == h);
        for (const auto& iter : fed1)
            CHECK(fed2.hasSame(iter));
        fed1.nil();
        fed1 = fed2;  // copy
        CHECK(fed1.hash() == h);
        CHECK(fed1.getDimension() == dim);
        CHECK(fed1.getDimension() == fed2.getDimension());
        CHECK(fed1.size() == fed2.size());
        for (const auto& iter : fed1)
            CHECK(fed2.hasSame(iter));
        // preview on relations
        CHECK(fed1 == fed2);
        CHECK(fed1 >= fed2);
        CHECK(fed1 <= fed2);
        CHECK(fed1.eq(fed2));
        CHECK(fed1.ge(fed1));
        CHECK(fed1.le(fed2));
        fed1.nil();
        fed2.nil();
        if (dim > 0) {
            dbm_t dbm(dim);
            dbm_generate(dbm.getDBM(), dim, 1000);
            fed1 = dbm;              // copy DBM
            fed2.setDimension(dim);  // need to know dim for matrix copy
            fed2 = dbm();            // copy matrix of raw_t
            CHECK(fed1 == fed2);
            CHECK(fed1 == dbm);
            CHECK(fed2.eq(fed1));
            CHECK(fed1.size() == 1);
            CHECK(dbm == *fed_t::const_iterator(fed1));
        }
    }
}

// Test |=
static void test_union(cindex_t dim, size_t size)
{
    SHOW_TEST();
    uint32_t k;
    for (k = 0; k < NB_LOOPS; ++k) {
        PROGRESS();
        fed_t fed1 = test_gen(dim, size);
        fed_t fed2 = test_gen(dim, size);
        fed_t fed3 = fed1;
        uint32_t h = fed1.hash();
        fed1 |= fed2;
        size_t siz = fed1.size();
        fed1 |= fed2;
        CHECK(fed1.size() == siz);  // no DBM added
        CHECK(fed2 <= fed1);
        CHECK(fed2.le(fed1));
        CHECK(fed1.ge(fed2));
        CHECK(fed1 >= fed2);
        CHECK(fed3.hash() == h);  // no side effect with ref etc..
        for (const auto& iter : fed2) {
            CHECK(iter <= fed1);
            fed3 |= iter;
        }
        CHECK(fed1 == fed3);
        CHECK(fed1 >= fed3);
        CHECK(fed1 <= fed3);
        CHECK(fed3 == fed1);
        CHECK(fed1.eq(fed3));
        CHECK(fed1.ge(fed3));
        CHECK(fed1.le(fed3));
        CHECK(fed3.eq(fed1));
        fed1.setDimension(dim);  // empty fed too
        fed1 |= fed2;
        CHECK(fed1 == fed2);
        CHECK(fed1 >= fed2);
        CHECK(fed1 <= fed2);
        CHECK(fed2 == fed1);
        CHECK(fed1.eq(fed2));
        CHECK(fed1.ge(fed2));
        CHECK(fed1.le(fed2));
        CHECK(fed2.eq(fed1));
        fed1.reduce();
        fed2 = test_genArg(size, fed1);
        fed1.removeIncludedIn(fed2);
        for (const auto& iter : fed1)
            CHECK(!(iter <= fed2));
    }
}

// Test +=
static void test_convexUnion(cindex_t dim, size_t size)
{
    SHOW_TEST();
    uint32_t k;
    for (k = 0; k < NB_LOOPS; ++k) {
        PROGRESS();
        fed_t fed1(test_gen(dim, size));
        fed_t fed2(test_gen(dim, size));
        dbm_t dbm(dim);
        uint32_t h1 = fed1.hash();
        uint32_t h2 = fed2.hash();
        dbm_generate(dbm.getDBM(), dim, 1000);
        uint32_t h3 = dbm.hash();
        fed_t fed3 = fed1 + fed2;
        CHECK(h1 == fed1.hash());
        CHECK(h2 == fed2.hash());
        CHECK(fed1 <= fed3);
        CHECK(fed2 <= fed3);
        CHECK(fed3 >= fed1);
        CHECK(fed3 >= fed2);
        CHECK(fed1.le(fed3));
        CHECK(fed2.le(fed3));
        CHECK(fed3.ge(fed1));
        CHECK(fed3.ge(fed2));
        CHECK(fed3.size() == 1);
        fed3 += dbm;
        CHECK(dbm.hash() == h3);
        CHECK(dbm <= fed3);
        fed3 = fed1;
        fed3.convexHull();
        CHECK(fed1.hash() == h1);
        CHECK(fed3.ge(fed1));
        CHECK(fed3 >= fed1);
        CHECK(fed3.size() == 1);
    }
}

// test &= intersection
static void test_intersection(cindex_t dim, size_t size)
{
    /**
     * This test was originally written using floating point clock valuations instead of integers
     * This caused the issues described in tests/test_fp_intersection.cpp. For more information,
     * git blame this comment
     * TODO: Make intersection of DBMs behave properly with floating point numbers
     */
    SHOW_TEST();
    uint32_t k;
    for (k = 0; k < NB_LOOPS; ++k) {
        PROGRESS();
        fed_t fed1(test_gen(dim, size));
        fed_t fed2(test_genArg(size, fed1));
        uint32_t h1 = fed1.hash();
        uint32_t h2 = fed2.hash();
        fed_t fed3 = fed1 & fed2;
        CHECK(fed1.hash() == h1);
        CHECK(fed2.hash() == h2);
        dbm_t dbm2(dim);
        if (!fed2.isEmpty())
            dbm2 = *fed_t::const_iterator(fed2);  // 1st DBM thanks
        uint32_t hd = dbm2.hash();
        fed_t fed4 = dbm2 & fed1;
        CHECK(dbm2.hash() == hd);
        CHECK((fed1 & fed2).eq(fed2 & fed1));
        CHECK(fed3.le(fed1));
        CHECK(fed3.le(fed2));
        CHECK(fed4.le(fed1));
        CHECK(fed4.le(dbm2));
        for (const auto& iter : fed3) {
            CHECK(iter.le(fed1));
            CHECK(iter.le(fed2));
        }
        for (const auto& iter : fed4) {
            CHECK(iter.le(fed1));
            CHECK(iter.le(dbm2));
        }
        if (fed3.isEmpty()) {
            auto pt = std::vector<double>(dim);
            for (int i = 0; i < 500; ++i) {
                // pt in fed1 not in fed2
                if (test_generateRealPoint(pt, fed1)) {
                    CHECK(!fed2.contains(pt));
                    CHECK(!test_isPointIn(pt, fed2));
                }
                // pt in fed2 not in fed1
                if (test_generateRealPoint(pt, fed2)) {
                    CHECK(!fed1.contains(pt));
                    CHECK(!test_isPointIn(pt, fed1));
                }
            }
        }
        if (fed4.isEmpty()) {
            auto pt = std::vector<double>(dim);
            for (int i = 0; i < 500; ++i) {
                // pt in fed1 not in fed2
                if (test_generateRealPoint(pt, fed1))
                    CHECK(!dbm2.contains(pt));
                // pt in fed2 not in fed1
                if (!dbm2.isEmpty() && dbm_generateRealPoint(pt.data(), dbm2(), dim)) {
                    CHECK(!fed1.contains(pt));
                    CHECK(!test_isPointIn(pt, fed1));
                }
            }
        }
    }
}

// test &= contrain
static void test_constrain(cindex_t dim, size_t size)
{
    SHOW_TEST();
    uint32_t k;
    for (k = 0; k < NB_LOOPS; ++k) {
        PROGRESS();
        fed_t fed1 = test_gen(dim, size);
        fed_t fed2 = fed1;
        uint32_t h = fed1.hash();
        auto constraints = std::vector<constraint_t>(3 * dim);
        auto onec = constraint_t{};
        size_t nb = test_genConstraints(fed1, 3 * dim, constraints.data());
        size_t n1 = test_genConstraints(fed1, 1, &onec);
        if (n1 != 0)
            fed1 &= onec;
        bool notEmpty = fed1.constrain(constraints.data(), nb);
        CHECK(notEmpty == !fed1.isEmpty());
        if (n1 != 0) {
            if (!fed1.isEmpty())
                CHECK((fed1 && onec));
            if (!(fed1 && onec))
                CHECK(fed1.isEmpty());
        }
        for (size_t i = 0; i < nb; ++i) {
            if (!fed1.isEmpty())
                CHECK((fed1 && constraints[i]));
            if (!(fed1 && constraints[i]))
                CHECK(fed1.isEmpty());
            for (const auto& iter : fed1) {
                CHECK((iter && constraints[i]));
                if (n1 != 0)
                    CHECK((iter && onec));
            }
        }
        CHECK(fed2.hash() == h);
        for (auto& iter : fed2.as_mutable()) {
            iter.constrain(constraints.data(), nb);
            if (n1 != 0)
                iter &= onec;
        }
        fed2.removeEmpty();  // invariant of fed_t
        CHECK(fed1.hash() == fed2.hash());
        if (fed1 != fed2) {
            cerr << fed1 << endl << fed2 << endl;
            CHECK(fed1 == fed2);
        }
        fed1.reduce();
        test_mix(fed1);
        test_mix(fed2);
        CHECK(fed1 == fed2);
        CHECK(fed2 == fed1);
        CHECK(fed1.eq(fed2));
        CHECK(fed2.eq(fed1));
        CHECK(fed1.hash() == fed2.reduce().hash());
        if (CHEAP) {
            fed2.mergeReduce().expensiveReduce();
            CHECK(fed1.eq(fed2));
            CHECK(fed2.eq(fed1));
            CHECK((fed1 - fed2).isEmpty());
            CHECK((fed2 - fed1).isEmpty());
        }
    }
}

// test up
static void test_up(cindex_t dim, size_t size)
{
    SHOW_TEST();
    uint32_t k;
    for (k = 0; k < NB_LOOPS; ++k) {
        PROGRESS();
        fed_t fed1 = test_gen(dim, size);
        fed_t fed2 = fed1;
        uint32_t h = fed1.hash();
        CHECK(fed2 <= fed1.up());
        if (dim != 0)
            CHECK(fed1.isUnbounded());
        if (!fed1.isUnbounded())
            CHECK(dim == 0);
        CHECK(fed2.hash() == h);
        for (auto& iter2 : fed2.as_mutable())
            iter2.up();
        CHECK(fed1 == fed2);
        CHECK(fed2 == fed1);
        fed1.reduce();
        CHECK(fed1 == fed2);
        CHECK(fed2 == fed1);
        CHECK(fed1.eq(fed2));
        CHECK(fed2.eq(fed1));
        CHECK(fed1.le(fed2));
        CHECK(fed1.ge(fed2));
        CHECK(fed1.hash() == fed2.reduce().hash());
        if (CHEAP) {
            fed2.mergeReduce().expensiveReduce();
            CHECK(fed1.eq(fed2));
            CHECK(fed2.eq(fed1));
            CHECK((fed1 - fed2).isEmpty());
            CHECK((fed2 - fed1).isEmpty());
        }
    }
}

// test down
static void test_down(cindex_t dim, size_t size)
{
    SHOW_TEST();
    uint32_t k;
    for (k = 0; k < NB_LOOPS; ++k) {
        PROGRESS();
        fed_t fed1 = test_gen(dim, size);
        fed_t fed2 = fed1;
        uint32_t h = fed1.hash();
        CHECK(fed2 <= fed1.down());
        CHECK(fed2.hash() == h);
        for (auto& iter2 : fed2.as_mutable())
            iter2.down();
        CHECK(fed1 == fed2);
        CHECK(fed2 == fed1);
        fed1.reduce();
        CHECK(fed1 == fed2);
        CHECK(fed2 == fed1);
        CHECK(fed1.eq(fed2));
        CHECK(fed2.eq(fed1));
        CHECK(fed1.le(fed2));
        CHECK(fed1.ge(fed2));
        CHECK(fed1.hash() == fed2.reduce().hash());
        if (CHEAP) {
            fed2.mergeReduce().expensiveReduce();
            CHECK(fed1.eq(fed2));
            CHECK(fed2.eq(fed1));
            CHECK((fed1 - fed2).isEmpty());
            CHECK((fed2 - fed1).isEmpty());
        }
    }
}

// test freeClock
static void test_freeClock(cindex_t dim, size_t size)
{
    if (dim <= 1) {
        NO_TEST();
    } else  // dim > 1 pre-condition
    {
        SHOW_TEST();
        uint32_t k;
        for (k = 0; k < NB_LOOPS; ++k) {
            PROGRESS();
            fed_t fed1(test_gen(dim, size));
            fed_t fed2 = fed1;
            uint32_t h = fed1.hash();
            cindex_t c = rand_int(dim - 1) + 1;  // not ref clock
            CHECK(fed2 <= fed1.freeClock(c));
            CHECK(fed2.hash() == h);
            for (auto& iter2 : fed2.as_mutable())
                iter2.freeClock(c);
            CHECK(fed1 == fed2);
            CHECK(fed2 == fed1);
            fed1.reduce();
            CHECK(fed1 == fed2);
            CHECK(fed2 == fed1);
            CHECK(fed1.eq(fed2));
            CHECK(fed2.eq(fed1));
            CHECK(fed1.le(fed2));
            CHECK(fed1.ge(fed2));
            CHECK(fed1.hash() == fed2.reduce().hash());
            if (CHEAP) {
                fed2.mergeReduce().expensiveReduce();
                CHECK(fed1.eq(fed2));
                CHECK(fed2.eq(fed1));
                CHECK((fed1 - fed2).isEmpty());
                CHECK((fed2 - fed1).isEmpty());
            }
        }
    }
}

// test updateValue
static void test_updateValue(cindex_t dim, size_t size)
{
    if (dim <= 1) {
        NO_TEST();
    } else  // dim > 1 pre-condition
    {
        SHOW_TEST();
        uint32_t k;
        for (k = 0; k < NB_LOOPS; ++k) {
            PROGRESS();
            fed_t fed1 = test_gen(dim, size);
            fed_t fed2 = fed1;
            fed_t fed3 = fed1;
            uint32_t h = fed1.hash();
            cindex_t c = rand_int(dim - 1) + 1;
            int32_t v = rand_int(100);
            fed1.updateValue(c, v);
            fed2(c) = v;
            CHECK(fed1.hash() == fed2.hash());
            CHECK(fed3.hash() == h);
            CHECK(fed1 == fed2);
            CHECK(fed2 == fed1);
            h = fed1.reduce().hash();
            CHECK(fed1 == fed2);
            CHECK(fed2 == fed1);
            CHECK(fed1.eq(fed2));
            CHECK(fed2.eq(fed1));
            CHECK(fed1.le(fed2));
            CHECK(fed1.ge(fed2));
            CHECK(h == fed2.reduce().hash());
            if (CHEAP) {
                fed2.mergeReduce().expensiveReduce();
                CHECK(fed1.eq(fed2));
                CHECK(fed2.eq(fed1));
                CHECK((fed1 - fed2).isEmpty());
                CHECK((fed2 - fed1).isEmpty());
            }
            for (auto& iter : fed3.as_mutable())
                iter.updateValue(c, v);
            CHECK(h == fed3.reduce().hash());
            CHECK(fed1 == fed3);
            CHECK(fed3 == fed1);
            CHECK(fed1.eq(fed3));
            CHECK(fed3.eq(fed1));
            CHECK(fed1.le(fed3));
            CHECK(fed1.ge(fed3));
            if (CHEAP) {
                fed3.mergeReduce().expensiveReduce();
                CHECK(fed1.eq(fed3));
                CHECK(fed3.eq(fed1));
                CHECK((fed1 - fed3).isEmpty());
                CHECK((fed3 - fed1).isEmpty());
            }
        }
    }
}

// test updateClock
static void test_updateClock(cindex_t dim, size_t size)
{
    if (dim <= 1) {
        NO_TEST();
    } else  // dim > 1 pre-condition
    {
        SHOW_TEST();
        uint32_t k;
        for (k = 0; k < NB_LOOPS; ++k) {
            PROGRESS();
            fed_t fed1 = test_gen(dim, size);
            fed_t fed2 = fed1;
            fed_t fed3 = fed1;
            uint32_t h = fed1.hash();
            cindex_t c1 = rand_int(dim - 1) + 1;
            cindex_t c2 = rand_int(dim - 1) + 1;
            fed1.updateClock(c1, c2);
            fed2(c1) = fed2(c2);
            CHECK(fed3.hash() == h);
            CHECK(fed1 == fed2);
            CHECK(fed2 == fed1);
            CHECK(fed1.eq(fed2));
            CHECK(fed2.eq(fed1));
            CHECK(fed1.le(fed2));
            CHECK(fed1.ge(fed2));
            h = fed1.reduce().hash();
            CHECK(fed1 == fed2);
            CHECK(fed2 == fed1);
            CHECK(h == fed2.reduce().hash());
            if (CHEAP) {
                fed2.mergeReduce().expensiveReduce();
                CHECK(fed1.eq(fed2));
                CHECK(fed2.eq(fed1));
                CHECK((fed1 - fed2).isEmpty());
                CHECK((fed2 - fed1).isEmpty());
            }
            for (auto& iter : fed3.as_mutable())
                iter.updateClock(c1, c2);
            CHECK(fed1 == fed3);
            CHECK(fed3 == fed1);
            CHECK(fed1.eq(fed3));
            CHECK(fed3.eq(fed1));
            CHECK(fed1.le(fed3));
            CHECK(fed1.ge(fed3));
            if (c1 != c2)
                CHECK(h == fed3.reduce().hash());
            if (h != fed3.reduce().hash())
                CHECK(c1 == c2);
            if (CHEAP) {
                fed3.mergeReduce().expensiveReduce();
                CHECK(fed1.eq(fed3));
                CHECK(fed3.eq(fed1));
                CHECK((fed1 - fed3).isEmpty());
                CHECK((fed3 - fed1).isEmpty());
            }
        }
    }
}

// test updateIncrement
static void test_updateIncrement(cindex_t dim, size_t size)
{
    if (dim <= 1) {
        NO_TEST();
    } else  // dim > 1 pre-condition
    {
        SHOW_TEST();
        uint32_t k;
        for (k = 0; k < NB_LOOPS; ++k) {
            PROGRESS();
            fed_t fed1(test_gen(dim, size));
            fed_t fed2 = fed1;
            fed_t fed3 = fed1;
            uint32_t h = fed1.hash();
            cindex_t c = rand_int(dim - 1) + 1;
            int32_t v = rand_int(50);
            fed1.updateIncrement(c, v);
            fed2(c) += v;
            CHECK(fed3.hash() == h);
            CHECK(fed1 == fed2);
            CHECK(fed2 == fed1);
            CHECK(fed1.eq(fed2));
            CHECK(fed2.eq(fed1));
            CHECK(fed1.le(fed2));
            CHECK(fed1.ge(fed2));
            h = fed1.reduce().hash();
            CHECK(fed1 == fed2);
            CHECK(fed2 == fed1);
            CHECK(h == fed2.reduce().hash());
            if (CHEAP) {
                fed2.mergeReduce().expensiveReduce();
                CHECK(fed1.eq(fed2));
                CHECK(fed2.eq(fed1));
                CHECK((fed1 - fed2).isEmpty());
                CHECK((fed2 - fed1).isEmpty());
            }
            for (auto& iter : fed3.as_mutable())
                iter.updateIncrement(c, v);
            CHECK(fed1 == fed3);
            CHECK(fed3 == fed1);
            CHECK(fed1.eq(fed3));
            CHECK(fed3.eq(fed1));
            CHECK(fed1.le(fed3));
            CHECK(fed1.ge(fed3));
            if (v != 0)
                CHECK(h == fed3.reduce().hash());
            if (h != fed3.reduce().hash())
                CHECK(v == 0);
            if (CHEAP) {
                fed3.mergeReduce().expensiveReduce();
                CHECK(fed1.eq(fed3));
                CHECK(fed3.eq(fed1));
                CHECK((fed1 - fed3).isEmpty());
                CHECK((fed3 - fed1).isEmpty());
            }
        }
    }
}

// test update
static void test_update(cindex_t dim, size_t size)
{
    if (dim <= 1) {
        NO_TEST();
    } else  // dim > 1 pre-condition
    {
        SHOW_TEST();
        uint32_t k;
        for (k = 0; k < NB_LOOPS; ++k) {
            PROGRESS();
            fed_t fed1(test_gen(dim, size));
            fed_t fed2 = fed1;
            fed_t fed3 = fed1;
            uint32_t h = fed1.hash();
            cindex_t c1 = rand_int(dim - 1) + 1;
            cindex_t c2 = rand_int(dim - 1) + 1;
            int32_t v = rand_int(50);
            fed1.update(c1, c2, v);
            fed2(c1) = fed2(c2) + v;
            CHECK(fed3.hash() == h);
            CHECK(fed1 == fed2);
            CHECK(fed2 == fed1);
            CHECK(fed1.eq(fed2));
            CHECK(fed2.eq(fed1));
            CHECK(fed1.le(fed2));
            CHECK(fed1.ge(fed2));
            h = fed1.reduce().hash();
            CHECK(fed1 == fed2);
            CHECK(fed2 == fed1);
            CHECK(h == fed2.reduce().hash());
            if (CHEAP) {
                fed2.mergeReduce().expensiveReduce();
                CHECK(fed1.eq(fed2));
                CHECK(fed2.eq(fed1));
                CHECK((fed1 - fed2).isEmpty());
                CHECK((fed2 - fed1).isEmpty());
            }
            for (auto& iter : fed3.as_mutable())
                iter.update(c1, c2, v);
            CHECK(fed1 == fed3);
            CHECK(fed3 == fed1);
            CHECK(fed1.eq(fed3));
            CHECK(fed3.eq(fed1));
            CHECK(fed1.le(fed3));
            CHECK(fed1.ge(fed3));
            if (c1 != c2 || v != 0)
                CHECK(h == fed3.reduce().hash());
            if (h != fed3.reduce().hash()) {
                CHECK(c1 == c2);
                CHECK(v == 0);
            }
            if (CHEAP) {
                fed3.mergeReduce().expensiveReduce();
                CHECK(fed1.eq(fed3));
                CHECK(fed3.eq(fed1));
                CHECK((fed1 - fed3).isEmpty());
                CHECK((fed3 - fed1).isEmpty());
            }
        }
    }
}

// test satisfies
static void test_satisfies(cindex_t dim, size_t size)
{
    if (dim <= 1) {
        NO_TEST();
    } else {
        SHOW_TEST();
        uint32_t k;
        for (k = 0; k < NB_LOOPS; ++k) {
            PROGRESS();
            fed_t fed(test_gen(dim, size));
            std::vector<int32_t> pt(dim);
            for (int c = 0; c < 30; ++c) {
                if (test_generatePoint(pt, fed)) {
                    // all the constraints derived from pt are satisfied
                    for (cindex_t i = 0; i < dim; ++i) {
                        for (cindex_t j = 0; j < dim; ++j)
                            if (i != j) {
                                raw_t val = dbm_bound2raw(pt[i] - pt[j], dbm_WEAK);
                                CHECK(fed.satisfies(i, j, val));
                            }
                    }
                }
            }
        }
    }
}

// test contrain and isEmpty
static void test_constrainEmpty(cindex_t dim, size_t size)
{
    if (dim <= 1 || size == 0) {
        NO_TEST();
    } else {
        SHOW_TEST();
        uint32_t k;
        for (k = 0; k < NB_LOOPS; ++k) {
            PROGRESS();
            fed_t fed(test_gen(dim, size));
            std::vector<int32_t> pt(dim);
            cindex_t i, j;
            // dim > 1 & size > 0 -> possible to generate a non-empty fed
            while (fed.isEmpty())
                fed = test_gen(dim, size);
            if (test_generatePoint(pt, fed)) {
                CHECK(fed.contains(pt));
                CHECK(test_isPointIn(pt, fed));

                // constrain fed so that it contains only this point
                for (i = 0; i < dim; ++i) {
                    for (j = 0; j < dim; ++j)
                        if (i != j) {
                            CHECK(fed.constrain(i, j, pt[i] - pt[j], dbm_WEAK));
                            CHECK(!fed.isEmpty());
                        }
                }
                fed.reduce();
                if (fed.size() != 1) {
                    debug_cppPrintVector(cerr, pt.data(), dim) << endl << fed << endl;
                    CHECK(fed.size() == 1);  // after reduce when fed has only 1 point!
                }
                // now empty the federation
                do {
                    i = rand_int(dim);
                    j = rand_int(dim);
                } while (i == j);
                if (fed.constrain(i, j, pt[i] - pt[j], dbm_STRICT)) {
                    CHECK(!fed.constrain(i, j, pt[i] - pt[j], dbm_STRICT));
                    debug_cppPrintVector(cerr, pt.data(), dim) << endl << fed << endl;
                }
                CHECK(fed.isEmpty());
            }
        }
    }
}

// test isUnbounded
static void test_isUnbounded(cindex_t dim, size_t size)
{
    SHOW_TEST();
    uint32_t k;
    for (k = 0; k < NB_LOOPS; ++k) {
        PROGRESS();
        fed_t fed(test_gen(dim, size));
        bool testu = false;
        for (const auto& iter : fed) {
            if (iter.isUnbounded()) {
                testu = true;
                break;
            }
        }
        CHECK(testu == fed.isUnbounded());
        if (dim != 0)
            CHECK(fed.up().isUnbounded());
        if (!fed.up().isUnbounded())
            CHECK(dim == 0);
    }
}

// additional tests on relations (exact and approx.)
static void test_relation(cindex_t dim, size_t size)
{
    SHOW_TEST();
    uint32_t k;
    for (k = 0; k < NB_LOOPS; ++k) {
        PROGRESS();
        fed_t fed(test_gen(dim, size));
        dbm_t dbm(dim);
        if (fed.size() != 0) {
            fed.reduce();  // otherwise the strict assertions are not valid
            bool strict = dbm_generateSubset(dbm.getDBM(), test_getDBM(fed)(), dim);
            CHECK(dbm <= fed);
            CHECK(dbm.le(fed));
            if (strict) {
                CHECK(dbm < fed);
                CHECK(dbm.lt(fed));
            }
            dbm += fed;  // convex union
            CHECK(dbm >= fed);
            CHECK(dbm.ge(fed));
            if (dbm != fed)
                CHECK(dbm > fed);
            if (dbm <= fed)
                CHECK(dbm == fed);
            if (!dbm.eq(fed))
                CHECK(dbm.gt(fed));
            if (dbm.le(fed))
                CHECK(dbm.eq(fed));
        }
    }
}

// test freeAllUp
static void test_freeAllUp(cindex_t dim, size_t size)
{
    SHOW_TEST();
    uint32_t k;
    for (k = 0; k < NB_LOOPS; ++k) {
        PROGRESS();
        fed_t fed1(test_gen(dim, size));
        fed_t fed2 = fed1;
        uint32_t h = fed1.hash();
        CHECK(fed2 <= fed1.freeAllUp());
        CHECK(up(fed2) <= fed1);
        if (dim != 0)
            CHECK(fed1.isUnbounded());
        if (!fed1.isUnbounded())
            CHECK(dim == 0);
        CHECK(fed2.hash() == h);
        for (auto& iter2 : fed2.as_mutable())
            iter2.freeAllUp();
        CHECK(fed1 == fed2);
        CHECK(fed2 == fed1);
        fed1.reduce();
        CHECK(fed1 == fed2);
        CHECK(fed2 == fed1);
        CHECK(fed1.eq(fed2));
        CHECK(fed2.eq(fed1));
        CHECK(fed1.le(fed2));
        CHECK(fed1.ge(fed2));
        CHECK(fed1.hash() == fed2.reduce().hash());
        if (CHEAP) {
            fed2.mergeReduce().expensiveReduce();
            CHECK(fed1.eq(fed2));
            CHECK(fed2.eq(fed1));
            CHECK((fed1 - fed2).isEmpty());
            CHECK((fed2 - fed1).isEmpty());
        }
    }
}

// test freeAllDown
static void test_freeAllDown(cindex_t dim, size_t size)
{
    SHOW_TEST();
    uint32_t k;
    for (k = 0; k < NB_LOOPS; ++k) {
        PROGRESS();
        fed_t fed1(test_gen(dim, size));
        fed_t fed2 = fed1;
        uint32_t h = fed1.hash();
        CHECK(fed2 <= fed1.freeAllDown());
        CHECK(down(fed2) <= fed1);
        CHECK(fed2.hash() == h);
        for (auto& iter2 : fed2.as_mutable())
            iter2.freeAllDown();
        CHECK(fed1 == fed2);
        CHECK(fed2 == fed1);
        fed1.reduce();
        CHECK(fed1 == fed2);
        CHECK(fed2 == fed1);
        CHECK(fed1.eq(fed2));
        CHECK(fed2.eq(fed1));
        CHECK(fed1.le(fed2));
        CHECK(fed1.ge(fed2));
        CHECK(fed1.hash() == fed2.reduce().hash());
        if (CHEAP) {
            fed2.mergeReduce().expensiveReduce();
            CHECK(fed1.eq(fed2));
            CHECK(fed2.eq(fed1));
            CHECK((fed1 - fed2).isEmpty());
            CHECK((fed2 - fed1).isEmpty());
        }
    }
}

// test freeDown
static void test_freeDown(cindex_t dim, size_t size)
{
    if (dim <= 1) {
        NO_TEST();
    } else {
        SHOW_TEST();
        uint32_t k;
        for (k = 0; k < NB_LOOPS; ++k) {
            PROGRESS();
            fed_t fed1(test_gen(dim, size));
            fed_t fed2 = fed1;
            uint32_t h = fed1.hash();
            cindex_t c = rand_int(dim - 1) + 1;  // not ref clock
            CHECK(fed2 <= fed1.freeDown(c));
            CHECK(fed2.hash() == h);
            for (auto& iter2 : fed2.as_mutable())
                iter2.freeDown(c);
            CHECK(fed1 == fed2);
            CHECK(fed2 == fed1);
            fed1.reduce();
            CHECK(fed1 == fed2);
            CHECK(fed2 == fed1);
            CHECK(fed1.eq(fed2));
            CHECK(fed2.eq(fed1));
            CHECK(fed1.le(fed2));
            CHECK(fed1.ge(fed2));
            CHECK(fed1.hash() == fed2.reduce().hash());
            if (CHEAP) {
                fed2.mergeReduce().expensiveReduce();
                CHECK(fed1.eq(fed2));
                CHECK(fed2.eq(fed1));
                CHECK((fed1 - fed2).isEmpty());
                CHECK((fed2 - fed1).isEmpty());
            }
        }
    }
}

// test freeUp
static void test_freeUp(cindex_t dim, size_t size)
{
    if (dim <= 1) {
        NO_TEST();
    } else {
        SHOW_TEST();
        uint32_t k;
        for (k = 0; k < NB_LOOPS; ++k) {
            PROGRESS();
            fed_t fed1(test_gen(dim, size));
            fed_t fed2 = fed1;
            uint32_t h = fed1.hash();
            cindex_t c = rand_int(dim - 1) + 1;  // not ref clock
            CHECK(fed2 <= fed1.freeUp(c));
            CHECK(fed2.hash() == h);
            for (auto& iter2 : fed2.as_mutable())
                iter2.freeUp(c);
            CHECK(fed1 == fed2);
            CHECK(fed2 == fed1);
            fed1.reduce();
            CHECK(fed1 == fed2);
            CHECK(fed2 == fed1);
            CHECK(fed1.eq(fed2));
            CHECK(fed2.eq(fed1));
            CHECK(fed1.le(fed2));
            CHECK(fed1.ge(fed2));
            CHECK(fed1.hash() == fed2.reduce().hash());
            if (CHEAP) {
                fed2.mergeReduce().expensiveReduce();
                CHECK(fed1.eq(fed2));
                CHECK(fed2.eq(fed1));
                CHECK((fed1 - fed2).isEmpty());
                CHECK((fed2 - fed1).isEmpty());
            }
        }
    }
}

// test relaxUp
static void test_relaxUp(cindex_t dim, size_t size)
{
    SHOW_TEST();
    uint32_t k;
    for (k = 0; k < NB_LOOPS; ++k) {
        PROGRESS();
        fed_t fed1(test_gen(dim, size));
        fed_t fed2 = fed1;
        uint32_t h = fed1.hash();
        CHECK(fed2 <= fed1.relaxUp());
        CHECK(fed2.hash() == h);
        for (auto& iter2 : fed2.as_mutable())
            iter2.relaxUp();
        CHECK(fed1 == fed2);
        CHECK(fed2 == fed1);
        fed1.reduce();
        CHECK(fed1 == fed2);
        CHECK(fed2 == fed1);
        CHECK(fed1.eq(fed2));
        CHECK(fed2.eq(fed1));
        CHECK(fed1.le(fed2));
        CHECK(fed1.ge(fed2));
        CHECK(fed1.hash() == fed2.reduce().hash());
        if (CHEAP) {
            fed2.mergeReduce().expensiveReduce();
            CHECK(fed1.eq(fed2));
            CHECK(fed2.eq(fed1));
            CHECK((fed1 - fed2).isEmpty());
            CHECK((fed2 - fed1).isEmpty());
        }
    }
}

// additional tests for ==
static void test_equal(cindex_t dim, size_t size)
{
    if (dim <= 1) {
        NO_TEST();
    } else {
        SHOW_TEST();
        uint32_t k;
        for (k = 0; k < NB_LOOPS; ++k) {
            PROGRESS();
            fed_t fed1(test_gen(dim, size));
            std::vector<int32_t> pt(dim);
            if (test_generatePoint(pt, fed1)) {
                CHECK(fed1.contains(pt));
                CHECK(test_isPointIn(pt, fed1));
                cindex_t i, j;
                do {
                    i = rand_int(dim);
                    j = rand_int(dim);
                } while (i == j);  // will terminate if dim > 1
                fed_t fed2 = fed1;
                fed2.constrain(i, j, pt[i] - pt[j], dbm_STRICT);
                CHECK(!test_isPointIn(pt, fed2));
                CHECK(!fed2.contains(pt));
                CHECK(fed2 < fed1);
                CHECK(fed1 > fed2);
                CHECK(fed1 != fed2);
            }
            fed_t fed2 = fed1;
            test_addDBMs(fed2, size);
            CHECK(fed2 >= fed1);
            fed1 = fed2;
            CHECK(fed1 == fed2);
            CHECK(fed2 == fed1);
            test_mix(fed1);
            CHECK(fed1 == fed2);
            CHECK(fed2 == fed1);
            fed1.reduce();
            CHECK(fed1 == fed2);
            CHECK(fed2 == fed1);
            CHECK(fed1.eq(fed2));
            CHECK(fed2.eq(fed1));
            CHECK(fed1.le(fed2));
            CHECK(fed1.ge(fed2));
            CHECK(fed1.hash() == fed2.reduce().hash());
            if (CHEAP) {
                fed2.mergeReduce().expensiveReduce();
                CHECK(fed1.eq(fed2));
                CHECK(fed2.eq(fed1));
                CHECK((fed1 - fed2).isEmpty());
                CHECK((fed2 - fed1).isEmpty());
            }
        }
    }
}

// test subtractions
static void test_subtract(cindex_t dim, size_t size)
{
    SHOW_TEST();
    uint32_t k;
    for (k = 0; k < NB_LOOPS; ++k) {
        PROGRESS();
        fed_t fed1(test_gen(dim, size));
        fed_t fed2(test_genArg(size, fed1));
        fed_t fed3 = fed1;
        uint32_t h1 = fed1.hash();
        uint32_t h2 = fed2.hash();
        fed_t fed4 = fed1 & fed2;
        std::vector<int> pt(dim);
        CHECK(fed4.le(fed1));
        CHECK(fed4.le(fed2));
        CHECK((fed4 - fed1).isEmpty());
        CHECK((fed4 - fed2).isEmpty());
        fed_t s12 = fed1 - fed2;
        fed_t s14 = fed1 - fed4;
        if (!s12.eq(s14)) {
            cerr << s12.reduce() << endl << s14.reduce() << endl;
            REQUIRE(s12.eq(s14));
        }
        fed_t s21 = fed2 - fed1;
        fed_t s24 = fed2 - fed4;
        if (!s21.eq(s24)) {
            cerr << s21.reduce() << endl << s24.reduce() << endl;
            REQUIRE(s21.eq(s24));
        }
        fed_t u1 = fed1 | fed2;
        fed_t u2 = s12 | s21 | fed4;
        if (!u1.eq(u2)) {
            cerr << u1.reduce() << endl << u2.reduce() << endl;
            REQUIRE(u1.eq(u2));
        }
        s12.nil();
        s14.nil();
        s21.nil();
        s24.nil();
        u1.nil();
        u2.nil();
        CHECK(fed1.hash() == h1);
        CHECK(fed2.hash() == h2);
        fed1 -= fed2;
        CHECK(fed3.hash() == h1);
        for (const auto& iter : fed2)
            fed3 -= iter;
        CHECK(fed3.eq(fed1));
        for (int count = 0; count < 500; ++count) {
            if (test_generatePoint(pt, fed1)) {  // points in fed1 not in fed2
                CHECK(test_isPointIn(pt, fed1));
                CHECK(fed1.contains(pt));
                CHECK(!test_isPointIn(pt, fed2));
                CHECK(!fed2.contains(pt));
            }
            if (test_generatePoint(pt, fed2)) {  // points in fed2 not in fed1
                CHECK(test_isPointIn(pt, fed2));
                CHECK(fed2.contains(pt));
                CHECK(!test_isPointIn(pt, fed1));
                CHECK(!fed1.contains(pt));
            }
            if (test_generatePoint(pt, fed4)) {  // points in fed4 not in fed1
                CHECK(test_isPointIn(pt, fed4));
                CHECK(fed4.contains(pt));
                CHECK(!test_isPointIn(pt, fed1));
                CHECK(!fed1.contains(pt));
            }
        }
    }
}

// test predt
static void test_predt(cindex_t dim, size_t size)
{
    if (dim < 1) {
        NO_TEST();
    } else {
        SHOW_TEST();
        uint32_t k;
        for (k = 0; k < NB_LOOPS; ++k) {
            PROGRESS();
            fed_t good(test_gen(dim, size));
            fed_t bad(test_genArg(size, good));
            uint32_t hg = good.hash();
            uint32_t hb = bad.hash();
            fed_t p = predt(good, bad);
            CHECK(good.hash() == hg);
            CHECK(bad.hash() == hb);
            CHECK((p & bad).isEmpty());
            if (!good.le(bad))
                CHECK(!(p & good).isEmpty());
            if ((p & good).isEmpty())
                CHECK(good.le(bad));
            CHECK(p.le(down(good) - bad));
            if (bad.isEmpty())
                CHECK(p.eq(down(good)));
            if (!p.eq(down(good)))
                CHECK(!bad.isEmpty());
        }
    }
}

// test different reduce methods.
static void test_reduce(cindex_t dim, size_t size)
{
    if (dim < 1) {
        NO_TEST();
    } else {
        SHOW_TEST();
        uint32_t k;
        for (k = 0; k < NB_LOOPS; ++k) {
            PROGRESS();
            fed_t f1(test_gen(dim, size));
            fed_t f2(test_genArg(size, f1));
            f1.append(f2);

            (f2 = f1).setMutable();
            CHECK(f2.reduce().eq(f1));

            (f2 = f1).setMutable();
            CHECK(f2.expensiveReduce().eq(f1));

            (f2 = f1).setMutable();
            CHECK(f2.mergeReduce().eq(f1));

            (f2 = f1).setMutable();
            CHECK(f2.convexReduce().eq(f1));

            (f2 = f1).setMutable();
            CHECK(f2.expensiveConvexReduce().eq(f1));

            (f2 = f1).setMutable();
            CHECK(f2.partitionReduce().eq(f1));
        }
    }
}

static void test(int dim, int size)
{
    test_setZero(dim);
    test_setInit(dim);
    test_copy(dim, size);
    test_union(dim, size);
    test_convexUnion(dim, size);
    test_intersection(dim, size);
    test_constrain(dim, size);
    test_up(dim, size);
    test_down(dim, size);
    test_freeClock(dim, size);
    test_updateValue(dim, size);
    test_updateClock(dim, size);
    test_updateIncrement(dim, size);
    test_update(dim, size);
    test_satisfies(dim, size);
    test_constrainEmpty(dim, size);
    test_isUnbounded(dim, size);
    test_relation(dim, size);
    test_freeUp(dim, size);
    test_freeAllUp(dim, size);
    test_freeDown(dim, size);
    test_freeAllDown(dim, size);
    test_relaxUp(dim, size);
    test_equal(dim, size);
    test_subtract(dim, size);
    test_predt(dim, size);
    test_reduce(dim, size);
}

TEST_CASE("Federation")
{
    int start, end, size;
    unsigned int seed;

    SUBCASE("start=1 end=1 size=7 random seed")
    {
        start = 1;
        end = 7;
        size = 7;
        seed = std::random_device{}();
    }
    gen.seed(seed);
    if (size < 1) {
        cerr << "Minimum size=1 taken\n";
        size = 1;
    }
    if (start < 1) {
        cerr << "Minimum dimension=1 taken\n";
        start = 1;
    }

    /* Print the seed for the random generator
     * to be able to repeat a failed test.
     */
    cout << "Test with seed=" << seed << endl;

    for (int i = start; i <= end; ++i) {
        cout << i << ": ";
        test(i, size);
        (cout << " \n").flush();
        if ((rand_int(2) & 1) != 0)
            cleanUp();
    }
}
