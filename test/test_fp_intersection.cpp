#ifdef NDEBUG
#undef NDEBUG
#endif

#include "dbm/dbm.h"
#include "dbm/print.h"
#include "base/doubles.h"

#include <iostream>
#include <limits>
#include <cmath>
#include <cstdint>

void printConstraint(const raw_t constraint, const double i, const double j)
{
    std::cout << i << (dbm_rawIsStrict(constraint) ? "<" : "<=") << j << "+" << dbm_raw2bound(constraint) << std::endl;
}

static inline raw_t get_bound(const raw_t* dbm, cindex_t dim, cindex_t i, cindex_t j) { return dbm[i * dim + j]; }
void printViolatingConstraint(const raw_t* dbm, const double* pt, const uint32_t dim)
{
    cindex_t i, j;
    for (i = 0; i < dim; ++i) {
        for (j = 0; j < dim; ++j) {
            if (get_bound(dbm, dim, i, j) < dbm_LS_INFINITY) {
                double bound = dbm_raw2bound(get_bound(dbm, dim, i, j));
                /* if strict: !(pi-pj < bij) -> false
                 * if weak  : !(pi-pj <= bij) -> false
                 */
                if (dbm_rawIsStrict(get_bound(dbm, dim, i, j))) {
                    // if (pt[i] >= pt[j]+bound) return false;
                    if (IS_GE(pt[i], pt[j] + bound))
                        printConstraint(get_bound(dbm, dim, i, j), pt[i], pt[j]);
                } else {
                    // if (pt[i] > pt[j]+bound) return false;
                    if (IS_GT(pt[i], pt[j] + bound))
                        printConstraint(get_bound(dbm, dim, i, j), pt[i], pt[j]);
                }
            }
        }
    }
}

int main()
{
    const size_t dim = 3;
    const raw_t dbm1[] = {dbm_boundbool2raw(0, false),   dbm_boundbool2raw(-8, true),  dbm_boundbool2raw(-182, true),
                          dbm_boundbool2raw(86, true),   dbm_boundbool2raw(0, false),  dbm_boundbool2raw(-174, true),
                          dbm_boundbool2raw(445, false), dbm_boundbool2raw(400, true), dbm_boundbool2raw(0, false)};
    const raw_t dbm2[] = {dbm_boundbool2raw(0, false),   dbm_boundbool2raw(-8, true),   dbm_boundbool2raw(-182, true),
                          dbm_boundbool2raw(10, false),  dbm_boundbool2raw(0, false),   dbm_boundbool2raw(-174, false),
                          dbm_boundbool2raw(184, false), dbm_boundbool2raw(174, false), dbm_boundbool2raw(0, false)};

    double x = 8.9844974150810408;
    double y = 182.98449741508105;
    const double pt[] = {0, x, y};

    std::cout.precision(std::numeric_limits<double>::max_digits10);
    std::cout << "DBMs exhibit some unexpected behaviour when it comes to doubles and intersections" << std::endl;
    std::cout << "Consider the following DBMS:" << std::endl;
    std::cout << "DBM 1:" << std::endl;
    dbm_cppPrint(std::cout, dbm1, dim);
    std::cout << std::endl << "DBM 2:" << std::endl;
    dbm_cppPrint(std::cout, dbm2, dim);
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
    std::cout << pt[1] << " < " << pt[2] << " - 174 = " << ((pt[1] < pt[2] - 174) ? "true" : "false") << std::endl;
    std::cout
        << "This finicky behaviour in doubles basically allows us to have a series of points which exist in both DBMs:"
        << std::endl;
    std::cout << "assert(dbm_isRealPointIncluded(pt, dbm1, dim));" << std::endl;
    assert(dbm_isRealPointIncluded(pt, dbm1, dim));
    std::cout << "assert(dbm_isRealPointIncluded(pt, dbm2, dim));" << std::endl;
    assert(dbm_isRealPointIncluded(pt, dbm2, dim));
    std::cout << "assert(!dbm_haveIntersection(dbm1, dbm2, dim));" << std::endl;
    assert(!dbm_haveIntersection(dbm1, dbm2, dim));
}
