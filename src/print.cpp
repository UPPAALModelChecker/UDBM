/* -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/*********************************************************************
 *
 * Filename : print.cpp (dbm)
 *
 * This file is a part of the UPPAAL toolkit.
 * Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
 * All right reserved.
 *
 * $Id: print.cpp,v 1.5 2005/05/24 19:13:24 adavid Exp $
 *
 *********************************************************************/

#include "dbm/print.h"

#include "dbm/dbm.h"
#include "dbm/fed.h"
#include "io/FileStreamBuffer.h"
#include "base/bitstring.h"

/* For debugging */
static inline void ASSERT_DIAG_OK(raw_t* DBM, cindex_t DIM)
{
    ASSERT(dbm_isDiagonalOK(DBM, DIM), dbm_print(stderr, DBM, DIM));
}

/** For easy conversion FILE* to ostream */
class file_ostream
{
    io::FileStreamBuffer buffer;
    std::ostream os;

public:
    file_ostream(FILE* file): buffer{file}, os{&buffer} {}
    operator std::ostream&() { return os; }
};

static const char* _print_prefix = "";
static bool _ruby_format = false;

/* Wrappers */
namespace dbm
{
    std::ostream& operator<<(std::ostream& os, const dbm::dbm_t& dbm) { return dbm_cppPrint(os, dbm.dbm_read()); }

    std::ostream& operator<<(std::ostream& os, const dbm::fed_t& fed)
    {
        size_t size = fed.size();
        cindex_t dim = fed.getDimension();
        if (size == 0) {
            return _ruby_format ? os << "Fed(" << dim << ") {}" : os << size << " DBM " << dim << 'x' << dim << "{}";
        }
        if (_ruby_format) {
            os << "Fed(" << dim << ") {";
            if (size > 1) {
                os << "[\n" << _print_prefix;
            }
            os << " matrix\\\n";
        } else {
            os << size << (size > 1 ? " DBMs " : " DBM ") << dim << 'x' << dim << " {\n";
        }

        if (dim == 1) {
            assert(!fed.isEmpty());
            dbm_cppPrintRaw(os, dbm_LE_ZERO);
            if (_ruby_format) {
                return os << " }";
            }
            os << '\n';
        } else {
            for (dbm::fed_t::const_iterator iter(fed); !iter.null();) {
                dbm_cppPrint(os, iter);
                ++iter;
                if (!iter.null()) {
                    os << _print_prefix << (_ruby_format ? ",matrix\\\n" : ",\n");
                }
            }
        }

        return os << _print_prefix << (_ruby_format && size > 1 ? "]}" : "}");
    }
}  // namespace dbm

std::ostream& operator<<(std::ostream& os, const constraint_t& c)
{
    return dbm_cppPrintRaw(os << 'x' << c.i << '-' << 'x' << c.j, c.value);
}

void dbm_setPrintPrefix(const char* prefix)
{
    if (prefix != nullptr) {
        _print_prefix = prefix;
    } else {
        _print_prefix = "";
    }
}

void dbm_setRubyFormat(bool mode) { _ruby_format = mode; }

/* Call print raw on all constraints.
 */
std::ostream& dbm_cppPrint(std::ostream& os, dbm::reader dbm)
{
    const auto dim = dbm.get_dim();
    if (dbm != nullptr) {
        for (cindex_t i = 0; i < dim; ++i) {
            os << _print_prefix;
            for (cindex_t j = 0; j < dim; ++j) {
                dbm_cppPrintRaw(os, dbm.at(i, j)) << '\t';
            }
            os << "\\\n";
        }
    }

    return os;
}
void dbm_print(FILE* file, const raw_t* dbm, cindex_t dim)
{
    if (dbm != nullptr) {
        auto fos = file_ostream{file};
        dbm_cppPrint(fos, {dbm, dim});
    }
}

/* Shortcuts to print the difference highlight
 * depending on the activation of the colors.
 */
#ifndef NPRETTY_COLORS
static inline void PRE_DIFF(std::ostream& os, const raw_t diff)
{
    if (diff != 0)
        os << RED(BOLD);
}
static inline void POST_DIFF(std::ostream& os, const raw_t diff)
{
    if (diff != 0)
        os << NORMAL;
}
#else
static inline void PRE_DIFF(std::ostream& os, bool diff) {}
static inline void POST_DIFF(std::ostream& os, bool diff)
{
    if (diff)
        os << '*';
}
#endif

/* Similar to print but do it twice and mark
 * the difference between the DBMs.
 */
std::ostream& dbm_cppPrintDiff(std::ostream& os, dbm::reader src, dbm::reader dst)
{
    cindex_t i, j;
    assert(src && dst);
    assert(src.get_dim() == dst.get_dim());

    const auto dim = src.get_dim();
    os << "DBM diff " << dim << 'x' << dim << ":\n";

    for (i = 0; i < dim; ++i) {
        raw_t diff = src.at(i, 0) ^ dst.at(i, 0);
        PRE_DIFF(os, diff);
        dbm_cppPrintRaw(os, src.at(i, 0));
        POST_DIFF(os, diff);
        for (j = 1; j < dim; ++j) {
            os << '\t';
            diff = src.at(i, j) ^ dst.at(i, j);
            PRE_DIFF(os, diff);
            dbm_cppPrintRaw(os, src.at(i, j));
            POST_DIFF(os, diff);
        }
        os << '\n';
    }

    os << '\n';
    for (i = 0; i < dim; ++i) {
        raw_t diff = src.at(i, 0) ^ dst.at(i, 0);
        PRE_DIFF(os, diff);
        dbm_cppPrintRaw(os, dst.at(i, 0));
        POST_DIFF(os, diff);
        for (j = 1; j < dim; ++j) {
            os << '\t';
            diff = src.at(i, 0) ^ dst.at(i, 0);
            PRE_DIFF(os, diff);
            dbm_cppPrintRaw(os, dst.at(i, j));
            POST_DIFF(os, diff);
        }
        os << '\n';
    }
    return os;
}
void dbm_printDiff(FILE* file, const raw_t* src, const raw_t* dst, cindex_t dim)
{
    dbm_cppPrintDiff(file_ostream{file}, {src, dim}, {dst, dim});
}

#undef PRE_DIFF
#undef POST_DIFF

/* - copy the original DBM
 * - close the copy
 * - print the difference
 */
std::ostream& dbm_cppPrintCloseDiff(std::ostream& out, dbm::reader dbm)
{
    assert(dbm);
    const auto dim = dbm.get_dim();
    auto copy = std::vector<raw_t>(dim * dim, 0);
    dbm_copy(copy.data(), dbm, dim);
    dbm_close(copy.data(), dim);
    if (dbm_isEmpty(copy.data(), dim)) {
        out << RED(BOLD) "Warning: empty DBM!" NORMAL " ";
    }
    dbm_cppPrintDiff(out, {dbm, dim}, {copy.data(), dim});

    return out;
}
void dbm_printCloseDiff(FILE* file, const raw_t* dbm, cindex_t dim)
{
    dbm_cppPrintCloseDiff(file_ostream{file}, {dbm, dim});
}

/* check for infinity values
 * before printing.
 */
std::ostream& dbm_cppPrintRaw(std::ostream& out, raw_t c)
{
    return dbm_cppPrintBound(out << (dbm_rawIsWeak(c) ? "<=" : "<"), dbm_raw2bound(c));
}
void dbm_printRaw(FILE* file, raw_t c) { dbm_cppPrintRaw(file_ostream{file}, c); }

/* Test for infinity and valid values.
 */
std::ostream& dbm_cppPrintBound(std::ostream& os, int32_t bound)
{
    if (bound == dbm_INFINITY) {
        return os << "INF";
    } else if (bound == -dbm_INFINITY) {
        return os << "-INF";
    } else if (bound > dbm_INFINITY || bound < -dbm_INFINITY) {
        return os << "illegal";
    } else {
        return os << bound;
    }
}
void dbm_printBound(FILE* file, int32_t bound) { dbm_cppPrintBound(file_ostream{file}, bound); }

/* Format: [a b c]
 * Print 1st element separately and other preceded by ' '.
 */
std::ostream& dbm_cppPrintRaws(std::ostream& os, const raw_t* data, size_t size)
{
    assert(size == 0 || data);

    os << '[';
    if (size != 0) {
        size_t i;
        dbm_cppPrintRaw(os, data[0]);
        for (i = 1; i < size; ++i) {
            dbm_cppPrintRaw(os << ' ', data[i]);
        }
    }
    return os << ']';
}
void dbm_printRaws(FILE* file, const raw_t* data, size_t size) { dbm_cppPrintRaws(file_ostream{file}, data, size); }

/* Print with the format [a b c]
 */
std::ostream& dbm_cppPrintBounds(std::ostream& os, const int32_t* data, size_t size)
{
    assert(size == 0 || data);

    os << '[';
    for (size_t i = 0; i < size; ++i) {
        if (i > 0)
            os << ' ';
        dbm_cppPrintBound(os, data[i]);
    }
    return os << ']';
}
void dbm_printBounds(FILE* file, const int32_t* data, size_t size)
{
    dbm_cppPrintBounds(file_ostream{file}, data, size);
}

std::ostream& dbm_cppPrintConstraints(std::ostream& os, const constraint_t* c, size_t n)
{
    assert(c);
    for (size_t i = 0; i < n; ++i) {
        if (i > 0)
            os << ' ';
        dbm_cppPrintRaw(os << '(' << c[i].i << ")-(" << c[i].j << ')', c[i].value);
    }
    return os;
}
void dbm_printConstraints(FILE* file, const constraint_t* c, size_t n)
{
    dbm_cppPrintConstraints(file_ostream{file}, c, n);
}
