// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-
////////////////////////////////////////////////////////////////////
//
// Filename : testfeddbm.cpp
//
// Test dbm_t (fed.h)
//
// This file is a part of the UPPAAL toolkit.
// Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
// All right reserved.
//
// $Id: test_fed_dbm.cpp,v 1.8 2005/05/31 19:35:14 adavid Exp $
//
///////////////////////////////////////////////////////////////////

#include "dbm/fed.h"
#include "dbm/gen.h"
#include "dbm/print.h"
#include "dbm/valuation.h"
#include "debug/utils.h"

#include <random>

#include <doctest/doctest.h>

using namespace std;
using namespace dbm;

// Range for DBM generation
constexpr auto MAXRANGE = 10000;
static auto gen = std::mt19937{};
static auto dist = std::uniform_int_distribution{10, MAXRANGE + 10};
static inline auto RANGE() { return dist(gen); }

// Progress
static inline void PROGRESS() { debug_spin(stderr); }

// DBM generation
static inline void GEN(dbm::writer dbm) { dbm_generate(dbm, dbm.get_dim(), RANGE()); }

// New DBM
static inline dbm::writer NEW(cindex_t dim) { return {new raw_t[dim * dim], dim}; }

// Delete DBM
static inline void FREE(const raw_t* ptr) { delete[] ptr; }

// Check pointer
static inline void POINTER(dbm_t& dbm) { CHECK(dbm.isEmpty() == (dbm() == nullptr)); }

// Invalidate matrix
static inline void INVALIDATE(dbm::writer dbm) { debug_randomize(dbm, dbm.get_dim() * dbm.get_dim()); }

// Equality
static inline void EQ(const dbm_t& A, const dbm_t& B)
{
    CHECK(A == B);
    CHECK(A.relation(B) == base_EQUAL);
}
static inline void EQ(const dbm_t& A, const raw_t* B, cindex_t dim)
{
    CHECK(A == B);
    CHECK(A.relation(B, dim) == base_EQUAL);
}

// Not equality
static inline void NEQ(const dbm_t& A, const dbm_t& B)
{
    CHECK(A != B);
    CHECK(A.relation(B) != base_EQUAL);
}
static inline void NEQ(const dbm_t& A, const raw_t* B, cindex_t dim)
{
    CHECK(A != B);
    CHECK(A.relation(B, dim) != base_EQUAL);
}

static void test(const cindex_t dim)
{
    auto dbm = NEW(dim);
    auto dbm2 = NEW(dim);
    auto cnstr = std::vector<constraint_t>(dim * dim);
    auto lower = std::vector<int32_t>(dim);
    auto upper = std::vector<int32_t>(dim);
    auto pt = valuation_int{dim};
    // coverage
    bool c1 = false, c2 = false, c4 = false;
    bool c5 = false, c6 = false, c7 = false;
    for (int kLoop = 0; kLoop < 23; ++kLoop) {
        auto a = dbm_t{};     // default
        auto b = dbm_t{dim};  // dim
        auto c = dbm_t{a};    // copy
        auto d = dbm_t{b};    // copy
        cindex_t i, j, k;
        bool ab_equal = false;
        const raw_t* ptr{nullptr};
        PROGRESS();

        // constructor post-conditions
        REQUIRE(a.isEmpty());
        REQUIRE(a.getDimension() == 1);
        EQ(a, c);
        EQ(b, d);
        REQUIRE(b.getDimension() == dim);
        REQUIRE(d.getDimension() == dim);
        REQUIRE(c.isEmpty());
        REQUIRE(c.getDimension() == 1);
        REQUIRE(b.isEmpty());
        REQUIRE(d.isEmpty());
        POINTER(a);
        POINTER(b);
        POINTER(c);
        POINTER(d);

        // check copy
        GEN(dbm);
        a = c = dbm;
        // copy -> only diagonal ok -> mark as non empty
        CHECK(!a.isEmpty());
        CHECK(a.getDimension() == 1);
        CHECK(!c.isEmpty());
        CHECK(c.getDimension() == 1);
        c.copyFrom(dbm, dim);  // copy succeeds: change dim
        a = c;
        b = dbm;
        d.copyFrom(dbm, dim);
#if 0  // checked
        if (kLoop == 0 && dim < 10)
        {
            cout << "a=\n" << a
                 << "b=\n" << b
                 << "c=\n" << c
                 << "d=\n" << d;
        }
#endif
        EQ(a, b);
        CHECK(a() == c());
        EQ(a, d);
        EQ(a, dbm, dim);
        EQ(b, c);
        EQ(b, d);
        EQ(c, d);
        EQ(b, dbm, dim);
        a.nil();
        CHECK(a.isEmpty());
        a.setDimension(dim);
        a = c;
        CHECK(a() == c());
        CHECK(a.getDimension() == c.getDimension());
        CHECK(a.getDimension() == dim);
        CHECK(a());
        CHECK(b());
        CHECK(c());
        CHECK(d());
        POINTER(a);
        POINTER(b);
        POINTER(c);
        POINTER(d);
        EQ(a, dbm, dim);
        INVALIDATE(dbm);
        a.copyTo(dbm, dim);
        CHECK(*dbm == dbm_LE_ZERO);  // always
        EQ(a, dbm, dim);
        // check copy with constraint access
        if (!a.isEmpty())  // pre-condition
            for (i = 0; i < dim; ++i)
                for (j = 0; j < dim; ++j)
                    CHECK(a(i, j) == dbm.at(i, j));
        // check !=
        GEN(dbm);
        if (a == dbm) {
            // very rare branch, don't check
            CHECK(a.relation(dbm, dim) == base_EQUAL);
            if (dim > 1)
                CHECK(dbm_areEqual(dbm, a(), dim));
            EQ(b, dbm, dim);
            EQ(c, dbm, dim);
            EQ(d, dbm, dim);
        } else {
            c1 = true;
            CHECK(a.relation(dbm, dim) != base_EQUAL);
            CHECK(dim > 1);
            CHECK(!dbm_areEqual(dbm, a(), dim));
            NEQ(a, dbm, dim);
            NEQ(b, dbm, dim);
            NEQ(c, dbm, dim);
            NEQ(d, dbm, dim);
        }

        // check relation
        a.copyTo(dbm, dim);
        CHECK(*dbm == dbm_LE_ZERO);  // always
        EQ(a, dbm, dim);
        if (rand() % 10 > 0)
            dbm_generateSuperset(dbm, dbm, dim);
        b = dbm;
        CHECK(a <= dbm.get());
        CHECK(c <= dbm.get());
        CHECK(a <= b);
        CHECK(c <= b);
        CHECK(b >= a);
        CHECK(b >= c);
        if (a != b) {
            ab_equal = false;
            c2 = true;
            CHECK(a < dbm.get());
            CHECK(c < dbm.get());
            CHECK(a < b);
            CHECK(c < b);
            CHECK(b > a);
            CHECK(b > c);
        }

        // zero
        dbm_zero(dbm, dim);
        a.setZero();
        d = a;
        a.setZero();
        CHECK(a() == d());
        d.nil();
        CHECK(a == dbm.get());
        CHECK(a.isZero());

        // init
        dbm_init(dbm, dim);
        a.setInit();
        CHECK(a == dbm.get());
        CHECK(a.isInit());

        // convex union, recall: c==d <= b
        if ((kLoop & 1) != 0) {
            a = c;
            CHECK(a <= b);
            a += b;  // a not mutable => get b
            CHECK(a == b);
            d = a;
            a += b;  // no effect
            CHECK(a() == d());
            d.nil();
        } else {
            CHECK(c <= b);
            a = c + b;  // c not mutable => +b returns b
            CHECK(a == b);
        }
        if (ab_equal)
            CHECK(a() == b());  // stronger than a == b
        // else a += b returned &a and not &b
        c += b;
        d = c;
        c += b;  // no effect
        CHECK(c() == d());
        d.nil();
        if (dim > 1)
            CHECK(c() != b());
        EQ(a, b);
        EQ(c, b);
        a.intern();
        b.intern();
        c.intern();
        CHECK(a() == b());
        CHECK(b() == c());
        GEN(dbm);
        b = dbm;
        // basic inclusion property of convex union
        c = a + b;
        CHECK(a <= c);
        CHECK(c >= b);
        if (a != c)
            CHECK(a < c);
        if (b != c)
            CHECK(c > b);
        // duplicate check to test garbage collection
        CHECK(a <= a + b);
        CHECK(a + b >= b);

        // intersection
        GEN(dbm);
        a = dbm;
        b.copyTo(dbm2, dim);
        bool i1 = dbm_haveIntersection(dbm2, dbm, dim);
        bool i2 = dbm_intersection(dbm2, dbm, dim);
        if (!i1)
            CHECK(!i2);
        if (!dbm_isEmpty(dbm2, dim))
            CHECK((dbm_relation(dbm2, dbm, dim) & base_SUBSET) != 0);
        CHECK((a & b) <= a);
        CHECK((a & b) <= b);  // ok with empty set
        CHECK((a & a) == a);
        CHECK((b & b) == b);
        CHECK((a & (a + b)) == a);
        CHECK((b & (a + b)) == b);
        k = rand() % dim;
        if (k != 0) {
            c = a(k) + 1;  // clock increment
            d = c & a;
            CHECK(d <= c);
            CHECK(d <= a);
            c4 = c4 || !d.isEmpty();
        }

        // constrain
        CHECK(a == dbm.get());
        for (k = 7; k > 0 && !dbm_generatePoint(pt.get_mutable().data(), dbm, dim); --k)
            ;
        if (k > 0) {
            bool stop = (rand() & 1) != 0;
            c5 = true;
            b = a;
            CHECK(b.contains(pt));
            cnstr.resize(dim * dim);
            for (i = 0, k = 0; i < dim; ++i) {
                for (j = 0; j < dim; ++j) {
                    if (i != j) {
                        cnstr[k] = constraint_t{i, j, dbm_bound2raw(pt[i] - pt[j], dbm_WEAK)};
                        CHECK((a && cnstr[k]) == true);  // satisfies
                        a &= cnstr[k];
                        c = a;
                        a &= cnstr[k];
                        CHECK(a() == c());
                        c.nil();
                        if (dim > 1)
                            CHECK(!a.isEmpty());
                        ++k;
                    }
                }
                if (stop && (rand() & 1) != 0)
                    break;
            }
            REQUIRE(k <= cnstr.size());
            cnstr.resize(k);
            b &= cnstr;
            CHECK(a == b);
            CHECK(dbm_constrainN(dbm, dim, cnstr.data(), k));
            CHECK(a == dbm.get());
            c = b;
            b &= cnstr;
            CHECK(b() == c());
            c.nil();
            CHECK(a.contains(pt));
            if (!stop && dim > 1 && k > 0) {
                c6 = true;
                CHECK(!a.isUnbounded());
                i = rand() % (dim - 1) + 1;
                j = rand() % dim;
                if (i == j)
                    j = 0;  // i != 0, ok
                if ((rand() & 1) != 0) {
                    k = i;
                    i = j;
                    j = k;
                }
                // constrain too much now
                constraint_t cij{i, j, dbm_bound2raw(pt[i] - pt[j], dbm_STRICT)};
                a &= cij;
                CHECK(a.isEmpty());
                CHECK(!(b && cij));
            }
        }

        // up & down
        GEN(dbm);
        c.setDimension(dim);  // because c.nil() before
        a = b = c = dbm;
        b.down();
        c.up();
        CHECK(a <= up(a));
        CHECK(a <= down(a));
        CHECK(b == down(a));
        CHECK(c == up(a));
        CHECK(a == dbm.get());
        CHECK(c.isUnbounded());
        dbm_up(dbm, dim);
        CHECK(c == dbm.get());
        a.copyTo(dbm, dim);
        dbm_down(dbm, dim);
        CHECK(b == dbm.get());

        // updates: only for dim > 1
        if (dim > 1) {
            GEN(a.getDBM_writer());
            a.copyTo(dbm, dim);
            CHECK(dbm_isValid(dbm, dim));
            i = rand() % (dim - 1) + 1;
            // updateValue
            a(i) = 0;
            b = a;
            a(i) = 0;
            CHECK(a() == b());
            dbm_updateValue(dbm, dim, i, 0);
            CHECK(a == dbm.get());
            a(i) = 1;
            CHECK(a() != b());  // mutable copy used
            b = a;
            a(i) = 1;
            CHECK(a() == b());
            dbm_updateValue(dbm, dim, i, 1);
            CHECK(a == dbm.get());
            // other updates: need 2 clocks != 0 -> dim >= 3
            if (dim >= 3) {
                int32_t inc = rand() % 100;
                b = a;
                a(i) = a(i);  // no effect
                CHECK(a() == b());
                do {
                    j = rand() % (dim - 1) + 1;
                } while (i == j);
                b.nil();
                // updateClock
                ptr = a();
                a(i) = a(j);
                CHECK(ptr == a());
                dbm_updateClock(dbm, dim, i, j);
                CHECK(a == dbm.get());
                b = a;
                a(i) = a(j) + 0;
                CHECK(a() == b());
                // updateIncrement
                ptr = a();
                a(i) += 0;
                CHECK(a() == ptr);
                CHECK(a == b);
                a(i) += inc;
                dbm_updateIncrement(dbm, dim, i, inc);
                CHECK(a == dbm.get());
                // update
                ptr = a();
                a(i) = a(i) + 0;
                CHECK(a() == ptr);
                a(i) = a(j) + inc;
                dbm_update(dbm, dim, i, j, inc);
                CHECK(a == dbm.get());
            }
        }

        // freeUp, freeDown, relaxXX
        GEN(dbm);
        a = dbm;
        b = freeAllUp(a);
        ptr = b();
        b.freeAllUp();
        CHECK(ptr == b());
        dbm_freeAllUp(dbm, dim);
        CHECK(b == dbm.get());
        b = freeAllDown(a);
        ptr = b();
        b.freeAllDown();
        CHECK(ptr == b());
        a.copyTo(dbm, dim);
        dbm_freeAllDown(dbm, dim);
        CHECK(b == dbm.get());
        k = rand() % dim;
        if (k != 0) {
            c7 = true;
            c = freeDown(a, k);
            ptr = c();
            c.freeDown(k);
            CHECK(ptr == c());
            a.copyTo(dbm, dim);
            dbm_freeDown(dbm, dim, k);
            CHECK(c == dbm.get());
            c = freeUp(a, k);
            ptr = c();
            c.freeUp(k);
            CHECK(ptr == c());
            a.copyTo(dbm, dim);
            dbm_freeUp(dbm, dim, k);
            CHECK(c == dbm.get());
        }
        d = relaxUp(a);
        ptr = d();
        d.relaxUp();
        CHECK(ptr == d());
        a.copyTo(dbm, dim);
        dbm_relaxUp(dbm, dim);
        CHECK(d == dbm.get());
        CHECK(a <= d);
        d = relaxDown(a);
        ptr = d();
        d.relaxDown();
        CHECK(ptr == d());
        a.copyTo(dbm, dim);
        dbm_relaxDown(dbm, dim);
        CHECK(d == dbm.get());
        CHECK(a <= d);
        k = rand() % dim;
        // k == 0 ok
        d = relaxUpClock(a, k);
        ptr = d();
        d.relaxUpClock(k);
        CHECK(ptr == d());
        a.copyTo(dbm, dim);
        dbm_relaxUpClock(dbm, dim, k);
        CHECK(d == dbm.get());
        CHECK(a <= d);
        d = relaxDownClock(a, k);
        ptr = d();
        d.relaxDownClock(k);
        CHECK(ptr == d());
        a.copyTo(dbm, dim);
        dbm_relaxDownClock(dbm, dim, k);
        CHECK(d == dbm.get());
        CHECK(a <= d);

        // extrapolations
        // bounds 1st
        lower[0] = upper[0] = 0;
        for (k = 1; k < dim; ++k) {
            int32_t low = RANGE() - (MAXRANGE / 2);
            int32_t up = RANGE() - (MAXRANGE / 2); /* low + RANGE()/2; */
            if (low < -2000) {
                lower[k] = upper[k] = -dbm_INFINITY;
            } else {
                lower[k] = low;
                upper[k] = up;
            }
        }
        for (k = 0; k < 4; ++k) {
            b = a;
            a.copyTo(dbm, dim);
            switch (k) {
            case 0:
                b.extrapolateMaxBounds(upper.data());
                dbm_extrapolateMaxBounds(dbm, dim, upper.data());
                break;
            case 1:
                b.diagonalExtrapolateMaxBounds(upper.data());
                dbm_diagonalExtrapolateMaxBounds(dbm, dim, upper.data());
                break;
            case 2:
                b.extrapolateLUBounds(lower.data(), upper.data());
                dbm_extrapolateLUBounds(dbm, dim, lower.data(), upper.data());
                break;
            case 3:
                b.diagonalExtrapolateLUBounds(lower.data(), upper.data());
                dbm_diagonalExtrapolateLUBounds(dbm, dim, lower.data(), upper.data());
                break;
            }
            CHECK(b == dbm.get());
            CHECK(a <= b);
        }
    }
    cnstr.clear();
    FREE(dbm2);
    FREE(dbm);
    // meant to check branches
    cout << c1 << c2 << c4 << c5 << c6 << c7 << ' ';
    PROGRESS();
}

TEST_CASE("Test DBM federation")
{
    cindex_t start;
    cindex_t end;
    int seed;
    SUBCASE("start=1 end=10 random")
    {
        start = 1;
        end = 10;
        seed = std::random_device{}();
    }
    gen.seed(seed);
    for (cindex_t i = start; i <= end; ++i) {
        (cout << '.').flush();
        for (int k = 0; k < 300; ++k) {
            test(i);
        }
    }
}
