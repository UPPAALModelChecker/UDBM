#ifndef INCLUDE_DBM_PARAMPRICEDDBM_H
#define INCLUDE_DBM_PARAMPRICEDDBM_H

#include "dbm.h"

#include <utility>
#include <memory>
#include <limits>
#include <vector>
#include <algorithm>



namespace PPDBM
{
    constexpr auto INF = std::numeric_limits<int>::max() >> 1u;

    template <typename T>
    struct Matrix
    {
        constexpr Matrix(uint32_t rows, uint32_t cols, const T x): _rows{rows}, _cols{cols}, _data(rows * cols, x){}
        constexpr auto getRows() const { return _rows; }
        constexpr auto getCols() const { return _cols; }
        constexpr auto& getData() const { return _data; }
        constexpr auto& getData() { return _data; }
        constexpr T& operator()(int i, int j) { return _data[i * _cols + j]; }
        constexpr const T& operator()(int i, int j) const { return _data[i * _cols + j]; }
        constexpr auto fill(T x){ std::fill(_data.begin(), _data.end(), x); }

    private:
        uint32_t _rows;
        uint32_t _cols;
        std::vector<T> _data;
    };




    /**
     * A parameter constraint is of the form (a_0 + sum^n_{i=1} a_i p_i) <= 0
     */
    struct ParameterConstraint { std::vector<int> coeffs; };

    struct Polyhedron { std::vector<ParameterConstraint> constraints; };




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
    constexpr auto DIAG_BOUND = DbmBound{0,false};

    /// x_i - x_j <= a and x_j - x_k < b gives x_i - x_k < a + b
    constexpr DbmBound operator+(const DbmBound& a, const DbmBound& b)
    {
        if (a.isInfinite() || b.isInfinite()) return INF_BOUND;
        return DbmBound{ a.getValue() + b.getValue(), a.getStrict() || b.getStrict() };
    }


    /**
     *
     */
    struct DbmConstraint
    {
        uint32_t i;
        uint32_t j;
        DbmBound bound;
    };



    using Vertex = std::vector<int32_t>;

    /**
     *
     */
    struct DBMatrix
    {
        constexpr DBMatrix(uint32_t dim) : _matrix{dim, dim, INF_BOUND}
        {
            for (uint32_t i = 0; i < dim; ++i) { _matrix(i, i) = DIAG_BOUND; _matrix(0,i) = DIAG_BOUND; }
        }

        constexpr auto getDim() const { return _matrix.getRows(); }
        constexpr DbmBound& operator()(int i, int j) { return _matrix(i, j); }
        constexpr const DbmBound& operator()(int i, int j) const { return _matrix(i, j); }
        constexpr void reset()
        {
            _matrix.fill(INF_BOUND);
            for (uint32_t i = 0; i < _matrix.getRows(); ++i) {
                _matrix(i,i)= DIAG_BOUND;
                _matrix(0,i) = DIAG_BOUND;
            }
        }
        void close(uint32_t i, uint32_t j);

        relation_t relation(const DBMatrix& other) const; ///< pure zone comparison (no cost involved).
        std::vector<Vertex> verticesDBM() const;


    private:
        Matrix<DbmBound> _matrix;
    };



    using Vertex     = std::vector<int32_t>; // size _dim, clock valuations
    using CostVector = std::vector<uint32_t>; // size _param + 1, coeffs on parameters

    /**
     * Parametric priced timed difference bound matrix
     */
    struct Ppdbm
    {
        constexpr Ppdbm(uint32_t dim, uint32_t param): _dim{dim}, _param{param}, _dbm{dim},
        _offsetCost(param + 1),
        _rates(dim, param + 1, 0) {}

        constexpr uint32_t getDim() const { return _dim; }
        constexpr DbmBound& operator()(int i, int j) { return _dbm(i, j); }
        constexpr const DbmBound& operator()(int i, int j) const { return _dbm(i, j); }
        void setRates(const Matrix<int>& rates) { _rates = rates; }

        constexpr bool isEmpty() const { return _emptyZone; }
        void reset();
        bool constrain_DBM(uint32_t i, uint32_t j, DbmBound constraint);
        void removeCostAtOffset();
        void addCostAtOffset();
        bool isDiagonalNegative(uint32_t i) const;
        bool constrain_DBM_N(const std::vector<DbmConstraint>& constraints);
        std::vector<uint32_t> costAtOtherOffset(const DBMatrix& otherDbm) const;

        /// Is there a domination relation between the two zones?
        relation_t relation(const Ppdbm& other) const;
        /// Vertices of the zone.
        constexpr std::vector<Vertex> verticesDBM(const DBMatrix& dbm) const{ return _dbm.verticesDBM(); };
        /// Evaluates the cost vector(coeffs on parameters) of *this* at vertex `v`.
        CostVector costAtVertex(const Vertex& v) const;
        /// a dominates b coefficient by coefficient?
        static bool dominatesVector(const CostVector& a, const CostVector& b);
        static bool equalVector(const CostVector& a, const CostVector& b);
        /// Compare two lists of cost vectors aligned vertex by vertex.
        static relation_t compareCostLists(const std::vector<CostVector>& c1, const std::vector<CostVector>& c2);

    private:
        uint32_t _dim, _param = 0; // number of clocks and parameters
        DBMatrix _dbm; // the classic zone
        bool _emptyZone = false; // is the zone represented by the dbm empty?
        std::vector<uint32_t> _offsetCost; // the cost of the offset (affine function of parameters with int coeffs)
        Matrix<int> _rates; // the cost rates of clocks (affine functions of parameters with int coeffs)
        Polyhedron _PC; // parametric constraints set
    };
} // namespace PPDBM

#endif