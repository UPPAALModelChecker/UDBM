/* -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*- */

#include "dbm/ParamPricedDBM.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <vector>

using namespace PPDBM;

TEST_CASE("Matrix basic operations")
{
    Matrix<int> m{2, 3, 7};
    CHECK(m.rows() == 2);
    CHECK(m.cols() == 3);
    for (uint32_t i = 0; i < 2; ++i)
        for (uint32_t j = 0; j < 3; ++j)
            CHECK(m(i, j) == 7);

    m(1, 2) = 42;
    CHECK(m(1, 2) == 42);
    CHECK(m(0, 0) == 7); // unchanged cell

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

    // boundary case a+b == 0: incompatible only if one side is strict
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

    // x1 >= 6 is incompatible with x1 <= 4: the zone becomes empty
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
    auto p = Ppdbm{2, 0};
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
    p.getRates() = rates;

    REQUIRE(p.constrain_DBM(1, 0, DbmBound{5, false}));  // x1 <= 5
    REQUIRE(p.constrain_DBM(0, 1, DbmBound{-2, false})); // x1 >= 2

    // at the lower corner of the zone, delta == 0: cost == offset cost
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
    p.getRates() = rates;
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
    a.getRates() = ratesA;
    REQUIRE(a.constrain_DBM(1, 0, DbmBound{5, false}));
    REQUIRE(a.constrain_DBM(0, 1, DbmBound{-2, false})); // x1 in [2,5]

    Ppdbm b{2, 1};
    Matrix<int> ratesB{2, 2, 0};
    ratesB(1, 0) = 2; // strictly more expensive on the first coefficient
    ratesB(1, 1) = 1;
    b.getRates() = ratesB;
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
    c.getRates() = ratesC;
    REQUIRE(c.constrain_DBM(1, 0, DbmBound{3, false})); // x1 in [0,3]

    // overlapping but neither zone contains the other -> DIFFERENT, regardless of cost
    CHECK(a.relation(c) == base_DIFFERENT);
}

//////////////////////////////////// New functions: relax / findLinkedClocks //////////////////////////////////

TEST_CASE("DBMatrix relaxDown and relaxUp remove strictness only")
{
    DBMatrix m{3};
    m(0, 1) = DbmBound{-2, true};  // x1 > 2 (strict lower bound)
    m(0, 2) = DbmBound{0, false};  // x2 >= 0 (already weak)
    m(1, 0) = DbmBound{5, true};   // x1 < 5 (strict upper bound)
    m(2, 0) = DbmBound{3, false};  // x2 <= 3 (already weak)

    m.relaxDown();
    CHECK_FALSE(m(0, 1).getStrict());
    CHECK(m(0, 1).getValue() == -2);       // the value does not change, only the strictness is removed
    CHECK_FALSE(m(0, 2).getStrict());      // already weak: unchanged
    CHECK(m(1, 0).getStrict());            // relaxDown does not touch upper bounds

    m.relaxUp();
    CHECK_FALSE(m(1, 0).getStrict());
    CHECK(m(1, 0).getValue() == 5);
    CHECK_FALSE(m(2, 0).getStrict());

    // an infinite bound remains infinite and is never "relaxed"
    DBMatrix m2{2};
    CHECK(m2(1, 0).isInfinite());
    m2.relaxUp();
    CHECK(m2(1, 0).isInfinite());
}

TEST_CASE("DBMatrix relaxDownClock and relaxUpClock only touch the given clock's column/row")
{
    DBMatrix m{3};
    m(0, 1) = DbmBound{-2, true};  // x1 > 2
    m(2, 1) = DbmBound{4, true};   // x2 - x1 < 4
    m(1, 0) = DbmBound{5, true};   // x1 < 5
    m(1, 2) = DbmBound{1, true};   // x1 - x2 < 1

    m.relaxDownClock(1); // relax (i,1) for every i != 1
    CHECK_FALSE(m(0, 1).getStrict());
    CHECK_FALSE(m(2, 1).getStrict());
    CHECK(m(1, 0).getStrict());   // untouched: this is not an entry (*, 1)
    CHECK(m(1, 2).getStrict());   // same

    m.relaxUpClock(1); // relax (1,i) for every i != 1
    CHECK_FALSE(m(1, 0).getStrict());
    CHECK_FALSE(m(1, 2).getStrict());
}

TEST_CASE("DBMatrix findLinkedClocks detects zero cycles")
{
    DBMatrix m{3};
    // x1 - x2 = 3 exactly: x1-x2<=3 and x2-x1<=-3
    m(1, 2) = DbmBound{3, false};
    m(2, 1) = DbmBound{-3, false};

    const auto next = m.findLinkedClocks();
    REQUIRE(next.size() == 3);
    CHECK(next[1] == 2);   // 1 is linked to 2
    CHECK(next[0] == 0);   // the reference is not linked to anything here
    CHECK(next[2] == 0);   // 2 is a representative (no j>2 to check)

    // if the difference is not exactly zero, there is no link
    DBMatrix m2{3};
    m2(1, 2) = DbmBound{3, false};
    m2(2, 1) = DbmBound{-2, false}; // x1-x2 in [2,3], not fixed
    CHECK(m2.findLinkedClocks() == std::vector<uint32_t>{0, 0, 0});
}

//////////////////////////////////////////// lowerFacets / upperFacets //////////////////////////////////////////////

TEST_CASE("Ppdbm lowerFacets and upperFacets on an independent box")
{
    Ppdbm p{3, 0};
    REQUIRE(p.constrain_DBM(1, 0, DbmBound{5, false})); // x1 <= 5
    REQUIRE(p.constrain_DBM(2, 0, DbmBound{3, false})); // x2 <= 3
    // x1, x2 >= 0 by default, no link between x1 and x2

    const auto lower = p.lowerFacets();
    REQUIRE(lower.size() == 2); // one facet per clock, no redundancy
    for (const auto& f : lower) {
        CHECK(f._ppdbm(f._constraint.i, 0).getValue() == 0); // each facet fixes the clock to 0
    }

    const auto upper = p.upperFacets();
    REQUIRE(upper.size() == 2);
    bool sawX1 = false, sawX2 = false;
    for (const auto& f : upper) {
        if (f._constraint.i == 1) { CHECK(f._ppdbm(1, 0).getValue() == 5); sawX1 = true; }
        if (f._constraint.i == 2) { CHECK(f._ppdbm(2, 0).getValue() == 3); sawX2 = true; }
    }
    CHECK(sawX1);
    CHECK(sawX2);
}

TEST_CASE("Ppdbm upperFacets discards a redundant facet")
{
    // x1 <= 5, x2 <= 5, x1 - x2 <= 0  =>  x1 <= 5 is implied by x2<=5 and x1<=x2
    Ppdbm p{3, 0};
    REQUIRE(p.constrain_DBM(1, 0, DbmBound{5, false}));
    REQUIRE(p.constrain_DBM(2, 0, DbmBound{5, false}));
    REQUIRE(p.constrain_DBM(1, 2, DbmBound{0, false}));

    const auto upper = p.upperFacets();
    REQUIRE(upper.size() == 1); // only x2 <= 5 is a true facet
    CHECK(upper[0]._constraint.i == 2);
    CHECK(upper[0]._ppdbm(2, 0).getValue() == 5);
}

///////////////////////////////////// lowerFacetsRelativeTo / upperFacetsRelativeTo ///////////////////////////////////

TEST_CASE("Ppdbm lowerFacetsRelativeTo and upperFacetsRelativeTo isolate the reset clock's own bounds")
{
    Ppdbm p{3, 0};
    REQUIRE(p.constrain_DBM(1, 0, DbmBound{5, false}));  // x1 <= 5
    REQUIRE(p.constrain_DBM(0, 1, DbmBound{-2, false})); // x1 >= 2
    REQUIRE(p.constrain_DBM(2, 0, DbmBound{3, false}));  // x2 <= 3, x2 >= 0, no link between x1 and x2

    const auto lower = p.lowerFacetsRelativeTo(1);
    REQUIRE(lower.size() == 1);                 // only the reference (0) gives a non-redundant facet
    CHECK(lower[0]._constraint.i == 1);
    CHECK(lower[0]._constraint.j == 0);
    CHECK(lower[0]._ppdbm(1, 0).getValue() == 2); // x1 fixed to its lower bound

    const auto upper = p.upperFacetsRelativeTo(1);
    REQUIRE(upper.size() == 1);
    CHECK(upper[0]._constraint.i == 1);
    CHECK(upper[0]._constraint.j == 0);
    CHECK(upper[0]._ppdbm(1, 0).getValue() == 5); // x1 fixed to its upper bound
}

////////////////////////////////////////////////////// getSlope /////////////////////////////////////////////////////

TEST_CASE("Ppdbm getSlope sums the rates of every clock")
{
    Ppdbm p{3, 1};
    Matrix<int> rates{3, 2, 0};
    rates(1, 0) = 2;
    rates(1, 1) = 3;
    rates(2, 0) = 1;
    rates(2, 1) = -1;
    p.getRates() = rates;

    CHECK(p.getSlope() == CostVector{3, 2});
}

////////////////////////////////////////////////////////// delay ////////////////////////////////////////////////////

TEST_CASE("Ppdbm delay(clock, c) removes upper bounds and rebalances the rate so the slope equals c")
{
    Ppdbm p{3, 1};
    Matrix<int> rates{3, 2, 0};
    rates(1, 0) = 1; rates(1, 1) = 0;
    rates(2, 0) = 2; rates(2, 1) = 1;
    p.getRates() = rates;
    REQUIRE(p.constrain_DBM(1, 0, DbmBound{5, false}));
    REQUIRE(p.constrain_DBM(2, 0, DbmBound{3, false}));

    p.delay(1, CostVector{5, 7});

    CHECK(p(1, 0).isInfinite());
    CHECK(p(2, 0).isInfinite());
    // the rate of clock1 absorbs the difference: the total slope becomes exactly c
    CHECK(p.getSlope() == CostVector{5, 7});
}

TEST_CASE("Ppdbm delay() only removes upper bounds")
{
    Ppdbm p{2, 0};
    REQUIRE(p.constrain_DBM(1, 0, DbmBound{5, false}));

    p.delay();

    CHECK(p(1, 0).isInfinite());
    CHECK(p(0, 0).getValue() == 0);        // the reference diagonal is not touched
    CHECK_FALSE(p(0, 0).getStrict());
    CHECK(p(0, 1).getValue() == 0);        // x1 >= 0 is still true
}

//////////////////////////////////////////////////// clockReset /////////////////////////////////////////////////////

TEST_CASE("Ppdbm clockReset applied on a facet from lowerFacetsRelativeTo")
{
    Ppdbm p{3, 0};
    Matrix<int> rates{3, 1, 0};
    rates(1, 0) = 2;
    rates(2, 0) = 3;
    p.getRates() = rates;
    REQUIRE(p.constrain_DBM(1, 0, DbmBound{5, false}));  // x1 <= 5
    REQUIRE(p.constrain_DBM(0, 1, DbmBound{-2, false})); // x1 >= 2
    REQUIRE(p.constrain_DBM(2, 0, DbmBound{3, false}));  // x2 <= 3

    CHECK(p.getSlope() == CostVector{5}); // 2 + 3

    const auto facets = p.lowerFacetsRelativeTo(1);
    REQUIRE(facets.size() == 1);
    Facet f = facets[0]; // copy: x1 is already fixed to 2 on this facet
    CHECK(f._ppdbm(1, 0).getValue() == 2);

    f._ppdbm.clockReset(1, f._constraint);

    // the zone: x1 is reset to 0, and its relation to x2 is copied from the reference
    CHECK(f._ppdbm(1, 0).getValue() == 0);
    CHECK_FALSE(f._ppdbm(1, 0).getStrict());
    CHECK(f._ppdbm(0, 1).getValue() == 0);
    CHECK(f._ppdbm(1, 2).getValue() == f._ppdbm(0, 2).getValue());
    CHECK(f._ppdbm(1, 2).getStrict() == f._ppdbm(0, 2).getStrict());
    CHECK(f._ppdbm(2, 1).getValue() == f._ppdbm(2, 0).getValue());
    CHECK(f._ppdbm(2, 1).getStrict() == f._ppdbm(2, 0).getStrict());

    // the rate of x1 is transferred (here to the reference, so it is "lost" for future cost calculations)
    CHECK(f._ppdbm.getSlope() == CostVector{3}); // 5 - 2: only rates(x2) remains observable
}

/////////////////////////////////////////////////////// post_delta //////////////////////////////////////////////////

TEST_CASE("Ppdbm post_delta produces the three symbolic successors of the formula")
{
    Ppdbm p{2, 0}; // one clock, no parameter (constants)
    Matrix<int> rates{2, 1, 0};
    rates(1, 0) = 2;
    p.getRates() = rates;
    REQUIRE(p.constrain_DBM(1, 0, DbmBound{5, false})); // x1 in [0,5]

    const auto result = p.post_delta(CostVector{3}, {}); // empty J: tests the constrain_DBM_N(empty) path
    REQUIRE(result.size() == 3);

    // successor from LF(Z) (case p <= slope): x1 becomes unbounded, the slope becomes p
    const auto& fromLower = result[0];
    CHECK(fromLower(1, 0).isInfinite());
    CHECK(fromLower(0, 1).getValue() == 0);
    CHECK(fromLower.getPC().getConstraints().size() == 1);
    CHECK_FALSE(fromLower.getPC().getConstraints()[0].getStrict()); // p - slope <= 0

    // successor { (l, Z) } for the case p > slope: zone and slope unchanged
    const auto& untouched = result[1];
    CHECK(untouched(1, 0).getValue() == 5);
    CHECK(untouched.getPC().getConstraints().size() == 1);
    CHECK(untouched.getPC().getConstraints()[0].getStrict()); // slope - p < 0

    // successor from UF(Z) (case p > slope): x1 started at 5, becomes unbounded, same slope p
    const auto& fromUpper = result[2];
    CHECK(fromUpper(1, 0).isInfinite());
    CHECK(fromUpper(0, 1).getValue() == -5);
    CHECK(fromUpper.getPC().getConstraints().size() == 1);
    CHECK(fromUpper.getPC().getConstraints()[0].getStrict());
}

///////////////////////////////////////////////////////// post_e ////////////////////////////////////////////////////

TEST_CASE("Ppdbm post_e without reset applies the guard and adds the constant cost")
{
    Ppdbm p{2, 0};
    REQUIRE(p.constrain_DBM(1, 0, DbmBound{5, false})); // x1 <= 5

    const std::vector<DbmConstraint> g{{1, 0, DbmBound{3, false}}}; // x1 <= 3
    const auto result = p.post_e(g, 0, std::vector<uint32_t>{7});

    REQUIRE(result.size() == 1);
    CHECK_FALSE(result[0].isEmpty());
    CHECK(result[0](1, 0).getValue() == 3);
    CHECK(result[0].getOffsetCost() == CostVector{7});
}

TEST_CASE("Ppdbm post_e returns nothing when the guard empties the zone")
{
    Ppdbm p{2, 0};
    REQUIRE(p.constrain_DBM(1, 0, DbmBound{3, false}));  // x1 <= 3
    REQUIRE(p.constrain_DBM(0, 1, DbmBound{-1, false})); // x1 >= 1

    const std::vector<DbmConstraint> g{{0, 1, DbmBound{-10, false}}}; // x1 >= 10: incompatible
    const auto result = p.post_e(g, 0, std::vector<uint32_t>{0});

    CHECK(result.empty());
}

TEST_CASE("Ppdbm post_e with a reset generates the lower- and upper-facet branches")
{
    Ppdbm p{3, 0};
    Matrix<int> rates{3, 1, 0};
    rates(1, 0) = 2;
    rates(2, 0) = 3;
    p.getRates() = rates;
    REQUIRE(p.constrain_DBM(1, 0, DbmBound{5, false}));  // x1 <= 5
    REQUIRE(p.constrain_DBM(0, 1, DbmBound{-2, false})); // x1 >= 2
    REQUIRE(p.constrain_DBM(2, 0, DbmBound{3, false}));  // x2 <= 3

    CHECK(p.getOffsetCost() == CostVector{4}); // rates(x1)=2 * lowerbound(x1)=2

    const auto result = p.post_e({}, 1u, std::vector<uint32_t>{10}); // no guard, reset x1, edge cost 10

    REQUIRE(result.size() == 2); // one branch per facet relative to x1 (lower then upper)

    for (const auto& r : result) {
        // in both cases x1 is reset to 0, and the zone only depends on x2
        CHECK(r(1, 0).getValue() == 0);
        CHECK_FALSE(r(1, 0).getStrict());
        CHECK(r(0, 1).getValue() == 0);
        CHECK(r(1, 2).getValue() == r(0, 2).getValue());
        CHECK(r(1, 2).getStrict() == r(0, 2).getStrict());
        CHECK(r(2, 1).getValue() == r(2, 0).getValue());
        CHECK(r(2, 1).getStrict() == r(2, 0).getStrict());
        CHECK(r.getSlope() == CostVector{3}); // the rate of x1 (2) is removed from the observable slope
        CHECK(r.getPC().getConstraints().size() == 1);
    }

    // branch from lowerFacetsRelativeTo (x1 was 2 before reset): cost = 4 (already accumulated) + 10
    CHECK(result[0].getOffsetCost() == CostVector{14});
    CHECK_FALSE(result[0].getPC().getConstraints()[0].getStrict());

    // branch from upperFacetsRelativeTo (x1 was 5 before reset): cost = 10 (already accumulated) + 10
    CHECK(result[1].getOffsetCost() == CostVector{20});
    CHECK(result[1].getPC().getConstraints()[0].getStrict());
}
