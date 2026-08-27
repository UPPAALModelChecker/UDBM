/* -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*- */

#include "dbm/ParamPricedDBM.h"

#include <debug/macros.h>

#include <algorithm>
#include <functional>
#include <cassert>
#include <memory>
#include <tuple>
#include <optional>

namespace PPDBM
{
    /**
     *
     * @param i
     * @param j
     */
    void DBMatrix::close(uint32_t i, uint32_t j)
    {
        const auto dim = getDim();
        const auto dij = _matrix(i, j);

        for (uint32_t a = 0; a < dim; ++a) {
            for (uint32_t b = 0; b < dim; ++b) {
                auto candidate = _matrix(a, i) + dij + _matrix(j, b);
                if (candidate.dominates(_matrix(a, b))) {
                    _matrix(a, b) = candidate;
                }
            }
        }
    }


    /**
     * resets a DBM to its unrestrained state.
     */
    void Ppdbm::reset()
    {
        _dbm.reset();
        std::fill(_offsetCost.begin(), _offsetCost.end(), 0);
        _rates.fill(0);
        _PC.getConstraints().clear();
        _emptyZone = false;
    }

    /**
     *
     */
    void Ppdbm::removeCostAtOffset()
    {
        for (uint32_t h = 1; h < _dim; h++) {
            for (uint32_t k = 0; k <= _param; k++) {
                _offsetCost[k] -= _rates(h,k) * (-_dbm(0, h).getValue());
            }
        }
    }

    /**
     *
     */
    void Ppdbm::addCostAtOffset()
    {
        for (uint32_t h = 1; h < _dim; h++) {
            for (uint32_t k = 0; k <= _param; k++) {
                _offsetCost[k] += _rates(h,k) * (-_dbm(0, h).getValue());
            }
        }
    }

    /**
     *
     * @param i
     * @return
     */
    bool Ppdbm::isDiagonalNegative(uint32_t i) const
    {
        const auto& d = _dbm(i, i);
        return d.getValue() < 0 || (d.getValue() == 0 && d.getStrict());
    }

    /**
     *
     * @param i
     * @param j
     * @param constraint
     * @return
     */
    bool Ppdbm::constrain_DBM(uint32_t i, uint32_t j, DbmBound constraint)
    {
        assert(i < _dim && j < _dim);

        // The constraint is useless?
        if (_dbm(i,j).dominates(constraint)) { return true; }

        // The constraint empties the zone?
        if (DbmBound::isIncompatible(constraint, _dbm(j,i))) {
            _dbm(i, j) = constraint;
            _emptyZone = true;
            return false;
        }

        // remove the cost at the previous offset
        removeCostAtOffset();

        // Add the constraint and close the DBM
        _dbm(i, j) = constraint;
        _dbm.close(i, j);
        if (isDiagonalNegative(i)) { _emptyZone = true; return false; }

        // Add the cost at the new offset
        addCostAtOffset();

        return true;
    }

    /**
     *
     * @param constraints
     * @return
     */
    bool Ppdbm::constrain_DBM_N(const std::vector<DbmConstraint>& constraints)
    {
        if (constraints.empty()) return true; // nothing to do --> done

        // remove the cost at the previous offset
        removeCostAtOffset();

        bool empty = false;
        for (auto it = constraints.begin(); it != constraints.end() && !empty; ++it) {
            const auto& [i, j, bound] = *it;

            if (_dbm(i,j).dominates(bound)) { continue; } // useless constraint

            if (DbmBound::isIncompatible(bound, _dbm(j,i))) {
                _dbm(i, j) = bound;
                empty = true;
                break;
            } // constraint empties the zone

            _dbm(i, j) = bound;
            _dbm.close(i, j);
            empty = isDiagonalNegative(i);
        }

        // add the cost of the new offset
        addCostAtOffset();

        if (empty) { _emptyZone = true; return false; }
        return true;
    }

    /**
     * cost function of another DBM's offset calculated with actual rates.
     * @param otherDbm
     * @return
     */
    std::vector<int> Ppdbm::costAtOtherOffset(const DBMatrix& otherDbm) const
    {
        std::vector<int> cost = _offsetCost;

        for (uint32_t x = 1; x < _dim; ++x) {
            // difference in x coordinate of the offset
            const int32_t offsetDifference = -otherDbm(0, x).getValue() - (-_dbm(0, x).getValue());

            // multiply the difference by the associated rate and add it to the cost
            for (uint32_t k = 0; k <= _param; ++k) {
                cost[k] += _rates(x, k) * offsetDifference;
            }
        }

        return cost;
    }

    /**
     *
     * @param q
     */
    void Ppdbm::addConstantCost(const CostVector& q)
    {
        for (uint32_t k = 0; k <= _param; ++k) _offsetCost[k] += q[k];
    }




    /////////////////////////////////////////////// Post operators ////////////////////////////////////////////////////
    /**
     * removes the strictness of lower bounds
     */
    void DBMatrix::relaxDown()
    {
        for (uint32_t i = 1; i < getDim(); ++i) {
            auto& b = _matrix(0, i);
            if (!b.isInfinite() && b.getStrict()) b = DbmBound{b.getValue(), false};
        }
    }

    /**
     * removes the strictness of upper bounds
     */
    void DBMatrix::relaxUp()
    {
        for (uint32_t i = 1; i < getDim(); ++i) {
            auto& b = _matrix(i, 0);
            if (!b.isInfinite() && b.getStrict()) b = DbmBound{b.getValue(), false};
        }
    }

    /**
     *
     * @param clock
     */
    void DBMatrix::relaxDownClock(uint32_t clock)
    {
        // relax (i, clock) for each i : "x_i - x_clock <= b"
        for (uint32_t i = 0; i < getDim(); ++i) {
            if (i == clock) continue;
            auto& b = _matrix(i, clock);
            if (!b.isInfinite() && b.getStrict()) b = DbmBound{b.getValue(), false};
        }
    }

    /**
     *
     * @param clock
     */
    void DBMatrix::relaxUpClock(uint32_t clock)
    {
        // relax (clock, i) for each i : "x_clock - x_i <= b"
        for (uint32_t i = 0; i < getDim(); ++i) {
            if (i == clock) continue;
            auto& b = _matrix(clock, i);
            if (!b.isInfinite() && b.getStrict()) b = DbmBound{b.getValue(), false};
        }
    }

    /**
     * finds the clocks that are linked together. ( x_i = x_j + constant)
     * @return
     */
    std::vector<uint32_t> DBMatrix::findLinkedClocks() const
    {
        const auto dim = getDim();
        std::vector<uint32_t> next(dim, 0);
        for (uint32_t i = 0; i < dim; ++i) {
            for (uint32_t j = i + 1; j < dim; ++j) {
                if (!(*this)(i, j).isInfinite() && !(*this)(j, i).isInfinite() &&
                    (*this)(i, j).getValue() + (*this)(j, i).getValue() == 0) {
                    // if DBM(i,j) + DBM(j,i) = 0 then clocks i and j are linked together by a constant.
                    next[i] = j;
                    break;
                }
            }
        }
        return next;
    }

    /**
     * if DBM(i,j) >= DBM(i,k) + DBM(k,j) then the facet is already covered by another one --> we dont consider it
     * @param dbm
     * @param i
     * @param j
     * @param next
     * @return
     */
    static bool isRedundant(const DBMatrix& dbm, uint32_t i, uint32_t j, const std::vector<uint32_t>& next)
    {
        if (i == j) return true;
        const auto bij = dbm(i, j);
        if (bij.isInfinite()) return true;

        for (uint32_t k = 0; k < dbm.getDim(); ++k) {
            if (k == i || k == j || next[k] != 0) continue;
            const auto bik = dbm(i, k);
            const auto bkj = dbm(k, j);
            if (!bik.isInfinite() && !bkj.isInfinite() &&
                bij.getValue() >= bik.getValue() + bkj.getValue()) {
                return true;
                }
        }
        return false;
    }

    /**
     * returns the list of lower facets (defined by Z \land (x_0 - x_i <= -L))
     * @return
     */
    std::vector<Facet> Ppdbm::lowerFacets() const
    {
        std::vector<Facet> result;

        Ppdbm relaxed = *this;
        relaxed._dbm.relaxDown();               // closure
        const auto next = relaxed._dbm.findLinkedClocks();

        for (uint32_t i = 1; i < _dim; ++i) {
            if (next[i] != 0) continue;
            if (isRedundant(relaxed._dbm, 0, i, next)) continue; // Facet covered by others

            Ppdbm facetZone = relaxed;
            const auto lower = relaxed(0, i);          // x0 - xi <= -L_i already set
            const DbmBound pin{-lower.getValue(), false};
            const bool ok = facetZone.constrain_DBM(i, 0, pin);    // xi - x0 <= L_i  as well, so now xi = L_i
            assert(ok); // should never empty the zone

            result.push_back(Facet{std::move(facetZone), DbmConstraint{i, 0, pin}});
        }
        return result;
    }

    /**
     * returns the list of upper facets (defined by Z \land (x_i - x_0 <= U))
     * @return
     */
    std::vector<Facet> Ppdbm::upperFacets() const
    {
        std::vector<Facet> result;

        Ppdbm relaxed = *this;
        relaxed._dbm.relaxUp();
        const auto next = relaxed._dbm.findLinkedClocks();

        for (uint32_t i = 1; i < _dim; ++i) {
            if (next[i] != 0) continue;
            if (isRedundant(relaxed._dbm, i, 0, next)) continue;

            Ppdbm facetZone = relaxed;
            const auto upper = relaxed(i, 0);              // xi - x0 <= U_i  already set
            const DbmBound pin{-upper.getValue(), false};
            const bool ok = facetZone.constrain_DBM(0, i, pin);    // x0 - xi <= -U_i as well, so now xi = U_i
            assert(ok);

            result.push_back(Facet{std::move(facetZone), DbmConstraint{i, 0, upper}});
        }
        return result;
    }

    /**
     *
     * @param clock
     * @return
     */
    std::vector<Facet> Ppdbm::lowerFacetsRelativeTo(uint32_t clock) const
    {
        std::vector<Facet> result;

        Ppdbm relaxed = *this;
        relaxed._dbm.relaxDownClock(clock);
        const auto next = relaxed._dbm.findLinkedClocks();

        for (uint32_t i = 0; i < _dim; ++i) {
            if (next[i] != 0) continue;
            if (isRedundant(relaxed._dbm, i, clock, next)) continue;

            Ppdbm facetZone = relaxed;
            const auto bound = relaxed(i, clock);                   // x_i - x_clock <= bound
            const DbmBound pin{-bound.getValue(), false};       // x_clock - x_i <= -bound
            const bool ok = facetZone.constrain_DBM(clock, i, pin);
            assert(ok);

            result.push_back(Facet{std::move(facetZone), DbmConstraint{clock, i, pin}});
        }
        return result;
    }

    /**
     *
     * @param clock
     * @return
     */
    std::vector<Facet> Ppdbm::upperFacetsRelativeTo(uint32_t clock) const
    {
        std::vector<Facet> result;

        Ppdbm relaxed = *this;
        relaxed._dbm.relaxUpClock(clock);
        const auto next = relaxed._dbm.findLinkedClocks();

        for (uint32_t i = 0; i < _dim; ++i) {
            if (next[i] != 0) continue;
            if (isRedundant(relaxed._dbm, clock, i, next)) continue;

            Ppdbm facetZone = relaxed;
            const auto bound = relaxed(clock, i);                  // x_clock - x_i <= bound
            const DbmBound pin{-bound.getValue(), false};        // x_i - x_clock <= -bound
            const bool ok = facetZone.constrain_DBM(i, clock, pin);
            assert(ok);

            result.push_back(Facet{std::move(facetZone), DbmConstraint{clock, i, bound}});
        }
        return result;
    }

    /**
     * returns sum_{x \in X} r(x)
     * @return
     */
    CostVector Ppdbm::getSlope() const
    {
        CostVector slope(_param + 1,0);
        for (uint32_t i = 1; i < _dim; ++i) {
            for (uint32_t j = 0; j < _param + 1; ++j) {
                slope[j] += _rates(i,j);
            }
        }
        return slope;
    }

    /**
     * suppress the clock constraints of the form x_i <= w. Delay operation on a facet.
     * @param clock
     * @param c
     */
    void Ppdbm::delay(uint32_t clock, CostVector c)
    {
        // modify the zone
        for (uint32_t i = 1; i < getDim(); ++i)
            _dbm(i, 0) = INF_BOUND;

        // adapt the price
        for (uint32_t j = 0; j < _param + 1; ++j) {
            // for every parameter, add the coeff of c and substract the coeffs of every other rate
            _rates(clock, j) = c[j];
            for (uint32_t k = 1; k < _dim; ++k)
                if (k != clock)
                    _rates(clock,j) -= _rates(k,j);
        }
    }

    /**
     * suppress the clock constraints of the form x_i <= w. Delay operation on a facet.
     */
    void Ppdbm::delay()
    {
        // modify the zone
        for (uint32_t i = 1; i < getDim(); ++i)
            _dbm(i, 0) = INF_BOUND;
    }

    /**
     * clock reset operation on a facet.
     * assumes that clockToReset is the clock to reset and facetConstraint is the active constraint defining the facet
     * on which we apply the reset, put in the form (clockToReset - anotherClock = value).
     * @param clockToReset
     * @param facetConstraint
     */
    void Ppdbm::clockReset(uint32_t clockToReset, DbmConstraint facetConstraint)
    {
        assert(clockToReset < _dim);

        // modify the zone
        _dbm(clockToReset, 0) = DIAG_BOUND;
        _dbm(0, clockToReset) = DIAG_BOUND;
        for (uint32_t i = 1; i < getDim(); ++i) {
            _dbm(clockToReset, i) = _dbm(0,i);
            _dbm(i,clockToReset) = _dbm(i,0);
        }

        // adapt the price
        for (uint32_t k = 0; k < _param + 1; ++k) {
            _rates(facetConstraint.j,k) += _rates(facetConstraint.i,k);
            _rates(facetConstraint.i,k) = 0;
        }
    }

    /**
     * returns every successor by delay
     * @param p
     * @param J
     * @return
     */
    std::vector<Ppdbm> Ppdbm::post_delta(CostVector p, const std::vector<DbmConstraint>& J)
    {
        std::vector<Ppdbm> result;

        {
            // case 1 : { (l, F↑p ∧ J) | F ∈ LF(Z) } and the constraint is p <= ∑r(x) thus p - ∑r(x) <= 0
            CostVector temp = getSlope();
            for (uint32_t i = 0; i < _param + 1; ++i) {
                temp[i] = p[i] - temp[i];
            }
            ParametricConstraint pconstraint{temp,false};

            std::vector<Facet> LF = lowerFacets();
            for (Facet f : LF ) {
                auto [ppdbm, constraint] = f;
                ppdbm.delay(constraint.i, p);
                ppdbm.constrain_DBM_N(J);
                ppdbm.getPC().addParametricConstraint(pconstraint);
                result.push_back(std::move(ppdbm));
            }
        }
        {
            // case 2 : { (l, Z) } ∪ { (l, F↑p ∧ J) | F ∈ UF(Z) } and the constraint is p > ∑r(x) thus ∑r(x) - p < 0
            CostVector temp = getSlope();
            for (uint32_t i = 0; i < _param + 1; ++i) {
                temp[i] -= p[i];
            }
            ParametricConstraint pconstraint{temp,true};

            Ppdbm copy = *this;
            copy.getPC().addParametricConstraint(pconstraint);
            result.push_back(std::move(copy));

            std::vector<Facet> UF = upperFacets();
            for (Facet f : UF ) {
                auto [ppdbm, constraint] = f;
                ppdbm.delay(constraint.i, p);
                ppdbm.constrain_DBM_N(J);
                ppdbm.getPC().addParametricConstraint(pconstraint);
                result.push_back(std::move(ppdbm));
            }
        }
        return result;
    }

    /**
     * returns every successor by transition e = (l, g, {x}, l'), with edge cost q.
     * resetClock = std::nullopt si e ne reset aucune horloge.
     */
    std::vector<Ppdbm> Ppdbm::post_e(const std::vector<DbmConstraint>& g, std::optional<uint32_t> resetClock, const CostVector& q) const
    {
        std::vector<Ppdbm> result;

        // Z ∧ g
        Ppdbm guarded = *this;
        if (!g.empty() && !guarded.constrain_DBM_N(g)) return result; // g empties the zone
        if (guarded.isEmpty()) return result;

        if (!resetClock) {
            // if no reset : { (l', Z ∧ g + q) }
            guarded.addConstantCost(q);
            result.push_back(std::move(guarded));
            return result;
        }

        const uint32_t x = *resetClock;
        CostVector rx(_param + 1);
        for (uint32_t k = 0; k <= _param; ++k) rx[k] = guarded._rates(x,k);

        {
            // cas r(x) >= 0  <=>  -r(x) <= 0
            CostVector temp(_param + 1);
            for (uint32_t k = 0; k <= _param; ++k) temp[k] = -rx[k];
            ParametricConstraint pconstraint{temp, false};

            for (Facet f : guarded.lowerFacetsRelativeTo(x)) {
                auto [ppdbm, constraint] = f;
                ppdbm.clockReset(x, constraint);
                ppdbm.addConstantCost(q);
                ppdbm.getPC().addParametricConstraint(pconstraint);
                result.push_back(std::move(ppdbm));
            }
        }
        {
            // cas r(x) < 0
            ParametricConstraint pconstraint{rx, true};

            for (Facet f : guarded.upperFacetsRelativeTo(x)) {
                auto [ppdbm, constraint] = f;
                ppdbm.clockReset(x, constraint);
                ppdbm.addConstantCost(q);
                ppdbm.getPC().addParametricConstraint(pconstraint);
                result.push_back(std::move(ppdbm));
            }
        }

        return result;
    }



    ////////////////////////////////////////////// WQO relation ////////////////////////////////////////////////////////


    /**
     * compares each constraints, and if every constraint of one dominates the other, then subset.
     * @param other
     * @return
     */
    relation_t DBMatrix::relation(const DBMatrix& other) const
    {
        assert(getDim() == other.getDim());
        const auto dim = getDim();

        bool thisLeqOther = true; // this <= other everywhere -> this subset of other
        bool otherLeqThis = true; // other <= this everywhere -> other subset of this

        for (uint32_t i = 0; i < dim; ++i) {
            for (uint32_t j = 0; j < dim; ++j) {
                if (!(*this)(i, j).dominates(other(i, j))) { thisLeqOther = false; }
                if (!other(i, j).dominates((*this)(i, j))) { otherLeqThis = false; }
            }
        }

        if (thisLeqOther && otherLeqThis) { return base_EQUAL; }
        if (thisLeqOther) { return base_SUBSET; }
        if (otherLeqThis) { return base_SUPERSET; }
        return base_DIFFERENT;
    }

    /**
     * computes the coefficients of the parameters in the cost function a the vertex v
     * @param v
     * @return
     */
    CostVector Ppdbm::costAtVertex(const Vertex& v) const
    {
        assert(v.size() == _dim);

        CostVector cost = _offsetCost; // copy: offset coeffs
        for (uint32_t x = 1; x < _dim; ++x) {
            const int32_t offsetX = -_dbm(0, x).getValue(); // min value of clock h
            const int32_t delta = v[x] - offsetX;
            for (uint32_t k = 0; k <= _param; ++k) {
                cost[k] += _rates(x, k) * delta;
            }
        }
        return cost;
    }

    /**
     * is every coefficient of a lesser or equal than every coefficient ob b?
     * @param a
     * @param b
     * @return
     */
    bool Ppdbm::dominatesVector(const CostVector& a, const CostVector& b)
    {
        assert(a.size() == b.size());
        for (uint32_t k = 0; k < a.size(); ++k) {
            if (a[k] > b[k]) { return false; }
        }
        return true;
    }

    /**
     * vector equality test
     * @param a
     * @param b
     * @return
     */
    bool Ppdbm::equalVector(const CostVector& a, const CostVector& b)
    {
        return a == b;
    }

    /**
     * Compares both versions of every cost function of a summit. If One DBM has lower coefficients everywhere,
     * then better cost functions and domination.
     * @param c1
     * @param c2
     * @return
     */
    relation_t Ppdbm::compareCostLists(const std::vector<CostVector>& c1, const std::vector<CostVector>& c2)
    {
        assert(c1.size() == c2.size());

        bool thisLeqOther = true; // cost of *this* <= cost of other, on every summit
        bool otherLeqThis = true; // cost of other <= cost of *this*, on every summit

        for (uint32_t v = 0; v < c1.size(); ++v) {
            if (!dominatesVector(c1[v], c2[v])) { thisLeqOther = false; }
            if (!dominatesVector(c2[v], c1[v])) { otherLeqThis = false; }
            if (!thisLeqOther && !otherLeqThis) { break; } // shortcut
        }

        if (thisLeqOther && otherLeqThis) { return base_EQUAL; }
        if (thisLeqOther) { return base_SUPERSET; } // *this* cheaper everywhere -> dominates
        if (otherLeqThis) { return base_SUBSET; }
        return base_DIFFERENT; // no domination --> no branch cutting
    }

    /**
     * most important function, decides when the loops stop / when we cut branches of the exploration graph.
     * is one or the other DBM not interesting to consider? meaning is there a zone who is a subset of the other and
     * with less interesting cost functions ? (a zone already covered basically?)
     * @param other
     * @return
     */
    relation_t Ppdbm::relation(const Ppdbm& other) const
    {
        assert(_dim == other._dim && _param == other._param);

        switch (_dbm.relation(other._dbm)) {
        case base_DIFFERENT:
            return base_DIFFERENT;

        case base_EQUAL: {
            auto vertices = _dbm.verticesDBM(); // same zone on both sides
            std::vector<CostVector> c1, c2;
            c1.reserve(vertices.size());
            c2.reserve(vertices.size());
            for (const auto& v : vertices) {
                c1.push_back(costAtVertex(v));
                c2.push_back(other.costAtVertex(v));
            }
            return compareCostLists(c1, c2);
        }

        case base_SUPERSET: {
            // *this* is the bigger zone, other is the smaller
            auto vertices = other._dbm.verticesDBM();
            std::vector<CostVector> c1, c2;
            c1.reserve(vertices.size());
            c2.reserve(vertices.size());
            for (const auto& v : vertices) {
                c1.push_back(costAtVertex(v));
                c2.push_back(other.costAtVertex(v));
            }
            auto costRel = compareCostLists(c1, c2);
            return (costRel == base_SUPERSET) ? base_SUPERSET : base_DIFFERENT;
        }

        case base_SUBSET: {
            // other is the bigger zone, *this* is the smaller:
            auto vertices = _dbm.verticesDBM();
            std::vector<CostVector> c1, c2;
            c1.reserve(vertices.size());
            c2.reserve(vertices.size());
            for (const auto& v : vertices) {
                c1.push_back(costAtVertex(v));
                c2.push_back(other.costAtVertex(v));
            }
            auto costRel = compareCostLists(c1, c2);
            return (costRel == base_SUBSET) ? base_SUBSET : base_DIFFERENT;
        }

        default:
            return base_DIFFERENT;
        }
    }







    //////////////////////////////////////////////// sommetsDBM() with claude's help /////////////////////////////////////


    /**
     * finds the vertices of a dbm! very important function!
     * @return
     */
    std::vector<Vertex> DBMatrix::verticesDBM() const
    {
        const uint32_t dim = getDim();
        std::vector<Vertex> vertices;
        if (dim <= 1) {
            vertices.push_back(Vertex{0}); return vertices;
        }

        // Matrix of bound values
        Matrix<int> boundValues{dim,dim,0};
        for (uint32_t i = 0; i < dim; ++i)
            for (uint32_t j = 0; j < dim; ++j)
                if (i != j) boundValues(i,j) = _matrix(i,j).getValue();

        // candidate edges {i,j} (i<j) : at least a finite direction
        struct Edge { uint32_t i, j; };
        std::vector<Edge> candidates;
        for (uint32_t i = 0; i < dim; ++i)
            for (uint32_t j = i + 1; j < dim; ++j)
                if (boundValues(i,j) != INF || boundValues(j,i) != INF)
                    candidates.push_back({i, j});

        // Union-find to build the spanning trees
        std::vector<uint32_t> parents(dim);
        for (uint32_t i = 0; i < dim; ++i) {parents[i] = i;};

        std::vector<Edge> chosen;

        //
        auto find = [&](uint32_t x) {
            while (parents[x] != x)
                x = parents[x];
            return x;
        };

        // evaluates a spanning tree : tries every possible direction
        // on every edge, propagates from 0, checks global feasibility.
        auto evaluateTree = [&](const std::vector<Edge>& tree) {
            const uint32_t nbedge = tree.size();

            // we register the options of active constraint for each couple (x_i, x_j) with i<j
            Matrix<std::tuple<uint32_t,uint32_t,int>> options(nbedge,2, {0, 0, INF});
            for (uint32_t k = 0; k < nbedge; ++k) {
                auto [i, j] = tree[k];
                if (boundValues(i,j) != INF) options(k,0) = {i, j, boundValues(i,j)}; // x_i - x_j = w
                if (boundValues(j,i) != INF) options(k,1) = {j, i, boundValues(j,i)}; // x_j - x_i = w
            }

            // initial combination: first valid option for everyone
            std::vector<uint32_t> combination(nbedge);
            for (uint32_t k = 0; k < nbedge; ++k) {
                // option 0 is valid
                if (std::get<2>(options(k, 0)) != INF) { combination[k] = 0; }
                // otherwise use option 1
                else { combination[k] = 1; }
            }

            // function to calculate the next combination of valid options for the edges
            auto nextCombination = [&]() {
                uint32_t k = 0;
                while (k < nbedge) {
                    ++combination[k];
                    // We reached the end of the options for this edge?
                    if (combination[k] >= 2) { combination[k] = 0; ++k; continue; }
                    // Skip this option if it does not exist.
                    auto [i, j, w] = options(k, combination[k]);
                    if (w != INF)  return true;
                }
                return false;
            };

            // we try every possible combination of active constraints for couples (x_i, x_j)
            for (;;) {
                // directed adjacency matrix (to go from x to y, costs -3, and from y to x, costs +3...)
                Matrix<int> directedAdj(dim, dim, INF);

                // fill the adjacency matrix
                for (uint32_t k = 0; k < nbedge; ++k) {
                    // example: x_0 - x_1 = 3.
                    auto [from, to, w] = options(k,combination[k]);
                    directedAdj(from,to) = -w; // x_1 = x_0 - 3. remove 3 to go from x0 to x1.
                    directedAdj(to, from) = w;  // x_0 = x_1 + 3. add 3 to go from x1 to x0.
                }

                // propagate the clock values through the spanning tree
                std::vector<int> val(dim, INF);
                val[0] = 0; // the ref clock is always 0
                std::vector<uint32_t> stack{0};

                while (!stack.empty()) {
                    uint32_t u = stack.back();
                    stack.pop_back();

                    for (uint32_t v = 0; v < dim; ++v) {
                        const int delta = directedAdj(u, v);
                        // if there is an active constraint linking u and v and v was not already visited
                        if (delta != INF && val[v] == INF) {
                            // visit v and add it to the stack
                            val[v] = val[u] + delta;
                            stack.push_back(v);
                        }
                    }
                }

                // is every constraint verified with this configuration?
                bool ok = true;
                for (uint32_t i = 0; ok && i < dim; ++i)
                    for (uint32_t j = 0; ok && j < dim; ++j)
                        if (i != j && boundValues(i, j) != INF && val[i] - val[j] > boundValues(i, j))
                            ok = false;

                // is the spanning tree covering every clock?
                for (uint32_t i = 0; i < dim; ++i) {
                    if (val[i] == INF) {
                        ok = false;
                        break;
                    }
                }

                // if everything fits, we found a vertex.
                if (ok) {
                    Vertex v(dim);
                    for (uint32_t k = 0; k < dim; ++k) v[k] = val[k];
                    vertices.push_back(std::move(v));
                }

                // then we check next combination
                if (!nextCombination())
                    break;
            }
        };


        // Backtracking : find all spanning trees on 'candidates'
        std::function<void(size_t)> backtrack = [&](size_t start) {
            if (chosen.size() == static_cast<size_t>(dim - 1)) { evaluateTree(chosen); return; }
            if (candidates.size() - start < (dim - 1 - chosen.size())) return;

            for (size_t k = start; k < candidates.size(); ++k) {
                auto [i, j] = candidates[k];
                cindex_t ri = find(i), rj = find(j);
                if (ri == rj) continue; // cycle
                parents[ri] = rj;
                chosen.push_back({i, j});
                backtrack(k + 1);
                chosen.pop_back();
                parents[ri] = ri;
            }
        };
        backtrack(0);

        std::sort(vertices.begin(), vertices.end());
        vertices.erase(std::unique(vertices.begin(), vertices.end()), vertices.end());
        return vertices;
    }
} // namespace PPBDM



