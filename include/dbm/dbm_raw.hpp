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
        cindex_t get_dim() const { return dim; }
        raw_t at(cindex_t i, cindex_t j) const { return dbm[i * dim + j]; }
        operator bool() const { return dbm != nullptr && dim != 0; }
        operator const raw_t*() const { return dbm; }
    };

    /** Non-owning raw DBM wrapper with mutable access to raw bounds. Drop-in replacement for "raw_t*". */
    class writer
    {
        raw_t* dbm;
        cindex_t dim;

    public:
        writer(raw_t* dbm, cindex_t dim): dbm{dbm}, dim{dim} {}
        cindex_t get_dim() const { return dim; }
        raw_t& at(cindex_t i, cindex_t j) { return dbm[i * dim + j]; }
        /** Emulate pointer assignment. */
        writer& operator=(raw_t* ptr)
        {
            dbm = ptr;
            return *this;
        }
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
        /** Emulate pointer iteration. */
        writer& operator++()
        {
            ++dbm;
            return *this;
        }
        /** Emulate pointer. */
        raw_t& operator*() { return *dbm; }
        /** Emulate pointer. */
        raw_t* operator->() { return dbm; }
        /** Check if not null. */
        operator bool() const { return dbm != nullptr && dim != 0; }
        /** Convert to C pointer access. */
        operator raw_t*() { return dbm; }
        /** Convert to read-only access. */
        operator reader() { return {dbm, dim}; }
    };

}  // namespace dbm

#endif  // INCLUDE_DBM_RAW_HPP