// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-
////////////////////////////////////////////////////////////////////
//
// Filename : fed.cpp
//
// This file is a part of the UPPAAL toolkit.
// Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
// All right reserved.
//
// $Id: fed_dbm.cpp,v 1.20 2005/11/22 12:18:59 adavid Exp $$
//
///////////////////////////////////////////////////////////////////

#include "DBMAllocator.h"
#include "dbm.h"

#include "dbm/config.h"
#include "base/bitstring.h"
#include "base/doubles.h"

#include <sstream>
#include <vector>
#include <cmath>

namespace dbm
{
    /********************
     * idbm_t
     ********************/

    // 1: Remove from the hash table
    // 2: Deallocate
    // Not inlined to reduce code size.
    void idbm_t::remove()
    {
        assert(refCounter == 0);
        if (isHashed()) {
            dbm_table.remove(this);
        }
        dbm_delete(this);
    }

#ifdef ENABLE_STORE_MINGRAPH
    const uint32_t* idbm_t::getMinGraph(size_t* size)
    {
        RECORD_NSTAT(MAGENTA(BOLD) "idbm_t::getMinGraph cache", isValid() ? "hit" : "miss");

        auto dim = getDimension();
        auto* ming = (uint32_t*)&matrix[dim * dim];
        if (!isValid()) {
            minSize = dbm_analyzeForMinDBM(matrix, dim, ming);
        }
        *size = minSize;
        return ming;
    }

    void idbm_t::copyMinGraphTo(idbm_t* arg)
    {
        assert(isValid() && !arg->isValid());
        assert(getDimension() == arg->getDimension());
        assert(dbm_areEqual(matrix, arg->matrix, getDimension()));

        arg->minSize = minSize;
        size_t d = getDimension();
        d *= d;
        std::copy(&matrix[d], &matrix[d] + bits2intsize(d), &arg->matrix[d]);
    }
#endif

    /********************
     * dbm_t
     ********************/

    static size_t dbm_fillBitMatrix(const raw_t* dbm, cindex_t dim, uint32_t* mingraph)
    {
        base_resetBits(mingraph, bits2intsize(dim * dim));

        for (cindex_t i = 0; i < dim; ++i) {
            for (cindex_t j = 0; j < dim; ++j) {
                if (i != j && dbm[i * dim + j] < dbm_LS_INFINITY) {
                    base_setOneBit(mingraph, i * dim + j);
                }
            }
        }
        return dim * dim - dim;
    }

    std::ostream& dbm_t::print(std::ostream& os, const ClockAccessor& access, bool full) const
    {
        if (isEmpty())
            return os << "false";

        const cindex_t dim = pdim();
        std::vector<uint32_t> mingraph = std::vector<uint32_t>(bits2intsize(dim * dim));
        auto dbm = dbm_read();
        size_t n = full
                       ? dbm_fillBitMatrix(dbm, dim, mingraph.data())
                       : dbm_cleanBitMatrix(dbm, dim, mingraph.data(), dbm_analyzeForMinDBM(dbm, dim, mingraph.data()));
        if (n == 0)
            return os << "true";

        bool firstConstraint = true;

        os << "(";
        for (cindex_t i = 0; i < dim; ++i) {
            for (cindex_t j = 0; j < dim; ++j) {
                if (base_readOneBit(mingraph.data(), i * dim + j) != 0) {
                    assert(i != j);  // Not part of mingraph.

                    // Conjunction of constraints.
                    if (!firstConstraint)
                        os << " && ";
                    firstConstraint = false;

                    // Write constraints xi-xj <|<= DBM[i,j]
                    if (i == 0) {
                        assert(j != 0);  // 0,0 never part of mingraph
                        if (dbm_addFiniteRaw(dbm[j], dbm[j * dim]) == dbm_LE_ZERO) {
                            assert(n);
                            n -= base_getOneBit(mingraph.data(), j * dim);
                            base_resetOneBit(mingraph.data(), j * dim);

                            // xj == b
                            os << access.getClockName(j) << "==" << -dbm.bound(0, j);
                        } else {  // b < xj, b <= xj
                            os << -dbm.bound(0, j) << (dbm.is_strict(0, j) ? "<" : "<=") << access.getClockName(j);
                        }
                    } else {
                        // xi-xj == b ?
                        if (dbm_addFiniteRaw(dbm.at(i, j), dbm.at(j, i)) == dbm_LE_ZERO) {
                            assert(n);
                            n -= base_getOneBit(mingraph.data(), j * dim + i);
                            base_resetOneBit(mingraph.data(), j * dim + i);

                            if (j > 0 && dbm.at(i, j) == dbm_LE_ZERO) {  // xi == xj
                                os << access.getClockName(i) << "==" << access.getClockName(j);
                            } else {
                                if (j == 0) {        // xi < b, xi <= b
                                    assert(i != 0);  // 0,0 never part of mingraph
                                    os << access.getClockName(i);
                                } else {  // xi-xj < b, xi-xj <= b
                                    os << access.getClockName(i) << "-" << access.getClockName(j);
                                }
                                os << "==" << dbm.bound(i, j);
                            }
                        } else {
                            if (j > 0 && dbm.bound(i, j) == 0) {  // xi < xj, xi <= xj
                                os << access.getClockName(i) << (dbm.is_strict(i, j) ? "<" : "<=")
                                   << access.getClockName(j);
                            } else {
                                if (j == 0) {        // xi < b, xi <= b
                                    assert(i != 0);  // 0,0 never part of mingraph
                                    os << access.getClockName(i);
                                } else {  // xi-xj < b, xi-xj <= b
                                    os << access.getClockName(i) << "-" << access.getClockName(j);
                                }
                                // < b, <= b
                                os << (dbm.is_strict(i, j) ? "<" : "<=") << dbm.bound(i, j);
                            }
                        }
                    }
                    assert(n);
                    --n;  // Consumed one constraint.
                    if (n == 0)
                        goto stop_loops;
                }
            }
        }
    stop_loops:
        os << ")";
        return os;
    }

    std::string dbm_t::str(const ClockAccessor& access, bool full) const
    {
        auto os = std::ostringstream{};
        print(os, access, full);
        return os.str();
    }

    dbm_t dbm_t::makeUnbounded(const int32_t* low, cindex_t dim)
    {
        dbm_t res;
        raw_t* dbm = res.setNew(dim);
        for (cindex_t i = 1; i < dim; ++i) {
            dbm[i] = low[i] != -dbm_INFINITY ? dbm_bound2raw(-low[i], dbm_STRICT) : dbm_LE_ZERO;
        }
        std::fill(dbm + dim, dbm + (dim * (dim - 1)), dbm_LS_INFINITY);
        for (cindex_t i = 0; i < dim; ++i) {
            *dbm = dbm_LE_ZERO;
            dbm += dim + 1;
        }
        return res;
    }

    bool dbm_t::hasZero() const { return !isEmpty() && dbm_hasZero(const_dbm(), pdim()); }

    bool dbm_t::canDelay() const
    {
        if (isEmpty())
            return false;
        auto dbm = dbm_read();
        const cindex_t dim = pdim();
        for (cindex_t i = 1; i < dim; ++i) {
            if (dbm.is_weak(i, 0) && dbm.is_weak(0, i) && dbm.at(i, 0) == dbm_weakNegRaw(dbm.at(0, i))) {
                return false;
            }
        }
        return true;
    }

    void dbm_t::ptr_intern()
    {
        idbm_t* dbm = idbmt();

        // Never hash twice
        if (!dbm->isHashed()) {
            dbm->markHashed();  // for equality test
            idbm_t** entry = dbm_table.getAtBucket(dbm->updateHash());
            for (idbm_t* bucket = *entry; bucket != nullptr; bucket = bucket->getNext()) {
                assert(bucket->isHashed());
                if (dbm->info == bucket->info &&  // test hash & dim
                    dbm_areEqual(dbm->const_dbm(), bucket->const_dbm(), dbm->info & MAX_DIM)) {
                    assert(bucket != dbm);
                    bucket->incRef();
                    setPtr(bucket);
#ifdef ENABLE_STORE_MINGRAPH
                    // Don't loose the mingraph if we have it.
                    if (!bucket->isValid() && dbm->isValid()) {
                        dbm->copyMinGraphTo(bucket);
                    }
#endif
                    dbm->unmarkHashed();
                    dbm->decRef();
                    return;
                }
            }
            dbm->link(entry);
            dbm_table.incBuckets();
        }
    }

    void dbm_t::copyFrom(const raw_t* src, cindex_t dim)
    {
        assert(dim && src);
        assertx(dbm_isValid(src, dim));

        if (!isEmpty()) {
            if (dim == pdim()) {
                if (!dbm_areEqual(const_dbm(), src, dim)) {
                    dbm_copy(getNew(), src, dim);
                }
                return;
            }
            decRef();
        }
        dbm_copy(setNew(dim), src, dim);
    }

    void dbm_t::copyTo(raw_t* dst, cindex_t dim) const
    {
        assert(dim == getDimension() && dim && dst);

        if (isEmpty()) {
            *dst = -1;  // mark as empty
        } else {
            dbm_copy(dst, const_dbm(), dim);
        }
    }

    bool dbm_t::operator==(const dbm_t& arg) const
    {
        assert(idbmPtr && arg.idbmPtr);

        if (sameAs(arg)) {
            return true;
        }
        // if one is empty or different dimensions
        else if (((uval() | arg.uval()) & 1) != 0 || pdim() != arg.pdim()) {
            return false;
        } else {
            return dbm_areEqual(const_dbm(), arg.const_dbm(), pdim());
        }
    }

    bool dbm_t::operator==(const raw_t* arg) const
    {
        assertx(dbm_isValid(arg, getDimension()));
        return !isEmpty() && dbm_areEqual(const_dbm(), arg, pdim());
    }

    bool dbm_t::operator<=(const dbm_t& arg) const
    {
        if (sameAs(arg)) {
            return true;
        }

        const cindex_t dim = getDimension();
        return (dim == arg.getDimension()) &&
               (isEmpty() || (!arg.isEmpty() && dbm_isSubsetEq(const_dbm(), arg.const_dbm(), pdim())));
    }

    bool dbm_t::operator<(const raw_t* arg) const
    {
        return isEmpty() || dbm_relation(const_dbm(), arg, pdim()) == base_SUBSET;
    }

    bool dbm_t::operator>(const raw_t* arg) const
    {
        return !isEmpty() && dbm_relation(const_dbm(), arg, pdim()) == base_SUPERSET;
    }

    relation_t dbm_t::relation(const dbm_t& arg) const
    {
        assert(idbmPtr && arg.idbmPtr);

        if (sameAs(arg)) {
            return base_EQUAL;
        }

        const cindex_t dim = getDimension();
        if (dim != arg.getDimension()) {
            return base_DIFFERENT;
        } else if (isEmpty()) {
            return base_SUBSET;
        } else if (arg.isEmpty()) {
            return base_SUPERSET;
        } else {
            return dbm_relation(const_dbm(), arg.const_dbm(), dim);
        }
    }

    relation_t dbm_t::relation(const raw_t* arg, cindex_t dim) const
    {
        assertx(dbm_isValid(arg, dim));  // in particular arg not empty
        return isEmpty() ? (dim == edim() ? base_SUBSET : base_DIFFERENT)
                         : (dim == pdim() ? dbm_relation(const_dbm(), arg, dim) : base_DIFFERENT);
    }

    // try to avoid to write or copy if possible
    dbm_t& dbm_t::setZero()
    {
        RECORD_STAT();
        cindex_t dim;
        if (isEmpty()) {
            RECORD_SUBSTAT("empty");
            dim = edim();
            if (dim == 1) {
#ifndef ENABLE_DBM_NEW
                newCopy(DBMAllocator::instance().dbm1());
#else
                newCopy(dbm::dbm1());
#endif
                return *this;
            }
        } else {
            dim = pdim();
            if (tryMutable()) {
                RECORD_SUBSTAT("mutable");
                dbm_zero(dbm(), dim);
                return *this;
            } else if (dbm_isEqualToZero(const_dbm(), dim)) {
                RECORD_SUBSTAT("isZero");
                return *this;
            }
            idbmt()->decRefImmutable();
        }
        dbm_zero(setNew(dim), dim);
        return *this;
    }

    // try to avoid to write or copy if possible
    dbm_t& dbm_t::setInit()
    {
        RECORD_STAT();
        cindex_t dim;
        if (isEmpty()) {
            RECORD_SUBSTAT("empty");
            dim = edim();
            if (dim == 1) {
#ifndef ENABLE_DBM_NEW
                newCopy(DBMAllocator::instance().dbm1());
#else
                newCopy(dbm::dbm1());
#endif
                return *this;
            }
        } else {
            dim = pdim();
            if (tryMutable()) {
                RECORD_SUBSTAT("mutable");
                dbm_init(dbm(), dim);
                return *this;
            } else if (dbm_isEqualToInit(const_dbm(), dim)) {
                RECORD_SUBSTAT("isInit");
                return *this;
            }
            idbmt()->decRefImmutable();
        }
        dbm_init(setNew(dim), dim);
        return *this;
    }

    int32_t dbm_t::getUpperMinimumCost(int32_t cost) const
    {
        assert(cost >= 0 && ((cindex_t)cost) < getDimension());

        if (isEmpty()) {
            return -dbm_INFINITY;
        }

        int32_t bound = -dbm_INFINITY;
        auto dbm = dbm_read();
        const cindex_t dim = pdim();

        for (cindex_t i = 1; i < dim; ++i) {
            if (dbm.at(i, 0) < dbm_LS_INFINITY) {
                // dbm.at(i,cost) <= dbm.at(i,0) + dbm.at(0,cost) with dbm.at(0,cost) <= 0
                assert(dbm.at(i, cost) < dbm_LS_INFINITY);
                bound = std::max(bound, dbm.bound(i, 0) - dbm.bound(i, cost));
            }
        }

        return bound == -dbm_INFINITY ? dbm_INFINITY : bound;
    }

    // if this >= arg return this
    // if this < arg return arg
    // else convex union
    dbm_t& dbm_t::operator+=(const dbm_t& arg)
    {
        assert(getDimension() == arg.getDimension());
        RECORD_STAT();

        if (arg.isEmpty())  // anything + empty = anything
        {
            RECORD_SUBSTAT("empty arg");
            return *this;
        } else if (isEmpty())  // empty + anything = anything
        {
            RECORD_SUBSTAT("empty self");
            newCopy(arg);
            return *this;
        } else {
            RECORD_SUBSTAT("real union");
            return ptr_convexUnion(arg.const_dbm(), pdim());
        }
    }

    // go through fed and += its dbm_t
    dbm_t& dbm_t::operator+=(const fed_t& fed)
    {
        assert(getDimension() == fed.getDimension());

        const cindex_t dim = getDimension();
        for (const auto& iter : fed) {
            assert(!iter.isEmpty());
            if (isEmpty()) {
                newCopy(iter);
            } else {
                ptr_convexUnion(iter.const_dbm(), dim);
            }
        }
        return *this;
    }

    // similar to += dbm_t, assume same dimension
    dbm_t& dbm_t::operator+=(const raw_t* arg)
    {
        assertx(dbm_isValid(arg, getDimension()));
        RECORD_STAT();

        if (isEmpty()) {
            RECORD_SUBSTAT("empty self");
            copyFrom(arg, edim());
            return *this;
        } else {
            return ptr_convexUnion(arg, pdim());
        }
    }

    dbm_t& dbm_t::driftWiden()
    {
        // This call makes sense only after delay.
        assert(!isEmpty() && isUnbounded());

        size_t dim = pdim();
        auto mdbm = getCopy_write();

        /* Open lower bounds. */
        for (size_t i = 1; i < dim; ++i) {
            mdbm.at(0, i) = dbm_strictRaw(mdbm.at(0, i));
        }

        /* Widen closed diagonals. */
        for (size_t i = 0; i < dim; ++i) {
            for (size_t j = 0; j < dim; ++j) {
                if (i != j && dbm_rawIsWeak(mdbm.at(i, j))) {
                    mdbm.at(i, j)++;
                }
            }
        }

        assertx(dbm_isClosed(mdbm, dim));
        return *this;
    }

    // implementation of convex union with pre-conditions
    dbm_t& dbm_t::ptr_convexUnion(const raw_t* arg, cindex_t dim)
    {
        assert(!isEmpty() && dim && getDimension() == dim);
        RECORD_STAT();

        if (tryMutable()) {
            RECORD_SUBSTAT("mutable");
            dbm_convexUnion(dbm(), arg, dim);
        } else if (dim > 1) {  // if something to do
            const raw_t* cdbm = const_dbm();
            size_t n = dim * dim - 2;  // skip 1st & last <=0 of diagonal
            while (*++cdbm >= *++arg && --n != 0)
                ;          // check superset
            if (n != 0) {  // this is not a superset
                RECORD_SUBSTAT("copy");
                size_t past = cdbm - const_dbm();  // before calling icopy!
                raw_t* mdbm = icopy(dim) + past;
                assert(*mdbm < *arg);
                *mdbm = *arg;  // continue convex union
                while (--n != 0) {
                    if (*++mdbm < *++arg)
                        *mdbm = *arg;
                }
            }
        }
        return *this;
    }

    dbm_t& dbm_t::operator&=(const dbm_t& arg)
    {
        RECORD_STAT();
        if (!isEmpty()) {
            const cindex_t dim = pdim();
            if (arg.isEmpty()) {
                RECORD_SUBSTAT("empty arg");
                empty(dim);
            } else if (ptr_intersectionIsArg(arg.const_dbm(), dim)) {
                RECORD_SUBSTAT("self == arg");
                updateCopy(arg);
            }
        }
        return *this;
    }

    dbm_t& dbm_t::operator&=(const raw_t* arg)
    {
        RECORD_STAT();
        if (!isEmpty()) {
            const cindex_t dim = pdim();
            if (ptr_intersectionIsArg(arg, dim)) {
                RECORD_SUBSTAT("self == arg");
                updateCopy(arg, dim);
            }
        }
        return *this;
    }

    /* Matrix corresponding to axis x:
     * dbm[i,j] = <=0 with free(x)
     */
    bool dbm_t::intersectionAxis(cindex_t x)
    {
        if (isEmpty()) {
            return false;
        }

        const cindex_t dim = pdim();
        auto mdbm = getCopy_write();
        std::vector<uint32_t> touched = std::vector<uint32_t>(bits2intsize(dim));
        uint32_t count = 0;
        cindex_t ci = 0, cj = 0;
        base_resetBits(touched.data(), bits2intsize(dim));
        for (cindex_t i = 0; i < dim; ++i)
            if (i != x) {
                for (cindex_t j = 0; j < dim; ++j)
                    if (j != x && i != j) {
                        if (mdbm.at(i, j) > dbm_LE_ZERO) {
                            mdbm.at(i, j) = dbm_LE_ZERO;
                            if (dbm_negRaw(dbm_LE_ZERO) >= mdbm.at(j, i)) {
                                mdbm.at(0, 0) = -1;
                                return false;
                            }
                            count++;
                            ci = i;
                            cj = j;
                            base_setOneBit(touched.data(), i);
                            base_setOneBit(touched.data(), j);
                        }
                    }
            }
        if (count > 1) {
            return dbm_closex(mdbm, dim, touched.data());
        } else if (count == 1) {
            dbm_closeij(mdbm, dim, ci, cj);
        }
        assert(!dbm_isEmpty(mdbm, dim));
        return true;
    }

    bool dbm_t::ptr_intersectionIsArg(const raw_t* arg, cindex_t dim)
    {
        assert(getDimension() == dim && !isEmpty());
        RECORD_STAT();

        if (tryMutable()) {
            RECORD_SUBSTAT("mutable");
            if (!dbm_intersection(dbm(), arg, dim)) {
                emptyMutable(dim);
            }
        } else {
            switch (dbm_relation(const_dbm(), arg, dim)) {
            case base_DIFFERENT:
                RECORD_SUBSTAT("this!=arg");
                if (!dbm_haveIntersection(const_dbm(), arg, dim)) {
                    RECORD_SUBSTAT("no copy");
                    emptyImmutable(dim);
                } else if (!dbm_intersection(icopy(dim), arg, dim)) {
                    emptyMutable(dim);
                }
                break;

            case base_SUPERSET:
                RECORD_SUBSTAT("this>arg");
                // we can skip a copy if arg comes from a dbm_t
                return true;  // this > arg => this & arg = arg

            case base_SUBSET:  // this <= arg => this & arg = this
            case base_EQUAL: RECORD_SUBSTAT("this<=arg");
            }
        }
        return false;
    }

    bool dbm_t::ptr_constrain(cindex_t i, cindex_t j, raw_t c)
    {
        const cindex_t dim = pdim();
        auto cdbm = dbm_read();
        assert(i < dim && j < dim);
        RECORD_STAT();

        if (cdbm.at(i, j) > c)  // if it is a tightening
        {
            RECORD_SUBSTAT("tighten");
            if (dbm_negRaw(c) >= cdbm.at(j, i))  // too tight!
            {
                RECORD_SUBSTAT("result empty");
                empty(dim);
                return false;
            } else  // not too tight then do it!
            {
                RECORD_SUBSTAT(isMutable() ? "mutable" : "copy");
                auto mdbm = getCopy_write();  // mutable
                mdbm.at(i, j) = c;
                dbm_closeij(mdbm, dim, i, j);
            }
        }
        return true;
    }

    // see dbm_constrainClock
    bool dbm_t::ptr_constrain(cindex_t k, int32_t value)
    {
        assert(getDimension() > k && k > 0 && value != dbm_INFINITY);
        RECORD_STAT();

        const cindex_t dim = pdim();
        const raw_t* cdbm = const_dbm();
        raw_t upper = dbm_bound2raw(value, dbm_WEAK);
        raw_t lower = dbm_bound2raw(-value, dbm_WEAK);
        uint32_t kdim = k * dim;
        bool up = cdbm[kdim] > upper;
        bool lo = cdbm[k] > lower;

        if (up || lo) {
            raw_t newUp = up ? upper : cdbm[kdim];
            raw_t newLo = lo ? lower : cdbm[k];

            // Trivial empty.
            if (dbm_negRaw(newUp) >= newLo) {
                RECORD_SUBSTAT("trivial empty");
                empty(dim);
                return false;
            }

            raw_t* mdbm = getCopy();  // mutable

            // constrain both upper && lower
            mdbm[kdim] = newUp;
            mdbm[k] = newLo;

            // test if empty or not after constrain + close
            if (!(dbm_close1(mdbm, dim, 0) && dbm_close1(mdbm, dim, k))) {
                RECORD_SUBSTAT("close empty");
                emptyMutable(dim);
                return false;
            }
        }
        return true;
    }

    // Try to avoid changing this DBM whenever possible:
    // look at all the constraints and
    // - if one tightens too much, return empty
    // - if a constraint tightens somehow, remember it
    // then tighten all constraints and closex
    bool dbm_t::ptr_constrain(const constraint_t* cnstr, size_t n)
    {
        const cindex_t dim = pdim();
        RECORD_STAT();
        if (tryMutable()) {
            RECORD_SUBSTAT("mutable");
            if (!dbm_constrainN(dbm(), dim, cnstr, n)) {
                RECORD_SUBSTAT("mutable empty");
                emptyMutable(dim);
                return false;
            }
        } else {
            std::vector<cindex_t> ck(n);  // to remember "good" constraints
            auto cdbm = dbm_read();       // const dbm
            cindex_t i, j, k, nk;
            raw_t c;

            // check first
            for (nk = 0, k = 0; k < n; ++k) {
                assert(cnstr);

                i = cnstr[k].i;
                j = cnstr[k].j;
                c = cnstr[k].value;
                assert(i < dim && j < dim && i != j);

                // if it is a tightening
                if (cdbm.at(i, j) > c) {
                    // but too tight
                    if (dbm_negRaw(c) >= cdbm.at(j, i)) {
                        RECORD_SUBSTAT("immutable empty");
                        emptyImmutable(dim);
                        return false;
                    }
                    // else not too tight and remember this constraint
                    ck[nk++] = k;
                }
            }
            RECORD_SUBSTAT(nk != 0 ? "copy" : "skip");

            if (nk != 0)  // something to do?
            {
                auto mdbm = icopy_write(dim);  // get mutable

                if (nk == 1)  // easy with closeij: safe constraint, checked
                {
                    i = cnstr[ck[0]].i;
                    j = cnstr[ck[0]].j;
                    mdbm.at(i, j) = cnstr[ck[0]].value;
                    dbm_closeij(mdbm, dim, i, j);
                } else  // constrain them all!
                {
                    std::vector<uint32_t> touched(bits2intsize(dim), 0);
                    k = 0;
                    do {
                        i = cnstr[ck[k]].i;
                        j = cnstr[ck[k]].j;
                        // test in case of multiple constraints on the same i,j!
                        if (mdbm.at(i, j) > cnstr[ck[k]].value) {
                            mdbm.at(i, j) = cnstr[ck[k]].value;
                        }
                        base_setOneBit(touched.data(), i);
                        base_setOneBit(touched.data(), j);
                    } while (++k < nk);

                    if (!dbm_closex(mdbm, dim, touched.data())) {
                        RECORD_SUBSTAT("emptied copy");
                        emptyMutable(dim);
                        return false;
                    }
                }
            }
        }
        return true;
    }

    // Similar to previous constrain but with indirection!
    bool dbm_t::ptr_constrain(const cindex_t* table, const constraint_t* cnstr, size_t n)
    {
        assert(table);
        RECORD_STAT();
        const cindex_t dim = pdim();
        if (tryMutable()) {
            RECORD_SUBSTAT("mutable");
            if (!dbm_constrainIndexedN(dbm(), dim, table, cnstr, n)) {
                RECORD_SUBSTAT("mutable empty");
                emptyMutable(dim);
                return false;
            }
        } else {
            std::vector<cindex_t> ck(n);  // to remember "good" constraints
            auto cdbm = dbm_read();       // const dbm
            cindex_t i, j, k, nk;
            raw_t c;

            // check first
            for (nk = 0, k = 0; k < n; ++k) {
                assert(cnstr);

                i = table[cnstr[k].i];
                j = table[cnstr[k].j];
                c = cnstr[k].value;
                assert(i < dim && j < dim && i != j);

                // if it is a tightening
                if (cdbm.at(i, j) > c) {
                    // but too tight
                    if (dbm_negRaw(c) >= cdbm.at(j, i)) {
                        RECORD_SUBSTAT("immutable empty");
                        emptyImmutable(dim);
                        return false;
                    }
                    // else not too tight and remember this constraint
                    ck[nk++] = k;
                }
            }
            RECORD_SUBSTAT(nk != 0 ? "copy" : "skip");

            if (nk != 0)  // something to do?
            {
                auto mdbm = icopy_write(dim);  // get mutable

                if (nk == 1)  // easy with closeij
                {
                    i = table[cnstr[ck[0]].i];
                    j = table[cnstr[ck[0]].j];
                    mdbm.at(i, j) = cnstr[ck[0]].value;
                    dbm_closeij(mdbm, dim, i, j);
                    assert(!dbm_isEmpty(mdbm, dim));
                } else  // constrain them all!
                {
                    std::vector<uint32_t> touched(bits2intsize(dim), 0);
                    k = 0;
                    do {
                        i = table[cnstr[ck[k]].i];
                        j = table[cnstr[ck[k]].j];
                        // test in case of multiple constraints on the same i,j!
                        if (mdbm.at(i, j) > cnstr[ck[k]].value) {
                            mdbm.at(i, j) = cnstr[ck[k]].value;
                        }
                        base_setOneBit(touched.data(), i);
                        base_setOneBit(touched.data(), j);
                    } while (++k < nk);

                    if (!dbm_closex(mdbm, dim, touched.data())) {
                        RECORD_SUBSTAT("emptied copy");
                        emptyMutable(dim);
                        return false;
                    }
                }
            }
        }
        return true;
    }

    bool dbm_t::intersects(const dbm_t& arg) const
    {
        assert(getDimension() == arg.getDimension());
        return !isEmpty() && !arg.isEmpty() && dbm_haveIntersection(const_dbm(), arg.const_dbm(), pdim());
    }

    bool dbm_t::intersects(const fed_t& fed) const
    {
        assert(getDimension() == fed.getDimension());
        RECORD_STAT();
        if (isEmpty() || fed.isEmpty()) {
            RECORD_SUBSTAT("trivial empty");
            return false;
        } else {
            fed_t::const_iterator iter = fed.begin(), e = fed.end();
            const cindex_t dim = pdim();
            assert(iter != fed.end());
            do {
                if (dbm_haveIntersection(const_dbm(), iter->const_dbm(), dim)) {
                    RECORD_SUBSTAT("maybe yes");
                    return true;
                }
            } while (++iter != e);
            RECORD_SUBSTAT("no");
            return false;
        }
    }

    bool dbm_t::intersects(const raw_t* arg, cindex_t dim) const
    {
        assert(dim == getDimension());
        assertx(dbm_isValid(arg, dim));
        return !isEmpty() && dbm_haveIntersection(const_dbm(), arg, dim);
    }

    void dbm_t::ptr_up()
    {
        const cindex_t dim = pdim();
        RECORD_STAT();
        if (tryMutable()) {
            RECORD_SUBSTAT("mutable");
            dbm_up(dbm(), dim);  // mutable => write directly
        } else {
            cindex_t i;
            auto cdbm = dbm_read();
            for (i = 1; i < dim; ++i) {
                // if update is necessary
                if (cdbm.at(i, 0) != dbm_LS_INFINITY) {
                    RECORD_SUBSTAT("copy");
                    // then update from where we are
                    auto mdbm = icopy_write(dim);
                    do {
                        mdbm.at(i, 0) = dbm_LS_INFINITY;
                    } while (++i < dim);
                    break;
                }
            }
        }
    }

    void dbm_t::ptr_upStop(const uint32_t* stopped)
    {
        RECORD_STAT();
        assert(stopped);
        dbm_up_stop(getCopy(), pdim(), stopped);
    }

    void dbm_t::ptr_downStop(const uint32_t* stopped)
    {
        RECORD_STAT();
        assert(stopped);
        dbm_down_stop(getCopy(), pdim(), stopped);
    }

    void dbm_t::ptr_down()
    {
        const cindex_t dim = pdim();
        RECORD_STAT();
        if (tryMutable()) {
            RECORD_SUBSTAT("mutable");
            dbm_down(dbm(), dim);  // mutable => write directly
        } else {
            cindex_t i, j;
            auto cdbm = dbm_read();
            raw_t min;
            for (j = 1; j < dim; ++j) {
                if (cdbm.at(0, j) < dbm_LE_ZERO) {
                    min = dbm_LE_ZERO;
                    for (i = 1; i < dim; ++i) {
                        if (min > cdbm.at(i, j)) {
                            min = cdbm.at(i, j);
                        }
                    }
                    // if update necessary
                    if (min != cdbm.at(0, j)) {
                        RECORD_SUBSTAT("copy");
                        // then update from where we are!
                        auto mdbm = icopy_write(dim);  // mutable
                        mdbm.at(0, j) = min;
                        dbm_downFrom(mdbm, dim, ++j);
                        break;
                    }
                }
            }
        }
    }

    void dbm_t::ptr_freeClock(cindex_t k)
    {
        assert(k > 0 && k < getDimension());
        RECORD_STAT();
        const cindex_t dim = pdim();
        if (tryMutable()) {
            RECORD_SUBSTAT("mutable");
            dbm_freeClock(dbm(), dim, k);  // mutable => write directly
        } else {
            cindex_t i;
            auto cdbm = dbm_read();
            for (i = 0; i < dim; ++i) {
                // if update necessary
                if (i != k && (cdbm.at(k, i) != dbm_LS_INFINITY || cdbm.at(i, k) != cdbm.at(i, 0))) {
                    RECORD_SUBSTAT("copy");
                    // then update with a mutable copy
                    auto mdbm = icopy_write(dim);
                    do {
                        if (i != k) {
                            mdbm.at(k, i) = dbm_LS_INFINITY;
                            mdbm.at(i, k) = mdbm.at(i, 0);
                        }
                    } while (++i < dim);
                    assertx(dbm_isValid(mdbm, dim));
                    break;
                }
            }
        }
    }

    void dbm_t::ptr_freeDown(cindex_t k)
    {
        RECORD_STAT();
        const cindex_t dim = pdim();
        assert(k > 0 && k < dim);
        if (tryMutable()) {
            RECORD_SUBSTAT("mutable");
            dbm_freeDown(dbm(), dim, k);
        } else {
            cindex_t i;
            auto cdbm = dbm_read();
            for (i = 0; i < dim; ++i)
                if (i != k) {
                    // if update necessary
                    if (cdbm.at(i, k) != cdbm.at(i, 0)) {
                        RECORD_SUBSTAT("copy");
                        // then update with mutable copy
                        auto mdbm = icopy_write(dim);
                        do {
                            if (i != k) {
                                mdbm.at(i, k) = mdbm.at(i, 0);
                            }
                        } while (++i < dim);
                        assertx(dbm_isValid(mdbm, dim));
                        break;
                    }
                }
        }
    }

    void dbm_t::ptr_freeUp(cindex_t k)
    {
        RECORD_STAT();
        const cindex_t dim = pdim();
        assert(k > 0 && k < dim);
        if (tryMutable()) {
            RECORD_SUBSTAT("mutable");
            dbm_freeUp(dbm(), dim, k);
        } else {
            cindex_t j;
            auto cdbm = dbm_read();
            for (j = 0; j < dim; ++j)
                if (j != k) {
                    // if update necessary
                    if (cdbm.at(k, j) != dbm_LS_INFINITY) {
                        RECORD_SUBSTAT("copy");
                        // then update with mutable copy
                        auto mdbm = icopy_write(dim);
                        do {
                            if (j != k) {
                                mdbm.at(k, j) = dbm_LS_INFINITY;
                            }
                        } while (++j < dim);
                        assertx(dbm_isValid(mdbm, dim));
                        break;
                    }
                }
        }
    }

    void dbm_t::ptr_freeAllUp()
    {
        RECORD_STAT();
        const cindex_t dim = pdim();
        if (tryMutable()) {
            RECORD_SUBSTAT("mutable");
            dbm_freeAllUp(dbm(), dim);
        } else if (!dbm_isFreedAllUp(const_dbm(), dim)) {
            RECORD_SUBSTAT("copy");
            dbm_freeAllUp(icopy(dim), dim);
        }
    }

    void dbm_t::ptr_freeAllDown()
    {
        RECORD_STAT();
        const cindex_t dim = pdim();
        uint32_t ij;
        if (tryMutable()) {
            RECORD_SUBSTAT("mutable");
            dbm_freeAllDown(dbm(), dim);
        } else if ((ij = dbm_testFreeAllDown(const_dbm(), dim)) != 0) {
            RECORD_SUBSTAT("copy");
            auto mdbm = icopy_write(dim);
            cindex_t i = ij & 0xffff;
            cindex_t j = ij >> 16;
            assert(i < dim && j < dim);
            goto cont_j;  // continue loop where it stopped
            do {
                j = 1;
            cont_j:
                do {
                    if (i != j) {
                        mdbm.at(i, j) = mdbm.at(i, 0);
                    }
                } while (++j < dim);
            } while (++i < dim);
        }
    }

    void dbm_t::update(cindex_t i, cindex_t j, int32_t v)
    {
        if (!isEmpty()) {
            if (i == j) {
                if (v != 0) {
                    dbm_updateIncrement(getCopy(), pdim(), i, v);
                }
            } else if (v == 0) {
                ptr_updateClock(i, j);
            } else {
                ptr_update(i, j, v);
            }
        }
    }

    void dbm_t::ptr_updateValue(cindex_t k, int32_t v)
    {
        assert(k > 0 && k < getDimension() && v >= 0 && v < dbm_INFINITY);
        RECORD_STAT();
        const cindex_t dim = pdim();
        if (tryMutable()) {
            RECORD_SUBSTAT("mutable");
            dbm_updateValue(dbm(), dim, k, v);
        } else {
            cindex_t i = 0;
            auto cdbm = dbm_read();
            raw_t dk0 = dbm_bound2raw(v, dbm_WEAK);
            raw_t d0k = dbm_bound2raw(-v, dbm_WEAK);
            do {
                // Updates for dbm[k,i] and dbm[i,k]
                raw_t dki = dbm_addFiniteRaw(dk0, cdbm.at(0, i));
                raw_t dik = dbm_addRawFinite(cdbm.at(i, 0), d0k);

                // If updates are necessary
                if (cdbm.at(k, i) != dki || cdbm.at(i, k) != dik) {
                    RECORD_SUBSTAT("copy");
                    // then update with a mutable copy
                    auto mdbm = icopy_write(dim);
                    mdbm.at(k, i) = dki;
                    mdbm.at(i, k) = dik;
                    while (++i < dim) {
                        mdbm.at(k, i) = dbm_addFiniteRaw(dk0, mdbm.at(0, i));
                        mdbm.at(i, k) = dbm_addRawFinite(mdbm.at(i, 0), d0k);
                    }
                    assert(mdbm.at(k, k) == dbm_LE_ZERO);
                    assertx(dbm_isValid(mdbm, dim));
                    break;
                }
            } while (++i < dim);
        }
    }

    void dbm_t::ptr_updateClock(cindex_t i, cindex_t j)
    {
        assert(i != j && i > 0 && j > 0 && i < getDimension() && j < getDimension());
        RECORD_STAT();
        const cindex_t dim = pdim();
        if (tryMutable()) {
            RECORD_SUBSTAT("mutable");
            dbm_updateClock(dbm(), dim, i, j);
        } else {
            cindex_t k;
            auto cdbm = dbm_read();
            for (k = 0; k < dim; ++k) {
                // if update necessary
                if (i != k && (cdbm.at(i, k) != cdbm.at(j, k) || cdbm.at(k, i) != cdbm.at(k, j))) {
                    RECORD_SUBSTAT("copy");
                    // then update with mutable copy
                    auto mdbm = icopy_write(dim);
                    do {
                        if (i != k) {
                            mdbm.at(i, k) = mdbm.at(j, k);
                            mdbm.at(k, i) = mdbm.at(k, j);
                        }
                    } while (++k < dim);
                    assertx(dbm_isValid(mdbm, dim));
                    break;
                }
            }
        }
    }

    void dbm_t::ptr_update(cindex_t i, cindex_t j, int32_t v)
    {
        assert(i != j && v != 0 && i > 0 && i < getDimension() && j > 0 && j < getDimension());
        RECORD_STAT();
        const cindex_t dim = pdim();
        if (tryMutable()) {
            RECORD_SUBSTAT("mutable");
            dbm_update(dbm(), dim, i, j, v);
        } else {
            cindex_t k;
            auto cdbm = dbm_read();
            for (v <<= 1, k = 0; k < dim; ++k)
                if (i != k) {
                    // updates for dbm[k,i] and dbm[i,k]
                    raw_t dik = dbm_rawInc(cdbm.at(j, k), v);
                    raw_t dki = dbm_rawDec(cdbm.at(k, j), v);

                    // if updates are necessary
                    if (cdbm.at(i, k) != dik || cdbm.at(k, i) != dki) {
                        RECORD_SUBSTAT("copy");
                        // then update with mutable copy
                        auto mdbm = icopy_write(dim);
                        mdbm.at(i, k) = dik;
                        mdbm.at(k, i) = dki;
                        while (++k < dim)
                            if (i != k) {
                                mdbm.at(i, k) = dbm_rawInc(mdbm.at(j, k), v);
                                mdbm.at(k, i) = dbm_rawDec(mdbm.at(k, j), v);
                            }
                        assertx(dbm_isValid(mdbm, dim));
                        break;
                    }
                }
        }
    }

    void dbm_t::ptr_tightenDown()
    {
        assert(!isEmpty());

        const cindex_t dim = pdim();
        raw_t* dbm = getCopy();

        if (!dbm_tightenDown(dbm, dim)) {
            emptyMutable(dim);
        }
    }

    void dbm_t::ptr_tightenUp()
    {
        assert(!isEmpty());

        const cindex_t dim = pdim();
        raw_t* dbm = getCopy();

        if (!dbm_tightenUp(dbm, dim)) {
            emptyMutable(dim);
        }
    }

    void dbm_t::ptr_relaxDownClock(cindex_t clock)
    {
        RECORD_STAT();
        const cindex_t dim = pdim();
        if (tryMutable()) {
            RECORD_SUBSTAT("mutable");
            dbm_relaxDownClock(dbm(), dim, clock);
        } else {
            cindex_t i, j;
            auto dbm = writer{idbmt()->getMatrix(), dim};
            bool isMutable = false;
            // see dbm.c
            for (i = 0; i < dim; ++i) {
                if (dbm.at(i, clock) < dbm_LS_INFINITY && dbm.is_strict(i, clock)) {
                    raw_t wik = dbm_weakRaw(dbm.at(i, clock));
                    for (j = 0; j < dim; ++j)
                        if (j != clock)  // dbm[i,clock] not weakened!
                        {
                            raw_t cik;
                            if (dbm.at(i, j) < dbm_LS_INFINITY && dbm.at(j, clock) < dbm_LS_INFINITY &&
                                (cik = dbm_addFiniteWeak(dbm.at(i, j), dbm_weakRaw(dbm.at(j, clock)))) < wik) {
                                assert(cik == dbm.at(i, clock));  // closed
                                // cannot weaken this constraint because it is tightened by a
                                // diagonal
                                break;
                            }
                        }
                    if (!(j < dim))  // weak constraint accepted!
                    {
                        // we need to write!
                        if (!isMutable) {
                            RECORD_SUBSTAT("copy");
                            dbm = icopy_write(dim);
                            isMutable = true;
                        }
                        dbm.at(i, clock) = wik;
                    }
                }
            }
        }
    }

    // Symmetric to ptr_relaxDownClock
    void dbm_t::ptr_relaxUpClock(cindex_t clock)
    {
        RECORD_STAT();
        const cindex_t dim = pdim();
        if (tryMutable()) {
            RECORD_SUBSTAT("mutable");
            dbm_relaxUpClock(dbm(), dim, clock);
        } else {
            cindex_t i, j;
            auto dbm = writer{idbmt()->getMatrix(), dim};  // bypass assertions because it's not mutable yet
            bool isMutable = false;
            // see dbm.c
            for (i = 0; i < dim; ++i) {
                if (dbm.at(clock, i) < dbm_LS_INFINITY && dbm.is_strict(clock, i)) {
                    raw_t wki = dbm_weakRaw(dbm.at(clock, i));
                    for (j = 0; j < dim; ++j)
                        if (j != clock)  // dbm[i,clock] not weakened!
                        {
                            raw_t cki;
                            if (dbm.at(j, i) < dbm_LS_INFINITY && dbm.at(clock, j) < dbm_LS_INFINITY &&
                                (cki = dbm_addFiniteWeak(dbm.at(j, i), dbm_weakRaw(dbm.at(clock, j)))) < wki) {
                                assert(cki == dbm.at(clock, i));  // closed
                                // cannot weaken this constraint because it is tightened by a
                                // diagonal
                                break;
                            }
                        }
                    if (!(j < dim))  // weak constraint accepted!
                    {
                        // need to write!
                        if (!isMutable) {
                            RECORD_SUBSTAT("copy");
                            dbm = icopy_write(dim);
                            isMutable = true;
                        }
                        dbm.at(clock, i) = wki;
                    }
                }
            }
        }
    }

    // Make all constraints non strict (except infinity)
    void dbm_t::ptr_relaxAll()
    {
        RECORD_STAT();
        const cindex_t dim = pdim();
        if (tryMutable()) {
            RECORD_SUBSTAT("mutable");
            dbm_relaxAll(dbm(), dim);
        } else {
            // +1, -1: ignore 1st and last elements of the diagonal
            for (const raw_t *start = const_dbm(), *cdbm = start + 1, *end = start + dim * dim - 1; cdbm < end;
                 ++cdbm) {
                if (dbm_rawIsStrict(*cdbm) && *cdbm != dbm_LS_INFINITY) {
                    RECORD_SUBSTAT("copy");
                    // need mutable!
                    auto mdbm = icopy_write(dim);
                    end = mdbm + (end - start);
                    mdbm += (cdbm - start);
                    *mdbm = dbm_weakRaw(*mdbm);
                    while (++mdbm < end) {
                        if (*mdbm != dbm_LS_INFINITY) {
                            *mdbm = dbm_weakRaw(*mdbm);
                        }
                    }
                    break;
                }
            }
        }
    }

    bool dbm_t::contains(const int32_t* point, cindex_t dim) const
    {
        assert(point && getDimension() == dim);
        return !isEmpty() && dbm_isPointIncluded(point, const_dbm(), dim);
    }

    bool dbm_t::contains(const double* point, cindex_t dim) const
    {
        assert(point && dim == getDimension());
        return !isEmpty() && dbm_isRealPointIncluded(point, const_dbm(), dim);
    }

    bool dbm_t::getMinDelay(const double* point, cindex_t dim, double* t, double* minVal, bool* minStrict,
                            const uint32_t* stopped) const
    {
        assert(dim == getDimension() && point && t);

        if (isEmpty()) {
            *t = HUGE_VAL;
            if (minVal != nullptr && minStrict != nullptr) {
                *minVal = HUGE_VAL;
                *minStrict = true;
            }
            return false;
        }
        auto dbm = dbm_read();

        // Special case, covers dim == 1.
        if (dbm_isRealPointIncluded(point, dbm, dim)) {
            *t = 0.0;
            if (minVal != nullptr && minStrict != nullptr) {
                *minVal = 0.0;
                *minStrict = false;
            }
            return true;
        }

        *t = HUGE_VAL;

        for (cindex_t k = 1; k < dim; ++k) {
            // Try to reach up to this lower bound.
            double di = (point[0] - point[k]) - (double)dbm.bound(0, k);

            if (di >= 0.0)  // Consider only future.
            {
                double value = di;
                bool isStrict = dbm.is_strict(0, k);

                // Need to get into the DBM if the bound is strict.
                if (isStrict) {
                    di = base_addEpsilon(di, base_EPSILON);
                }

                // Consider only better delays.
                if (di < *t) {
                    // Copy point and apply delay, respecting stopped clocks.
                    std::vector<double> pt(dim);
                    double dt = isStrict ? base_addEpsilon(value, 1e-6 * base_EPSILON) : value;
                    pt[0] = point[0];
                    for (cindex_t i = 1; i < dim; ++i) {
                        pt[i] = stopped != nullptr && base_readOneBit(stopped, i) != 0 ? point[i] : point[i] + dt;
                    }

                    // Check if point + di would be inside.
                    // Decrease ref <=> + other clocks.
                    // The point maybe choosen such that base_EPSILON is big enough
                    // to be correct but value (strict) may be incorrect, which is
                    // why we have two checks here.
                    // Validate value.
                    if (dbm_isRealPointIncluded(pt.data(), dbm, dim)) {
                        // We know that value+small epsilon if fine
                        // so we must be able to find di+large epsilon.
                        double addEpsilon = base_EPSILON;
                        do {
                            // And validate di.
                            for (cindex_t i = 1; i < dim; ++i) {
                                pt[i] =
                                    stopped != nullptr && base_readOneBit(stopped, i) != 0 ? point[i] : point[i] + di;
                            }
                            if (dbm_isRealPointIncluded(pt.data(), dbm, dim)) {
                                *t = di;
                                if (minVal != nullptr && minStrict != nullptr) {
                                    *minVal = value;
                                    *minStrict = isStrict;
                                }
                                break;
                            }
                            // else possible only if isStrict
                            addEpsilon *= 0.1;
                            di = base_addEpsilon(di, addEpsilon);
                        } while (!isStrict && addEpsilon >= 1e-6 * base_EPSILON);
                    }
                }
            }
        }

        return (*t < HUGE_VAL);
    }

    bool dbm_t::getMaxDelay(const double* point, cindex_t dim, double* t, double* minVal, bool* minStrict,
                            const uint32_t* stopped) const
    {
        assert(dim == getDimension() && point && t);

        *t = HUGE_VAL;
        if (minVal != nullptr && minStrict != nullptr) {
            *minVal = HUGE_VAL;
            *minStrict = true;
        }

        if (isEmpty()) {
            return false;
        }
        auto dbm = dbm_read();
        for (cindex_t k = 1; k < dim; ++k) {
            if (dbm.at(k, 0) != dbm_LS_INFINITY && !(stopped != nullptr && base_readOneBit(stopped, k) != 0)) {
                bool isStrict = dbm.is_strict(k, 0);
                auto bound = (double)dbm.bound(k, 0);
                double d = bound - (point[k] - point[0]);
                if (d < 0.0) {
                    return false;
                }
                double val = d;
                if (isStrict) {
                    d = base_subtractEpsilon(d, base_EPSILON);
                }
                if (d < *t) {
                    *t = d;
                    if (minVal != nullptr && minStrict != nullptr) {
                        *minVal = val;
                        *minStrict = isStrict;
                    }
                }
            }
        }
        return true;
    }

    bool dbm_t::getMaxBackDelay(const double* point, cindex_t dim, double* t, double max) const
    {
        assert(dim == getDimension() && point && t);

        if (isEmpty()) {
            *t = HUGE_VAL;
            return false;
        }
        // Special case: only ref clock.
        if (dim == 1) {
            *t = max;
            return true;
        }
        auto dbm = dbm_read();

        *t = 0.0;
        auto* pt = (double*)point;  // Cheat.
        double pt0 = point[0];      // Save; Should be 0.0.

        for (cindex_t k = 1; k < dim; ++k) {
            // Try to reach down to this lower bound.
            double di = (double)dbm.bound(0, k) - point[0] + point[k];

            if (di >= 0.0) {  // Consider only past.
                // Need to stay inside the DBM if the bound is strict.
                if (dbm.is_strict(0, k)) {
                    di = base_subtractEpsilon(di, base_EPSILON);
                }

                // Consider only better delays.
                if (di > *t) {
                    // Check if point + di would be inside.
                    pt[0] = pt0 + di;  // + ref <=> - other clock values.
                    if (dbm_isRealPointIncluded(pt, dbm, dim)) {
                        *t = di;
                    }
                    pt[0] = pt0;  // Restore.
                }
            }
        }

        if (*t > max) {
            *t = max;
        }
        return (*t > 0.0);
    }

    bool dbm_t::isConstrainedBy(cindex_t i, cindex_t j, raw_t c) const
    {
        // Don't think about constraining the diagonal with something inconsistent.
        assert(i != j || c == dbm_LE_ZERO);
        // The current DBM must be consistent.
        assert(i != j || const_dbm()[i * pdim() + j] == dbm_LE_ZERO);

        return !isEmpty() && c < const_dbm()[i * pdim() + j];
    }

    void dbm_t::resize(const uint32_t* bitSrc, const uint32_t* bitDst, size_t bitSize, cindex_t* table)
    {
        assert(idbmPtr != nullptr && bitSrc && bitDst && table);
        assert(*bitSrc & *bitDst & 1);  // ref clock in both

        if (!base_areBitsEqual(bitSrc, bitDst, bitSize)) {  // pre-condition
            cindex_t newDim = base_countBitsN(bitDst, bitSize);
            assert(newDim);
            if (isEmpty()) {
                base_bits2indexTable(bitDst, bitSize, table);
                setEmpty(newDim);
            } else if (newDim <= 1) {
#ifndef ENABLE_DBM_NEW
                newCopy(DBMAllocator::instance().dbm1());
#else
                newCopy(dbm::dbm1());
#endif
                table[0] = 0;
            } else {
                idbm_t* old = idbmt();
                dbm_shrinkExpand(old->const_dbm(), setNew(newDim), old->getDimension(), bitSrc, bitDst, bitSize, table);
                old->decRef();  // free old DBM
            }
        }
    }

    void dbm_t::changeClocks(const cindex_t* cols, cindex_t newDim)
    {
        if (isEmpty()) {
            setEmpty(newDim);
        } else if (newDim <= 1) {
#ifndef ENABLE_DBM_NEW
            newCopy(DBMAllocator::instance().dbm1());
#else
            newCopy(dbm::dbm1());
#endif
        } else {
            idbm_t* old = idbmt();
            dbm_updateDBM(setNew(newDim), old->const_dbm(), newDim, old->getDimension(), cols);
            old->decRef();  // free old DBM
        }
    }

    // Swapping x & y has an effect iff x != y.
    // We can check easily if x == y to skip the swap.
    void dbm_t::ptr_swapClocks(cindex_t x, cindex_t y)
    {
        const cindex_t dim = pdim();
        auto dbm = writer{idbmt()->getMatrix(), dim};
        if (dbm.at(x, y) != dbm_LE_ZERO || dbm.at(y, x) != dbm_LE_ZERO) {
            dbm_swapClocks(tryMutable() ? dbm : icopy(dim), dim, x, y);
        }
    }

    void dbm_t::getValuation(double* cval, cindex_t dimen, bool* freeC) const
    {
        assert(dimen == getDimension());

        if (isEmpty()) {
            throw std::out_of_range("No clock valuation for empty DBMs");
        }

        const cindex_t dim = pdim();
        bool* freeClocks = (bool*)calloc(dim, sizeof(bool));

        if (freeC != nullptr) {
            std::fill(freeClocks, freeClocks + dim, 1);
            freeC = freeClocks;
        }
        if (!ptr_getValuation(cval, dimen, freeC)) {
            throw std::out_of_range("Clock valuation");
        }
    }

    // private: implementation of getValuation
    bool dbm_t::ptr_getValuation(double* cval, cindex_t dimen, bool* freeC) const
    {
        assert(freeC);

        // We iteratively compute the value of each free clock.
        const cindex_t dim = pdim();
        auto cdbm = dbm_read();

        cval[0] = 0.0;
        for (cindex_t i = 1; i < dim; ++i) {
            if (freeC[i]) {
                cval[i] = 0.0;
            }
        }

        for (cindex_t i = 1; i < dim; ++i) {
            if (freeC[i]) {
                // Derive lower and upper bounds for this clock.
                bool lStrict = cdbm.is_strict(0, i);
                double lower = -cdbm.bound(0, i);
                bool uStrict = cdbm.is_strict(i, 0);
                double upper = cdbm.bound(i, 0);
                double bound;

                for (cindex_t j = 1; j < dim; j++) {
                    if (freeC[j]) {
                        continue;
                    }

                    bound = cdbm.bound(i, j) + cval[j];
                    if (bound < upper || (bound <= upper && !uStrict)) {
                        upper = bound;
                        uStrict = cdbm.is_strict(i, j);
                    }

                    bound = cval[j] - cdbm.bound(j, i);
                    if (bound > lower || (bound >= lower && !lStrict)) {
                        lower = bound;
                        lStrict = cdbm.is_strict(j, i);
                    }
                }

                // Find a good value within the range.
                if (floor(lower) == lower && !lStrict) {
                    cval[i] = lower;
                } else if ((floor(lower + 1) <= upper && !uStrict) || floor(lower + 1) < upper) {
                    cval[i] = floor(lower + 1);
                } else if (!uStrict) {
                    cval[i] = upper;
                } else {
                    cval[i] = (lower + upper) / 2.0;
                }
                freeC[i] = false;
            }
        }

        // check the generated point
        return contains(cval, dimen);
    }

    // this dbm - fed
    bool dbm_t::isSubtractionEmpty(const fed_t& fed) const
    {
        assert(getDimension() == fed.getDimension());

        // trivial: empty - anything = empty
        if (isEmpty()) {
            return true;
        }
        // trivial: non empty - empty = non empty
        else if (fed.isEmpty()) {
            return false;
        }
#ifndef DISABLE_RELATION_BEFORE_SUBTRACTION
        // full relation test
        else if (*this <= fed) {
            return true;
        }
#else
        // limited relation test: if this DBM <= fed's 1st DBM
        else if (dbm_isSubsetEq(const_dbm(), fed.begin()->const_dbm(), pdim())) {
            return true;
        }
#endif
        // trivial: one DBM only -> we know the final result
        else if (fed.size() == 1) {
            return false;
        } else  // not simple
        {
            return (fed_t(*this) -= fed).isEmpty();
        }
    }
}  // namespace dbm
