#ifndef INCLUDE_DBM_PARAMPRICEDDBM_H
#define INCLUDE_DBM_PARAMPRICEDDBM_H

#include "dbm.h"
#include "mingraph.h"
#include "constraints.h"

#include <iosfwd>
#include <utility>
#include <memory>
#include <cstdio>
#include <vector>




constexpr int INF = INT_MAX >> 1;


/**
 * A parameter constraint is of the form (a_0 + sum^n_{i=1} a_i p_i) <= 0
 */
class ParameterConstraint { public: std::vector<int> coeffs; };

class Polyhedron { public: std::vector<ParameterConstraint> constraints; };

/**
 *  a constraint stored in a DBM is of the form x_i - x_j ~ value with ~ being < or <=
 */
class DbmBound
{
public:
    int value; // constant coeff of the constraint
    bool strict; // is the constraint strict?

    constexpr DbmBound(int value, bool strict): value(value), strict(strict){}
    bool isInfinite() const { return value == INF; } // is this constraint unrestraining?
    bool isLooserThan(const DbmBound& other) const {
        return value > other.value || (value == other.value && strict && !other.strict);
    }
    DbmBound negate() const { assert(!isInfinite()); return DbmBound(-value, !strict); }
};
constexpr DbmBound INF_BOUND(INF,true);
constexpr DbmBound DIAG_BOUND(0,false);

class DBMatrix
{
public:
    DBMatrix(int dim): dim(dim), data(dim * dim, INF_BOUND)
    {
        for (int i = 0; i < dim; ++i) {
            data[i * dim + i] = DIAG_BOUND;
        };
    };

    DbmBound& operator()(int i, int j){ return data[i * dim + j]; };
    const DbmBound& operator()(int i, int j) const { return data[i * dim + j]; };

    int dim; // number of clocks + 1 (for the ref clock)
    std::vector<DbmBound> data;
};


/**
 * Parametric priced timed difference bound matrix
 */
class Ppdbm
{
    int dim, param = 0; // number of clocks and parameters
    DBMatrix DBM; // the classic zone
    bool emptyZone = false; // is the zone represented by the dbm empty?

    std::vector<int> offsetCost; // the cost of the offset (affine function of parameters with int coeffs)
    std::vector<std::vector<int>> rates; // the cost rates of clocks (affine functions of parameters with int coeffs)
    Polyhedron PC; // parametric constraints set

public:
    Ppdbm(int dim, int param);
    void reset();
    bool constrain_DBM(int i, int j, DbmBound constraint);
};


#endif
