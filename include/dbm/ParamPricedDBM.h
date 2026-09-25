#ifndef INCLUDE_DBM_PARAMPRICEDDBM_H
#define INCLUDE_DBM_PARAMPRICEDDBM_H

#include "dbm.h"

#include <utility>
#include <memory>
#include <limits>
#include <vector>
#include <algorithm>
#include <optional>



namespace PPDBM
{
    ///to facilitate notations
    using Vertex     = std::vector<uint32_t>; // size _dim, clock valuations
    using CostVector = std::vector<int32_t>; // size _param + 1, coeffs on parameters
    ///copied from relation.h because include problems otherwise

    ///to represent infinity
    constexpr auto INF = std::numeric_limits<int32_t>::max() >> 1u;


    /// generic matrix type (used everywhere)
    template <typename T>
    struct Matrix
    {
        Matrix(uint32_t rows, uint32_t cols, const T x): _rows{rows}, _cols{cols}, _data(rows * cols, x){}
        auto rows() const { return _rows; }
        auto cols() const { return _cols; }
        const std::vector<T>& getData() const { return _data; }
        std::vector<T>& getData() { return _data; }
        T& operator()(int i, int j) { return _data[i * _cols + j]; }
        const T& operator()(int i, int j) const { return _data[i * _cols + j]; }
        auto fill(T x){ std::fill(_data.begin(), _data.end(), x); }

    private:
        uint32_t _rows;
        uint32_t _cols;
        std::vector<T> _data;
    };




    /**
     * A parameter constraint is of the form (a_0 + sum^n_{i=1} a_i p_i) <= 0
     */
    struct ParametricConstraint
    {
        constexpr ParametricConstraint(std::vector<int> coeffs, bool strict):
            _coeffs{std::move(coeffs)},
            _strict{strict} {}
        constexpr const bool& getStrict() const { return _strict; }
        constexpr const std::vector<int>& getCoeffs() const { return _coeffs; }
    private:
        std::vector<int> _coeffs;
        bool _strict;
    };

    struct Polyhedron
    {
        constexpr std::vector<ParametricConstraint>& getConstraints(){ return _constraints; }
        constexpr const std::vector<ParametricConstraint>& getConstraints() const { return _constraints; }
        constexpr void addParametricConstraint(const ParametricConstraint& constraint)
        { _constraints.push_back(constraint); }

        bool isEmpty();
        /// removes every constraint implied by the others (does not change the represented set)
        void minimize();
        static void printPolyhedron(const std::vector<ParametricConstraint>& constraints);
    private:
        std::vector<ParametricConstraint> _constraints;
    };




    /**
     *  a constraint stored in a DBM is of the form x_i - x_j ~ value with ~ being < or <=
     */
    struct DbmBound
    {
        constexpr auto getValue() const { return _value; }
        constexpr auto getStrict() const { return _strict; }

        constexpr DbmBound( int32_t value, bool strict): _value{value}, _strict{strict}{}

        /// is this constraint unrestraining?
        constexpr bool isInfinite() const { return _value == INF; }

        /// is the new constraint useless?
        constexpr bool dominates(const DbmBound& other) const {
            return _value < other._value || (_value == other._value && (_strict || !other._strict)); }

        /// do the two constraints force the zone to be empty?
        static constexpr bool isIncompatible(const DbmBound& a, const DbmBound& b) {
            return a.getValue() + b.getValue() < 0 ||
                   (a.getValue() + b.getValue() == 0 && (a.getStrict() || b.getStrict()));
        }

    private:
        int32_t _value : 31;
        bool _strict : 1; /// constant coeff of the constraint, is the constraint strict?
    };

    constexpr auto INF_BOUND = DbmBound{INF,true};
    constexpr auto ZERO_BOUND = DbmBound{0,false};

    /// x_i - x_j <= a and x_j - x_k < b gives x_i - x_k < a + b
    constexpr DbmBound operator+(const DbmBound& a, const DbmBound& b)
    {
        if (a.isInfinite() || b.isInfinite()) return INF_BOUND;
        return DbmBound{ a.getValue() + b.getValue(), a.getStrict() || b.getStrict() };
    }


    /**
     * xi - xj <= value
     */
    struct DbmConstraint
    {
        uint32_t i;
        uint32_t j;
        DbmBound bound;
    };




    /**
     *
     */
    struct DBMatrix
    {
        DBMatrix(uint32_t dim) : _matrix{dim, dim, INF_BOUND}
        {
            for (uint32_t i = 0; i < dim; ++i) { _matrix(i, i) = ZERO_BOUND; _matrix(0,i) = ZERO_BOUND; }
        }

        constexpr auto getDim() const { return _matrix.rows(); }
        constexpr DbmBound& operator()(int i, int j) { return _matrix(i, j); }
        constexpr const DbmBound& operator()(int i, int j) const { return _matrix(i, j); }
        constexpr void reset()
        {
            _matrix.fill(INF_BOUND);
            for (uint32_t i = 0; i < _matrix.rows(); ++i) {
                _matrix(i,i)= ZERO_BOUND;
                _matrix(0,i) = ZERO_BOUND;
            }
        }
        void close(uint32_t i, uint32_t j);
        void relaxDown();
        void relaxUp();
        void relaxDownClock(uint32_t clock);
        void relaxUpClock(uint32_t clock);
        std::vector<uint32_t> findLinkedClocks() const;

        relation_t relation(const DBMatrix& other) const; ///< pure zone comparison (no cost involved).
        std::vector<Vertex> verticesDBM() const;


    private:
        Matrix<DbmBound> _matrix;
    };





    struct Facet;
    struct Edge;
    struct ParamPricedTimedAutomata;




    //////////////////////////// The PPDBM class that represents Parametric priced zones //////////////////////////////
    /**
     * Parametric priced timed difference bound matrix
     */
    struct Ppdbm
    {
        Ppdbm(uint32_t dim, uint32_t param, bool init = false):
        _dim{dim},
        _param{param},
        _dbm{dim},
        _offsetCost(param + 1),
        _rates(dim, param + 1, 0),
        _PC(Polyhedron())
        {
            if (init) {
                for (uint32_t i = 0; i < _dim; ++i) {_dbm(i,0) = ZERO_BOUND;}
                for (uint32_t i = 1; i <= _param; ++i) {
                    auto temp = std::vector<int>(_param + 1,0);
                    temp[i] = -1;
                    _PC.addParametricConstraint(ParametricConstraint(temp,false));
                }
            }
        }

        constexpr uint32_t getDim() const { return _dim; }
        constexpr uint32_t getParam() const { return _param; }
        constexpr Polyhedron& getPC() {return _PC;}
        constexpr const Polyhedron& getPC() const {return _PC;}
        constexpr Matrix<int>& getRates() { return _rates; }
        constexpr const Matrix<int>& getRates() const { return _rates; }
        constexpr std::vector<int>& getOffsetCost() { return _offsetCost; }
        constexpr const std::vector<int>& getOffsetCost() const { return _offsetCost; }
        constexpr DbmBound& operator()(int i, int j) { return _dbm(i, j); }
        constexpr const DbmBound& operator()(int i, int j) const { return _dbm(i, j); }
        void display() const;

        constexpr bool isEmpty() const { return _emptyZone; }
        void reset();
        bool constrain_DBM(uint32_t i, uint32_t j, DbmBound constraint);
        void removeCostAtOffset();
        void addCostAtOffset();
        bool isDiagonalNegative(uint32_t i) const;
        bool constrain_DBM_N(const std::vector<DbmConstraint>& constraints);
        std::vector<int> costAtOtherOffset(const DBMatrix& otherDbm) const;
        CostVector getSlope() const;
        bool hasUnboundedDelay() const;
        void addConstantCost(const std::vector<uint32_t>& q);
        static bool alreadyCoveredBy(std::pair<uint32_t, Ppdbm> symbolicState,
        const std::vector<std::pair<uint32_t, Ppdbm>>& symbolicStateListe);

        /// Delay operation on a facet put in the form Z \land (clock - otherClock = value).
        void delay(uint32_t clock, const CostVector& c);
        void delay();
        /// clock reset operation on a facet put in the form Z \land (clockToReset - otherClock = value).
        void clockReset(uint32_t clockToReset, DbmConstraint facetConstraint);
        /// lower & upper facets functions for the operators
        std::vector<Facet> lowerFacets() const;
        std::vector<Facet> upperFacets() const;
        std::vector<Facet> lowerFacetsRelativeTo(uint32_t clock) const;
        std::vector<Facet> upperFacetsRelativeTo(uint32_t clock) const;

        /// operator post, computes the successors of the ppdbm by delay.
        std::vector<Ppdbm> post_delta(CostVector p, const std::vector<DbmConstraint>& J, bool init = false) const;
        /// operator post, computes the succesors of the ppdbm by transition e.
        std::vector<Ppdbm> post_e(const std::vector<DbmConstraint>& g, uint32_t resetClock, const std::vector<uint32_t>& q) const;
        ///
        static std::vector<std::pair<uint32_t,Ppdbm>> post(const std::pair<uint32_t,Ppdbm>& symbolicState, ParamPricedTimedAutomata automaton, uint32_t target);

        /// Is there a domination relation between the two zones?
        relation_t relation(const Ppdbm& other) const;
        /// Vertices of the zone.
        constexpr std::vector<Vertex> verticesDBM() const{ return _dbm.verticesDBM(); };
        /// Evaluates the cost vector(coeffs on parameters) of *this* at vertex `v`.
        CostVector costAtVertex(const Vertex& v) const;
        /// a dominates b coefficient by coefficient?
        static bool dominatesVector(const CostVector& a, const CostVector& b);
        static bool equalVector(const CostVector& a, const CostVector& b);
        /// Compare two lists of cost vectors aligned vertex by vertex.
        static relation_t compareCostLists(const std::vector<CostVector>& c1, const std::vector<CostVector>& c2);

        /// removes every result whose (cost, region) is subsumed by another result with the SAME cost function
        static std::vector<Ppdbm> pruneRedundant(std::vector<Ppdbm> results);

    private:
        uint32_t _dim, _param = 0; // number of clocks and parameters
        DBMatrix _dbm; // the classic zone
        bool _emptyZone = false; // is the zone represented by the dbm empty?
        std::vector<int> _offsetCost; // the cost of the offset (affine function of parameters with int coeffs)
        Matrix<int> _rates; // the cost rates of clocks (affine functions of parameters with int coeffs)
        Polyhedron _PC; // parametric constraints set
    };


    struct Facet
    {
        Ppdbm _ppdbm;
        DbmConstraint _constraint;
    };

    struct OptimalRegion
    {
        static std::vector<OptimalRegion> pruneRedundantRegions(std::vector<OptimalRegion> regions);

        CostVector cost;
        std::vector<ParametricConstraint> constraints;
    };

    using PCList = std::vector<ParametricConstraint>;

    struct CostCandidate { CostVector cost; PCList region; };

    std::vector<CostCandidate> buildCandidates(const std::vector<Ppdbm>& results);

    /// splits a set of (cost, region) results into the regions where each cost is
    /// actually the minimum among all of them (the "lower envelope").
    std::vector<OptimalRegion> resolveOptimalPartition(const std::vector<CostCandidate>& candidates);




    //////////////////////////////////////////////// automata model ///////////////////////////////////////////////////

    struct Edge
    {
        Edge(uint32_t from, uint32_t to, uint32_t clockToReset, const std::vector<DbmConstraint>& guards,
            std::vector<uint32_t> costFunction):
        _from{from},
        _to{to},
        _clockToReset{clockToReset},
        _guards{guards},
        _costFunction{std::move(costFunction)} {}

        uint32_t _from;
        uint32_t _to;
        uint32_t _clockToReset;
        std::vector<DbmConstraint> _guards;
        std::vector<uint32_t> _costFunction;
    };

    struct ParamPricedTimedAutomata
    {
        ParamPricedTimedAutomata(uint32_t nbLocations, uint32_t initLocation, uint32_t nbClocks, uint32_t nbParam,
            std::vector<Edge> edges, PPDBM::Matrix<PPDBM::DbmBound> inv, PPDBM::Matrix<uint32_t> costs):
        _nbLocations{nbLocations},
        _initialLocation{initLocation},
        _nbClocks{nbClocks},
        _nbParam{nbParam},
        _edges{std::move(edges)},
        _invariants{std::move(inv)},
        _locationCosts{std::move(costs)} {}

        std::vector<int32_t> getCostFunction(uint32_t location)
        {
            std::vector<int32_t> costVector{};
            for (uint32_t j = 0; j <= _nbParam; ++j) {
                costVector.push_back(_locationCosts(location,j));
            }
            return costVector;
        };

        std::vector<PPDBM::DbmConstraint> getInvariants(uint32_t location)
        {
            std::vector<PPDBM::DbmConstraint> invariantsVector{};
            for (uint32_t j = 0; j <= _nbClocks; ++j) {
                if (_invariants(location,j).getValue() != PPDBM::INF) invariantsVector.push_back(PPDBM::DbmConstraint{j,0, _invariants(location,j)});
            }
            return invariantsVector;
        }

        uint32_t _nbLocations;
        uint32_t _initialLocation;
        uint32_t _nbClocks;
        uint32_t _nbParam;
        std::vector<Edge> _edges;
        PPDBM::Matrix<PPDBM::DbmBound> _invariants;
        PPDBM::Matrix<uint32_t> _locationCosts;
    };


} // namespace PPDBM

#endif