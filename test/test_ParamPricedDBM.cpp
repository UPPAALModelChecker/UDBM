#include "dbm/ParamPricedDBM.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <vector>

using namespace PPDBM;

TEST_CASE("DbmBound dominates")
{
    // valeur plus petite => contrainte plus forte => domine
    const auto a = DbmBound{3, false}; // <= 3
    const auto b = DbmBound{5, false}; // <= 5
    CHECK(a.dominates(b));
    CHECK(!b.dominates(a));

    // meme valeur : strict domine non-strict (< 3 est plus fort que <= 3)
    const auto strict3 = DbmBound{3, true};
    const auto loose3 = DbmBound{3, false};
    CHECK(strict3.dominates(loose3));
    CHECK(!loose3.dominates(strict3));

    // une borne se domine toujours elle-meme
    CHECK(a.dominates(a));

    // INF ne domine jamais une borne finie, l'inverse est toujours vrai
    CHECK(!INF_BOUND.dominates(a));
    CHECK(a.dominates(INF_BOUND));
}

TEST_CASE("DbmBound isIncompatible")
{
    // x_i - x_j <= 3 et x_j - x_i <= -4 => x_i - x_j >= 4, incompatible avec <=3
    const auto a = DbmBound{3, false};
    const auto b = DbmBound{-4, false};
    CHECK(DbmBound::isIncompatible(a, b));

    // x_i - x_j <= 3 et x_j - x_i <= -3 => point unique {x_i - x_j = 3}, compatible
    const auto c = DbmBound{-3, false};
    CHECK(!DbmBound::isIncompatible(a, c));

    // si l'une des deux bornes est stricte, la somme nulle devient incompatible
    const auto aStrict = DbmBound{3, true};
    CHECK(DbmBound::isIncompatible(aStrict, c));

    // deux bornes larges, somme > 0 => compatible
    const auto d = DbmBound{1, false};
    const auto e = DbmBound{1, false};
    CHECK(!DbmBound::isIncompatible(d, e));
}

TEST_CASE("DbmBound operator+")
{
    const auto a = DbmBound{3, false};
    const auto b = DbmBound{2, false};
    const auto sum = a + b;
    CHECK(sum.getValue() == 5);
    CHECK(!sum.getStrict());

    // si l'une des deux est stricte, le resultat est strict
    const auto bStrict = DbmBound{2, true};
    const auto sum2 = a + bStrict;
    CHECK(sum2.getValue() == 5);
    CHECK(sum2.getStrict());

    // INF + n'importe quoi = INF
    const auto sum3 = a + INF_BOUND;
    CHECK(sum3.isInfinite());
}

TEST_CASE("DBMatrix construction and reset")
{
    auto m = DBMatrix{3};
    REQUIRE(m.getDim() == 3);

    SUBCASE("etat initial : diagonale a 0, reste a INF")
    {
        for (uint32_t i = 0; i < 3; ++i) {
            CHECK(m(i, i).getValue() == 0);
            CHECK(!m(i, i).getStrict());
            for (uint32_t j = 0; j < 3; ++j) {
                if (i != j) CHECK(m(i, j).isInfinite());
            }
        }
    }

    SUBCASE("reset revient a l'etat initial")
    {
        m(0, 1) = DbmBound{7, false};
        m.reset();
        CHECK(m(0, 1).isInfinite());
        CHECK(m(0, 0).getValue() == 0);
        CHECK(m(1, 1).getValue() == 0);
    }
}

TEST_CASE("DBMatrix close")
{
    SUBCASE("propagation simple : x1-x0<=3, x2-x1<=2 => x2-x0<=5")
    {
        auto m = DBMatrix{3};
        m(1, 0) = DbmBound{3, false};
        m.close(1, 0);
        m(2, 1) = DbmBound{2, false};
        m.close(2, 1);

        CHECK(m(2, 0).getValue() == 5);
        CHECK(!m(2, 0).getStrict());
    }

    SUBCASE("propagation d'une borne stricte")
    {
        // x1-x0<3 (stricte), x2-x1<=2 (large) => x2-x0<5 (stricte)
        auto m = DBMatrix{3};
        m(1, 0) = DbmBound{3, true};
        m.close(1, 0);
        m(2, 1) = DbmBound{2, false};
        m.close(2, 1);

        CHECK(m(2, 0).getValue() == 5);
        CHECK(m(2, 0).getStrict());
    }
}

TEST_CASE("DBMatrix relation")
{
    // m1: x1<=5, m2: x1<=3 => m2 subset de m1
    auto m1 = DBMatrix{2};
    m1(1, 0) = DbmBound{5, false};
    m1.close(1, 0);

    auto m2 = DBMatrix{2};
    m2(1, 0) = DbmBound{3, false};
    m2.close(1, 0);

    CHECK(m1.relation(m2) == base_SUPERSET);
    CHECK(m2.relation(m1) == base_SUBSET);

    auto m3 = DBMatrix{2};
    m3(1, 0) = DbmBound{5, false};
    m3.close(1, 0);
    CHECK(m1.relation(m3) == base_EQUAL);
}

TEST_CASE("DBMatrix verticesDBM")
{
    // 2 horloges : x0 = reference (toujours 0), x1 dans [2,5]
    auto m = DBMatrix{2};
    m(1, 0) = DbmBound{5, false};  // x1 - x0 <= 5
    m(0, 1) = DbmBound{-2, false}; // x0 - x1 <= -2, donc x1 >= 2

    const auto vertices = m.verticesDBM();
    const auto v25 = Vertex{0, 5};
    const auto v22 = Vertex{0, 2};

    REQUIRE(vertices.size() == 2);
    CHECK(std::find(vertices.begin(), vertices.end(), v25) != vertices.end());
    CHECK(std::find(vertices.begin(), vertices.end(), v22) != vertices.end());
}

TEST_CASE("Ppdbm reset")
{
    auto p = Ppdbm{2, 1};
    p.constrain_DBM(1, 0, DbmBound{4, false});
    p.reset();
    CHECK(!p.isEmpty());
    CHECK(p(1, 0).isInfinite());
}

TEST_CASE("Ppdbm constrain_DBM")
{
    SUBCASE("une contrainte plus faible que l'existante est ignoree")
    {
        auto p = Ppdbm{2, 0};
        CHECK(p.constrain_DBM(1, 0, DbmBound{5, false}));
        CHECK(p.constrain_DBM(1, 0, DbmBound{9, false}));
        CHECK(p(1, 0).getValue() == 5);
    }

    SUBCASE("une contrainte plus forte resserre la zone")
    {
        auto p = Ppdbm{2, 0};
        CHECK(p.constrain_DBM(1, 0, DbmBound{5, false}));
        CHECK(p.constrain_DBM(1, 0, DbmBound{3, false}));
        CHECK(p(1, 0).getValue() == 3);
        CHECK(!p.isEmpty());
    }

    SUBCASE("une contrainte incompatible vide la zone")
    {
        // x1-x0<=3 puis x0-x1<=-4 (x1>=4) => incompatible avec x1<=3
        auto p = Ppdbm{2, 0};
        CHECK(p.constrain_DBM(1, 0, DbmBound{3, false}));
        CHECK(!p.constrain_DBM(0, 1, DbmBound{-4, false}));
        CHECK(p.isEmpty());
    }
}

TEST_CASE("Ppdbm constrain_DBM_N")
{
    SUBCASE("plusieurs contraintes compatibles appliquees d'un coup")
    {
        // x1 dans [2,5], x2 dans [1,4]
        auto p = Ppdbm{3, 0};
        const auto constraints = std::vector<DbmConstraint>{
            {1, 0, DbmBound{5, false}},
            {0, 1, DbmBound{-2, false}},
            {2, 0, DbmBound{4, false}},
            {0, 2, DbmBound{-1, false}},
        };
        CHECK(p.constrain_DBM_N(constraints));
        CHECK(!p.isEmpty());
        CHECK(p(1, 0).getValue() == 5);
        CHECK(p(0, 1).getValue() == -2);
        CHECK(p(2, 0).getValue() == 4);
        CHECK(p(0, 2).getValue() == -1);
    }

    SUBCASE("une contrainte incompatible dans la liste vide la zone")
    {
        auto p = Ppdbm{2, 0};
        const auto constraints = std::vector<DbmConstraint>{
            {1, 0, DbmBound{3, false}},
            {0, 1, DbmBound{-4, false}},
        };
        CHECK(!p.constrain_DBM_N(constraints));
        CHECK(p.isEmpty());
    }
}

TEST_CASE("Ppdbm isDiagonalNegative")
{
    auto p = Ppdbm{2, 0};
    CHECK(!p.isDiagonalNegative(0));
    CHECK(!p.isDiagonalNegative(1));

    p.constrain_DBM(1, 0, DbmBound{3, false});
    p.constrain_DBM(0, 1, DbmBound{-4, false});
    CHECK(p.isDiagonalNegative(1));
}

TEST_CASE("Ppdbm costAtVertex")
{
    // 1 parametre. rate(1,0)=2 (constante), rate(1,1)=1 (coeff parametre)
    auto p = Ppdbm{2, 1};
    auto rates = Matrix<int>{2, 2, 0};
    rates(1, 0) = 2;
    rates(1, 1) = 1;
    p.setRates(rates);
    p.constrain_DBM(1, 0, DbmBound{5, false});
    p.constrain_DBM(0, 1, DbmBound{-2, false}); // x1 dans [2,5]

    const auto v = Vertex{0, 5}; // delta = 5 - 2 = 3
    const auto cost = p.costAtVertex(v);
    REQUIRE(cost.size() == 2);
    CHECK(cost[0] == 6); // 0 + 2*3
    CHECK(cost[1] == 3); // 0 + 1*3
}

TEST_CASE("Ppdbm dominatesVector and equalVector")
{
    const auto a = CostVector{1, 2, 3};
    const auto b = CostVector{2, 2, 4};
    const auto c = CostVector{1, 2, 3};
    const auto d = CostVector{1, 2, 4};

    CHECK(Ppdbm::dominatesVector(a, b));
    CHECK(!Ppdbm::dominatesVector(b, a));
    CHECK(Ppdbm::dominatesVector(a, c)); // egalite => domine (<=)

    CHECK(Ppdbm::equalVector(a, c));
    CHECK(!Ppdbm::equalVector(a, d));
}

TEST_CASE("Ppdbm compareCostLists")
{
    const auto c1 = std::vector<CostVector>{{1, 1}, {2, 2}};
    const auto c2 = std::vector<CostVector>{{1, 1}, {2, 2}};
    CHECK(Ppdbm::compareCostLists(c1, c2) == base_EQUAL);

    const auto cheaper = std::vector<CostVector>{{1, 1}, {1, 1}};
    const auto costly = std::vector<CostVector>{{2, 2}, {3, 3}};
    // cheaper <= costly partout => cheaper "domine" => base_SUPERSET
    CHECK(Ppdbm::compareCostLists(cheaper, costly) == base_SUPERSET);
    CHECK(Ppdbm::compareCostLists(costly, cheaper) == base_SUBSET);

    const auto mixed1 = std::vector<CostVector>{{1, 5}, {5, 1}};
    const auto mixed2 = std::vector<CostVector>{{5, 1}, {1, 5}};
    CHECK(Ppdbm::compareCostLists(mixed1, mixed2) == base_DIFFERENT);
}

TEST_CASE("Ppdbm relation")
{
    SUBCASE("meme zone, couts differents => domination par le cout")
    {
        auto p1 = Ppdbm{2, 1};
        auto p2 = Ppdbm{2, 1};
        p1.constrain_DBM(1, 0, DbmBound{5, false});
        p1.constrain_DBM(0, 1, DbmBound{-2, false});
        p2.constrain_DBM(1, 0, DbmBound{5, false});
        p2.constrain_DBM(0, 1, DbmBound{-2, false});

        auto rates1 = Matrix<int>{2, 2, 0};
        rates1(1, 1) = 1; // cout = x1
        p1.setRates(rates1);

        auto rates2 = Matrix<int>{2, 2, 0};
        rates2(1, 1) = 2; // cout = 2*x1, toujours plus cher (ou egal)
        p2.setRates(rates2);

        // meme zone, p1 moins cher partout => p1 domine (SUPERSET), p2 est domine (SUBSET)
        CHECK(p1.relation(p2) == base_SUPERSET);
        CHECK(p2.relation(p1) == base_SUBSET);
    }

    SUBCASE("zones differentes, le resultat reste stable")
    {
        auto p1 = Ppdbm{2, 0};
        auto p2 = Ppdbm{2, 0};
        p1.constrain_DBM(1, 0, DbmBound{5, false});
        p2.constrain_DBM(1, 0, DbmBound{3, false});
        p2.constrain_DBM(0, 1, DbmBound{-1, false});

        const auto r1 = p1.relation(p2);
        const auto r2 = p2.relation(p1);
        CHECK((r1 == base_SUPERSET || r1 == base_DIFFERENT));
        CHECK((r2 == base_SUBSET || r2 == base_DIFFERENT));
    }
}