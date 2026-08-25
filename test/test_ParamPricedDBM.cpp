/* -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*- */

#include "dbm/ParamPricedDBM.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <vector>

using namespace PPDBM;

TEST_CASE("Matrix basic operations")
{
    Matrix<int> m{2, 3, 7};
    CHECK(m.getRows() == 2);
    CHECK(m.getCols() == 3);
    for (uint32_t i = 0; i < 2; ++i)
        for (uint32_t j = 0; j < 3; ++j)
            CHECK(m(i, j) == 7);

    m(1, 2) = 42;
    CHECK(m(1, 2) == 42);
    CHECK(m(0, 0) == 7); // untouched cell

    m.fill(3);
    for (uint32_t i = 0; i < 2; ++i)
        for (uint32_t j = 0; j < 3; ++j)
            CHECK(m(i, j) == 3);
}

TEST_CASE("DbmBound dominates and isIncompatible")
{
    const DbmBound b5{5, false};      // x - y <= 5
    const DbmBound b5strict{5, true}; // x - y < 5
    const DbmBound b3{3, false};      // x - y <= 3

    // a tighter bound dominates a looser one
    CHECK(b3.dominates(b5));
    CHECK_FALSE(b5.dominates(b3));

    // same value: strict dominates non-strict
    CHECK(b5strict.dominates(b5));
    CHECK_FALSE(b5.dominates(b5strict));

    // a bound dominates itself
    CHECK(b5.dominates(b5));

    // infinite bound is dominated by anything finite
    CHECK(INF_BOUND.isInfinite());
    CHECK_FALSE(b5.isInfinite());
    CHECK(b3.dominates(INF_BOUND));

    // x - y <= 3 and y - x <= -5  =>  x <= y - 5 and x >= y + 3 : impossible
    CHECK(DbmBound::isIncompatible(DbmBound{-5, false}, b3));
    CHECK_FALSE(DbmBound::isIncompatible(b3, b5));

    // boundary case a+b == 0 : incompatible only if one side is strict
    CHECK_FALSE(DbmBound::isIncompatible(DbmBound{3, false}, DbmBound{-3, false}));
    CHECK(DbmBound::isIncompatible(DbmBound{3, true}, DbmBound{-3, false}));
}

TEST_CASE("DbmBound addition (transitive composition)")
{
    const DbmBound a{3, false};
    const DbmBound b{2, false};

    const auto sum = a + b;
    CHECK(sum.getValue() == 5);
    CHECK_FALSE(sum.getStrict());

    const DbmBound strictB{2, true};
    const auto sumStrict = a + strictB;
    CHECK(sumStrict.getValue() == 5);
    CHECK(sumStrict.getStrict()); // strictness propagates

    const auto sumInf = a + INF_BOUND;
    CHECK(sumInf.isInfinite());
}

TEST_CASE("DBMatrix initial state")
{
    DBMatrix m{3};
    CHECK(m.getDim() == 3);

    for (uint32_t i = 0; i < 3; ++i) {
        CHECK(m(i, i).getValue() == 0);
        CHECK_FALSE(m(i, i).getStrict());
        CHECK(m(0, i).getValue() == 0); // every clock >= 0 by construction
    }
    // no upper bound has been set yet
    CHECK(m(1, 0).isInfinite());
    CHECK(m(2, 0).isInfinite());
}

TEST_CASE("DBMatrix close propagates transitive bounds")
{
    DBMatrix m{3}; // clocks: 0 (reference), 1, 2
    m(1, 0) = DbmBound{3, false}; // x1 <= 3
    m(2, 1) = DbmBound{2, false}; // x2 - x1 <= 2

    CHECK(m(2, 0).isInfinite()); // not propagated yet

    m.close(2, 1);

    CHECK(m(2, 0).getValue() == 5); // x2 <= x1 + 2 <= 5
    CHECK_FALSE(m(2, 0).getStrict());
}

TEST_CASE("DBMatrix relation between boxes")
{
    DBMatrix big{2}; // x1 in [0,5]
    big(1, 0) = DbmBound{5, false};

    DBMatrix small{2}; // x1 in [0,3]
    small(1, 0) = DbmBound{3, false};

    CHECK(big.relation(small) == base_SUPERSET);
    CHECK(small.relation(big) == base_SUBSET);
    CHECK(big.relation(big) == base_EQUAL);

    DBMatrix overlapA{2}; // x1 in [0,3]
    overlapA(1, 0) = DbmBound{3, false};

    DBMatrix overlapB{2}; // x1 in [2,5]
    overlapB(1, 0) = DbmBound{5, false};
    overlapB(0, 1) = DbmBound{-2, false};

    CHECK(overlapA.relation(overlapB) == base_DIFFERENT);
    CHECK(overlapB.relation(overlapA) == base_DIFFERENT);
}

TEST_CASE("DBMatrix verticesDBM on a trivial (single-clock) dimension")
{
    DBMatrix m{1};
    const auto vertices = m.verticesDBM();
    REQUIRE(vertices.size() == 1);
    CHECK(vertices[0] == Vertex{0});
}

TEST_CASE("DBMatrix verticesDBM computes the corners of a box")
{
    DBMatrix m{3}; // reference clock 0, x1, x2
    m(1, 0) = DbmBound{5, false}; // x1 <= 5
    m(2, 0) = DbmBound{3, false}; // x2 <= 3
    // x1 >= 0 and x2 >= 0 come from the initial state

    auto vertices = m.verticesDBM();

    std::vector<Vertex> expected{{0, 0, 0}, {0, 0, 3}, {0, 5, 0}, {0, 5, 3}};
    std::sort(expected.begin(), expected.end());

    REQUIRE(vertices.size() == expected.size());
    CHECK(vertices == expected);
}

TEST_CASE("Ppdbm constrain_DBM tightens bounds and detects emptiness")
{
    Ppdbm p{2, 0}; // 1 clock, no parameters

    CHECK(p.constrain_DBM(1, 0, DbmBound{5, false})); // x1 <= 5
    CHECK_FALSE(p.isEmpty());

    // a weaker constraint is useless: nothing changes
    CHECK(p.constrain_DBM(1, 0, DbmBound{10, false}));
    CHECK(p(1, 0).getValue() == 5);

    // a tighter constraint is applied
    CHECK(p.constrain_DBM(1, 0, DbmBound{4, false}));
    CHECK(p(1, 0).getValue() == 4);

    // x1 >= 6 is incompatible with x1 <= 4 : the zone becomes empty
    CHECK_FALSE(p.constrain_DBM(0, 1, DbmBound{-6, false}));
    CHECK(p.isEmpty());
}

TEST_CASE("Ppdbm constrain_DBM_N applies a batch of constraints atomically")
{
    Ppdbm p{3, 0};

    const std::vector<DbmConstraint> batch{
        {1, 0, DbmBound{4, false}},  // x1 <= 4
        {2, 0, DbmBound{6, false}},  // x2 <= 6
        {0, 1, DbmBound{-1, false}}, // x1 >= 1
    };

    CHECK(p.constrain_DBM_N(batch));
    CHECK_FALSE(p.isEmpty());
    CHECK(p(1, 0).getValue() == 4);
    CHECK(p(2, 0).getValue() == 6);
    CHECK(p(0, 1).getValue() == -1);

    // x1 >= 10 is incompatible with the x1 <= 4 constraint set above
    const std::vector<DbmConstraint> contradiction{
        {0, 1, DbmBound{-10, false}},
    };
    CHECK_FALSE(p.constrain_DBM_N(contradiction));
    CHECK(p.isEmpty());
}

TEST_CASE("Ppdbm reset returns the zone to its unconstrained state")
{
    Ppdbm p{2, 0};
    p.constrain_DBM(1, 0, DbmBound{5, false});
    p.constrain_DBM(0, 1, DbmBound{-100, false}); // makes the zone empty
    REQUIRE(p.isEmpty());

    p.reset();

    CHECK_FALSE(p.isEmpty());
    CHECK(p(1, 0).isInfinite());
    CHECK(p(0, 1).getValue() == 0);
}

TEST_CASE("Ppdbm costAtVertex reflects rates and the zone's lower bound")
{
    Ppdbm p{2, 1}; // 1 clock, 1 parameter

    Matrix<int> rates{2, 2, 0};
    rates(1, 0) = 1;
    rates(1, 1) = 1;
    p.setRates(rates);

    REQUIRE(p.constrain_DBM(1, 0, DbmBound{5, false}));  // x1 <= 5
    REQUIRE(p.constrain_DBM(0, 1, DbmBound{-2, false})); // x1 >= 2

    // at the lower corner of the zone, delta == 0 : cost == offset cost
    CHECK(p.costAtVertex(Vertex{0, 2}) == CostVector{2, 2});

    // at the upper corner, cost == offset + rate * (5 - 2)
    CHECK(p.costAtVertex(Vertex{0, 5}) == CostVector{5, 5});
}

TEST_CASE("Ppdbm costAtOtherOffset translates the cost to another zone's offset")
{
    Ppdbm p{2, 1};
    Matrix<int> rates{2, 2, 0};
    rates(1, 0) = 1;
    rates(1, 1) = 1;
    p.setRates(rates);
    REQUIRE(p.constrain_DBM(1, 0, DbmBound{5, false}));
    REQUIRE(p.constrain_DBM(0, 1, DbmBound{-2, false})); // x1 in [2,5]

    DBMatrix otherDbm{2}; // x1 in [0,5] : offset is 0 instead of 2
    otherDbm(1, 0) = DbmBound{5, false};

    const auto cost = p.costAtOtherOffset(otherDbm);
    CHECK(cost == CostVector{0, 0});
}

TEST_CASE("Ppdbm::dominatesVector and equalVector")
{
    const CostVector a{1, 2, 3};
    const CostVector b{1, 2, 4};
    const CostVector c{2, 1, 4};

    CHECK(Ppdbm::dominatesVector(a, a));
    CHECK(Ppdbm::dominatesVector(a, b));      // a <= b on every coefficient
    CHECK_FALSE(Ppdbm::dominatesVector(b, a));
    CHECK_FALSE(Ppdbm::dominatesVector(a, c)); // incomparable vectors
    CHECK_FALSE(Ppdbm::dominatesVector(c, a));

    CHECK(Ppdbm::equalVector(a, a));
    CHECK_FALSE(Ppdbm::equalVector(a, b));
}

TEST_CASE("Ppdbm::compareCostLists aggregates per-vertex domination")
{
    const std::vector<CostVector> cheaper{{1, 1}, {2, 2}};
    const std::vector<CostVector> pricier{{2, 2}, {3, 3}};
    const std::vector<CostVector> mixed{{0, 5}, {5, 0}};

    CHECK(Ppdbm::compareCostLists(cheaper, cheaper) == base_EQUAL);
    CHECK(Ppdbm::compareCostLists(cheaper, pricier) == base_SUPERSET); // cheaper dominates
    CHECK(Ppdbm::compareCostLists(pricier, cheaper) == base_SUBSET);
    CHECK(Ppdbm::compareCostLists(cheaper, mixed) == base_DIFFERENT);
}

TEST_CASE("Ppdbm::relation compares zones and, when equal, their costs")
{
    Ppdbm a{2, 1};
    Matrix<int> ratesA{2, 2, 0};
    ratesA(1, 0) = 1;
    ratesA(1, 1) = 1;
    a.setRates(ratesA);
    REQUIRE(a.constrain_DBM(1, 0, DbmBound{5, false}));
    REQUIRE(a.constrain_DBM(0, 1, DbmBound{-2, false})); // x1 in [2,5]

    Ppdbm b{2, 1};
    Matrix<int> ratesB{2, 2, 0};
    ratesB(1, 0) = 2; // strictly more expensive on the first coefficient
    ratesB(1, 1) = 1;
    b.setRates(ratesB);
    REQUIRE(b.constrain_DBM(1, 0, DbmBound{5, false}));
    REQUIRE(b.constrain_DBM(0, 1, DbmBound{-2, false})); // same zone as a

    // same zone, a is cheaper everywhere -> a dominates (SUPERSET), b is dominated (SUBSET)
    CHECK(a.relation(b) == base_SUPERSET);
    CHECK(b.relation(a) == base_SUBSET);
    CHECK(a.relation(a) == base_EQUAL);

    Ppdbm c{2, 1};
    Matrix<int> ratesC{2, 2, 0};
    ratesC(1, 0) = 1;
    ratesC(1, 1) = 1;
    c.setRates(ratesC);
    REQUIRE(c.constrain_DBM(1, 0, DbmBound{3, false})); // x1 in [0,3]

    // overlapping but neither zone contains the other -> DIFFERENT, regardless of cost
    CHECK(a.relation(c) == base_DIFFERENT);
}