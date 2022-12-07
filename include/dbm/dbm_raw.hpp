#ifndef INCLUDE_DBM_RAW_HPP
#define INCLUDE_DBM_RAW_HPP

#include "dbm/constraints.h"

#include <cstddef>  // ptrdiff_t

namespace dbm
{
    /** Non-owning raw DBM wrapper for read-only/const access to raw bounds. Drop-in replacement for "const raw_t*". */
    class reader
    {
        const raw_t* dbm;
        cindex_t dim;

    public:
        reader(const raw_t* dbm, cindex_t dim): dbm{dbm}, dim{dim} {}
        const raw_t* get() const { return dbm; }  ///< TODO: needed only for doctest-2.4.8/NIX, can be removed later
        /** Returns the number of dimensions/clocks in this DBM. */
        cindex_t get_dim() const { return dim; }
        /** Returns the raw bound at {i,j} of DBM which includes the strictness bit. */
        raw_t at(cindex_t i, cindex_t j) const { return dbm[i * dim + j]; }
        /** Returns the integer bound at {i,j} of DBM with strictness bit removed. */
        int32_t bound(cindex_t i, cindex_t j) const { return dbm_raw2bound(at(i, j)); }
        /** Returns true if the bound at {i,j} is strict ("<"). */
        bool is_strict(cindex_t i, cindex_t j) const { return dbm_rawIsStrict(at(i, j)); }
        /** Returns true if the bound at {i,j} is weak ("<="). */
        bool is_weak(cindex_t i, cindex_t j) const { return dbm_rawIsWeak(at(i, j)); }
        /** Trivial comparison. */
        bool operator==(const reader& other) const { return dbm == other.dbm && dim == other.dim; }
        bool operator!=(const reader& other) const { return !(*this == other); }
        /** Checks if the dbm has data. */
        operator bool() const { return dbm != nullptr && dim != 0; }
        /** Converts to raw_t pointer (compatibility with C function calls). */
        operator const raw_t*() const { return dbm; }
    };

    /** Non-owning raw DBM wrapper with mutable access to raw bounds. Drop-in replacement for "raw_t*". */
    class writer
    {
        raw_t* dbm;
        cindex_t dim;

    public:
        writer(raw_t* dbm, cindex_t dim): dbm{dbm}, dim{dim} {}
        raw_t* get() const { return dbm; }  ///< needed only for doctest-2.4.8/NIX, can be removed later
        /** Returns the number of dimensions/clocks in this DBM. */
        cindex_t get_dim() const { return dim; }
        /** Returns the {i,j} bound of DBM. */
        raw_t& at(cindex_t i, cindex_t j) { return dbm[i * dim + j]; }
        /** Returns the {i,j} bound of DBM. */
        raw_t at(cindex_t i, cindex_t j) const { return dbm[i * dim + j]; }
        /** Returns the integer bound at {i,j} of DBM with strictness bit removed. */
        int32_t bound(cindex_t i, cindex_t j) const { return dbm_raw2bound(at(i, j)); }
        /** Returns just the strictness bit of the bound at {i,j} of the DBM. */
        bool is_strict(cindex_t i, cindex_t j) const { return dbm_rawIsStrict(at(i, j)); }
        /** Trivial comparison. */
        bool operator==(const writer& other) const { return dbm == other.dbm && dim == other.dim; }
        bool operator!=(const writer& other) const { return !(*this == other); }
        /** Emulate pointer arithmetic. */
        writer operator+(std::ptrdiff_t offset) const
        {
            auto res = writer{*this};
            res += offset;
            return res;
        }
        /** Emulate pointer arithmetic. */
        writer operator-(std::ptrdiff_t offset) const
        {
            auto res = writer{*this};
            res -= offset;
            return res;
        }
        /** Emulate pointer arithmetic. */
        writer& operator-=(std::ptrdiff_t offset)
        {
            dbm -= offset;
            return *this;
        }
        /** Emulate pointer arithmetic. */
        writer& operator+=(std::ptrdiff_t offset)
        {
            dbm += offset;
            return *this;
        }
        /** Emulate pointer/iterator pre-increment. */
        writer& operator++()
        {
            ++dbm;
            return *this;
        }
        /** Emulate pointer/iterator. */
        raw_t& operator*() { return *dbm; }
        /** Emulate pointer/iterator. */
        raw_t* operator->() { return dbm; }
        /** Check if not null. */
        operator bool() const { return dbm != nullptr && dim != 0; }
        /** Converts to raw_t pointer (compatibility with C function calls). */
        operator raw_t*() { return dbm; }
        /** Convert to read-only access. */
        operator reader() { return {dbm, dim}; }
    };

}  // namespace dbm

#endif  // INCLUDE_DBM_RAW_HPP