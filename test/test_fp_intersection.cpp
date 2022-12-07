#include "dbm/dbm.h"
#include "dbm/print.h"
#include "base/doubles.h"

#include <iostream>
#include <limits>
#include <cmath>
#include <cstdint>

#include <doctest/doctest.h>

void printConstraint(const raw_t raw, const double i, const double j)
{
    std::cout << i << (dbm_rawIsStrict(raw) ? "<" : "<=") << j << "+" << dbm_raw2bound(raw) << std::endl;
}

void printViolatingConstraint(dbm::reader dbm, const double* pt)
{
    const auto dim = dbm.get_dim();
    for (cindex_t i = 0; i < dim; ++i) {
        for (cindex_t j = 0; j < dim; ++j) {
            if (dbm.at(i, j) < dbm_LS_INFINITY) {
                double bound = dbm.bound(i, j);
                /* if strict: !(pi-pj < bij) -> false
                 * if weak  : !(pi-pj <= bij) -> false
                 */
                if (dbm.is_strict(i, j)) {
                    // if (pt[i] >= pt[j]+bound) return false;
                    if (IS_GE(pt[i], pt[j] + bound))
                        printConstraint(dbm.at(i, j), pt[i], pt[j]);
                } else {
                    // if (pt[i] > pt[j]+bound) return false;
                    if (IS_GT(pt[i], pt[j] + bound))
                        printConstraint(dbm.at(i, j), pt[i], pt[j]);
                }
            }
        }
    }
}

TEST_CASE("Floating point intersection")
{
    const size_t dim = 3;
    const raw_t dbm1_raw[] = {
        dbm_boundbool2raw(0, false),   dbm_boundbool2raw(-8, true),  dbm_boundbool2raw(-182, true),
        dbm_boundbool2raw(86, true),   dbm_boundbool2raw(0, false),  dbm_boundbool2raw(-174, true),
        dbm_boundbool2raw(445, false), dbm_boundbool2raw(400, true), dbm_boundbool2raw(0, false)};
    auto dbm1 = dbm::reader{dbm1_raw, dim};
    const raw_t dbm2_raw[] = {
        dbm_boundbool2raw(0, false),   dbm_boundbool2raw(-8, true),   dbm_boundbool2raw(-182, true),
        dbm_boundbool2raw(10, false),  dbm_boundbool2raw(0, false),   dbm_boundbool2raw(-174, false),
        dbm_boundbool2raw(184, false), dbm_boundbool2raw(174, false), dbm_boundbool2raw(0, false)};
    auto dbm2 = dbm::reader{dbm2_raw, dim};

    double x = 8.9844974150810408;
    double y = 182.98449741508105;
    const double pt[] = {0, x, y};

    std::cout.precision(std::numeric_limits<double>::max_digits10);
    std::cout << "DBMs exhibit some unexpected behaviour when it comes to doubles and intersections" << std::endl;
    std::cout << "Consider the following DBMS:" << std::endl;
    std::cout << "DBM 1:" << std::endl;
    dbm_cppPrint(std::cout, dbm1);
    std::cout << std::endl << "DBM 2:" << std::endl;
    dbm_cppPrint(std::cout, dbm2);
    std::cout << std::endl
              << "DBM 1 corresponds to the set of clock valuations {(0, x, y) | 8 < x < 86, 182 < y <= 445, x + 174 < "
                 "y < x + 437}"
              << std::endl;
    std::cout
        << std::endl
        << "DBM 2 corresponds to the set of clock valuations {(0, x, y) | 8 < x <= 10, 182 < y < 184, x - y == -174}"
        << std::endl;
    std::cout << "Since the constraints x - y == -174 and x + 174 < y, the two DBMs should have no intersection."
              << std::endl;
    std::cout << "But consider the valuation [0, " << pt[1] << ", " << pt[2] << "]" << std::endl;
    std::cout << pt[1] << " - " << pt[2] << " = " << pt[1] - pt[2] << std::endl;
    CHECK(pt[1] - pt[2] == -174);
    std::cout << pt[1] << " < " << pt[2] << " - 174 = " << ((pt[1] < pt[2] - 174) ? "true" : "false") << std::endl;
    CHECK(pt[1] < pt[2] - 174);
    std::cout
        << "This finicky behaviour in doubles basically allows us to have a series of points which exist in both DBMs:"
        << std::endl;
    CHECK(dbm_isRealPointIncluded(pt, dbm1, dim));
    CHECK(dbm_isRealPointIncluded(pt, dbm2, dim));
    CHECK(!dbm_haveIntersection(dbm1, dbm2, dim));
}
