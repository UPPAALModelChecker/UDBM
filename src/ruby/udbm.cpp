/* -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-
 *
 * This file is part of the Ruby binding to the UPPAAL DBM library.
 *
 * This binding is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * This binding is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this binding; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA.
 */
// DBM plugin for ruby.
// $Id: udbm.cpp,v 1.12 2005/11/05 21:43:04 adavid Exp $

#include "ruby.h"

#include "dbm/fed.h"
#include "dbm/gen.h"
#include "dbm/mingraph.h"
#include "base/bitstring.h"
#include "hash/compute.h"
#include "hash/tables.h"

#include <string>
#include <cmath>

// Option to have the lines DBMs->axis short (when drawing them)

#define SHORT_LINES

// Don't bother with prefixing.

using namespace std;
using namespace dbm;

// Versions.

static VALUE RUBY_UDBM_VERSION;
static VALUE UDBM_VERSION;

// IDs to access names

static ID NEW_ID;
static ID SIZE_ID;
static ID TO_A_ID;
static ID BOUND_ID;
static ID STRICT_QID;

// Constants

static const char rDifferent[] = "Different";
static const char rSuperset[] = "Superset";
static const char rSubset[] = "Subset";
static const char rEqual[] = "Equal";

// Classes

static VALUE mUDBM;        // module UDBM
static VALUE cFed;         // class Fed
static VALUE cMatrix;      // class Matrix
static VALUE cConstraint;  // class Constraint
static VALUE cRelation;    // class Relation

// Useful macros

#define FED(VAL, NAME) \
    fed_t* NAME;       \
    Data_Get_Struct(VAL, fed_t, NAME)
#define FED2VALUE(FED)       Data_Wrap_Struct(cFed, NULL, delete_fed, FED)
#define RELATION2VALUE(R)    Data_Wrap_Struct(cRelation, NULL, NULL, (void*)((R) << 4))
#define CONSTRAINT2BOUND(C)  NUM2INT(rb_funcall(C, BOUND_ID, 0))
#define CONSTRAINT2STRICT(C) value2bool(rb_funcall(C, STRICT_QID, 0))

#define CONST_OP(OP)                 \
    FED(self, fed);                  \
    fed_t* result = new fed_t(*fed); \
    result->OP;                      \
    return FED2VALUE(result)

#define SELF_OP(OP) \
    FED(self, fed); \
    fed->OP;        \
    return self

#define CONST_DO(WHAT)  \
    FED(self, fed);     \
    fed_t result(*fed); \
    WHAT;               \
    return FED2VALUE(new fed_t(result))

#define SELF_DO(WHAT) \
    FED(self, fed);   \
    WHAT;             \
    return self

#define CONST_OPX(OP)                \
    fed_t* result = new fed_t(*fed); \
    result->OP;                      \
    return FED2VALUE(result)

#define SELF_OPX(OP) \
    fed->OP;         \
    return self

#define FED_OP(OP)                          \
    if (check_Fed_Matrix(arg)) {            \
        FED(arg, fed2);                     \
        check_dim(fed, fed2);               \
        OP(*fed2);                          \
    } else {                                \
        cindex_t dim = fed->getDimension(); \
        check_size(call_size(arg), dim);    \
        OP(matrix2dbm(arg, dim));           \
    }

#define FED_REL_OP(OP1, OP2)                \
    if (check_Fed_Matrix(arg)) {            \
        FED(arg, fed2);                     \
        check_dim(fed, fed2);               \
        if (relationCache) {                \
            OP1(*fed2);                     \
        } else {                            \
            OP2(*fed2);                     \
        }                                   \
    } else {                                \
        cindex_t dim = fed->getDimension(); \
        check_size(call_size(arg), dim);    \
        OP2(matrix2dbm(arg, dim));          \
    }

#define TEST_OP(OP)         \
    FED(self, fed);         \
    bool test;              \
    FED_OP(test = fed->OP); \
    return test ? Qtrue : Qfalse

#define TEST_REL_OP(OP)                                                \
    FED(self, fed);                                                    \
    bool test;                                                         \
    FED_REL_OP(test = relationCache->caller(fed)->OP, test = fed->OP); \
    return test ? Qtrue : Qfalse

// Relation cache: Remember relations between
// federations + intern() these federations.

static dbm_t* fed2array(fed_t&, size_t*);

class RelationCache
{
public:
    RelationCache(): fed(NULL) {}

    // The API is designed to by-pass fed_t but
    // caches relations only between federations.
    RelationCache* caller(fed_t* f)
    {
        fed = f;
        return this;
    }
    bool lt(fed_t&);
    bool gt(fed_t&);
    bool le(fed_t&);
    bool ge(fed_t&);
    bool eq(fed_t&);
    uintptr_t exactRelation(fed_t&);

private:
    enum info_t { LE = 1 << 0, GE = 1 << 1, RELATION = LE | GE };

    fed_t* fed;  // Valid only during ptr->caller(fed)->method(arg).

    struct fed_pair_t
    {
        size_t size1, size2;
        dbm_t *fed1, *fed2;
    };

    struct entry_t : public uhash::TableSingle<entry_t>::Bucket_t
    {
        fed_pair_t feds;
        uint32_t type;
        uintptr_t rel;
    };

    uhash::TableSingle<entry_t> table;

    entry_t* get_entry(fed_t&, fed_t&);
    static void ensure_le(const fed_t&, const fed_t&, entry_t*);
    static void ensure_ge(const fed_t&, const fed_t&, entry_t*);
};

static RelationCache* relationCache = NULL;

// Raise meaningful exceptions
// Note: we avoid calling 'new' as long as exceptions
// may be raised to avoid leaks. We allocate safely on
// the stack in these cases.

static void check_Matrix(VALUE val)
{
    if (!rb_obj_is_kind_of(val, cMatrix)) {
        rb_raise(rb_eTypeError, "UDBM::Matrix expected");
    }
}

static void check_size(int size, cindex_t dim)
{
    if (dim * dim != (cindex_t)size) {
        rb_raise(rb_eArgError, "not a square matrix (%d instead of %u*%u)", size, dim, dim);
    }
}

static void check_Constraint(VALUE val)
{
    if (!rb_obj_is_kind_of(val, cConstraint)) {
        rb_raise(rb_eTypeError, "UDBM::Constraint expected");
    }
}

static cindex_t check_clock(VALUE val, cindex_t dim)
{
    int clk = NUM2INT(val);
    if (clk == 0) {
        rb_raise(rb_eRangeError, "reference clock (0) forbidden");
    }
    if (clk < 0) {
        rb_raise(rb_eRangeError, "clock must be positive");
    }
    if (clk >= (int)dim) {
        rb_raise(rb_eRangeError, "clock (%d) must be < dimension (%u)", clk, dim);
    }
    return clk;
}

static cindex_t check_index(VALUE val, cindex_t dim)
{
    int index = NUM2INT(val);
    if (index < 0) {
        rb_raise(rb_eRangeError, "index must be positive");
    }
    if (index >= (int)dim) {
        rb_raise(rb_eRangeError, "index (%d) must be < dimension (%u)", index, dim);
    }
    return index;
}

static void check_dim(int dim1, int dim2)
{
    if (dim1 != dim2) {
        rb_raise(rb_eArgError, "incompatible dimensions (%d and %d)", dim1, dim2);
    }
}

static void check_dim(const fed_t* f1, const fed_t* f2) { check_dim(f1->getDimension(), f2->getDimension()); }

static cindex_t check_dim(VALUE val)
{
    int dim = NUM2INT(val);
    if (dim < 1) {
        rb_raise(rb_eRangeError, "minimal dimension is 1");
    }
    if (dim > 0xffff) {
        rb_raise(rb_eRangeError, "maximal dimension is %u", 0xffff);
    }
    return (cindex_t)dim;
}

static bool value2bool(VALUE val)
{
    if (val == Qtrue) {
        return true;
    } else if (val != Qfalse) {
        rb_raise(rb_eTypeError, "bool expected");
    }
    return false;
}

static uintptr_t value2relation(VALUE val)
{
    void* dummy;
    Data_Get_Struct(val, void, dummy);
    return uintptr_t(dummy) >> 4;
}

static uintptr_t check_relation(VALUE val)
{
    if (rb_obj_is_kind_of(val, cRelation)) {
        return value2relation(val);
    }
    int rel = NUM2INT(val);
    if (rel < 0 || rel > base_EQUAL) {
        rb_raise(rb_eRangeError, "integer must be between 0 and 3");
    }
    return (uintptr_t)rel;
}

static VALUE call_toa(VALUE val)
{
    VALUE result = rb_funcall(val, TO_A_ID, 0);
    if (TYPE(result) != T_ARRAY) {
        rb_raise(rb_eTypeError, "Array expected");
    }
    return result;
}

static int call_size(VALUE val)
{
    int size = NUM2INT(rb_funcall(val, SIZE_ID, 0));
    if (size <= 0) {
        rb_raise(rb_eArgError, "minimum size is 1");
    }
    return size;
}

static void check_argc(int argc, int nb)
{
    if (argc < nb) {
        rb_raise(rb_eArgError, "wrong number of arguments (%d instead of %d+)", argc, nb);
    }
}

static bool check_Fed_Matrix(VALUE arg)
{
    if (rb_obj_is_kind_of(arg, cFed)) {
        return true;
    }
    if (!rb_obj_is_kind_of(arg, cMatrix)) {
        rb_raise(rb_eTypeError, "UDBM::Matrix or UDBM::Fed expected");
    }
    return false;
}

static void value2intarray(VALUE arg, int* vec, int size)
{
    for (int i = 0; i < size; ++i) {
        vec[i] = NUM2INT(rb_ary_entry(arg, i));
    }
    if (vec[0] != 0) {
        rb_raise(rb_eArgError, "first element must be zero");
    }
}

static void value2doublearray(VALUE arg, double* vec, int size)
{
    for (int i = 0; i < size; ++i) {
        vec[i] = NUM2DBL(rb_ary_entry(arg, i));
    }
    if (fabs(vec[0]) > 1e-100) {
        rb_raise(rb_eArgError, "first element must be zero");
    }
}

static void check_intarray(VALUE arg, int* vec, cindex_t dim)
{
    if (TYPE(arg) != T_ARRAY) {
        rb_raise(rb_eTypeError, "Array of Fixnum expected");
    }
    check_dim(call_size(arg), dim);
    value2intarray(arg, vec, dim);
}

// Useful functions

static dbm_t matrix2dbm(VALUE matrix, cindex_t dim)
{
    VALUE arr = call_toa(matrix);
    dbm_t tmp(dim);
    raw_t* dbm = tmp.getDBM();
    int size = dim * dim;
    // read constraints
    for (int i = 0; i < size; ++i) {
        VALUE c = rb_ary_entry(arr, i);
        check_Constraint(c);
        dbm[i] = dbm_boundbool2raw(CONSTRAINT2BOUND(c), CONSTRAINT2STRICT(c));
    }
    // fix matrix
    for (cindex_t i = 0; i < dim; ++i) {
        // clocks positive, thank you
        if (dbm[i] > dbm_LE_ZERO) {
            dbm[i] = dbm_LE_ZERO;
        }
        // fix diagonal
        if (dbm[i * dim + i] > dbm_LE_ZERO) {
            dbm[i * dim + i] = dbm_LE_ZERO;
        }
    }
    if (!dbm_close(dbm, dim)) {
        tmp.setEmpty();
    }
    return tmp;
}

static void add_matrix(fed_t* fed, cindex_t dim, VALUE matrix)
{
    check_Matrix(matrix);
    check_size(call_size(matrix), dim);
    *fed |= matrix2dbm(matrix, dim);
}

static void delete_fed(void* fed) { delete (fed_t*)fed; }

// Methods of UDBM::Relation

static VALUE cRelation_new(VALUE classValue, VALUE arg) { return RELATION2VALUE(check_relation(arg)); }

static VALUE cRelation_toi(VALUE self) { return INT2FIX(value2relation(self)); }

static VALUE cRelation_tos(VALUE self)
{
    const char* s = NULL;
    switch (value2relation(self)) {
    case base_DIFFERENT: s = rDifferent; break;
    case base_SUPERSET: s = rSuperset; break;
    case base_SUBSET: s = rSubset; break;
    case base_EQUAL: s = rEqual;
    }
    return rb_str_new2(s);
}

static VALUE cRelation_and(VALUE self, VALUE arg) { return RELATION2VALUE(value2relation(self) & check_relation(arg)); }

static VALUE cRelation_weaker(VALUE self, VALUE arg)
{
    uintptr_t a = value2relation(self);
    uintptr_t b = check_relation(arg);
    uintptr_t c = a & b;
    return (c != 0 && c == a) || (a | b) == 0 ? Qtrue : Qfalse;
}

// Methods of UDBM::Fed

static VALUE cFed_new(int argc, VALUE* argv, VALUE classValue)
{
    fed_t* fed = new fed_t();
    VALUE instance = Data_Wrap_Struct(classValue, NULL, delete_fed, fed);
    rb_obj_call_init(instance, argc, argv);
    return instance;
}

static VALUE cFed_initialize(int argc, VALUE* argv, VALUE self)
{
    FED(self, fed);
    if (argc == 1 && rb_obj_is_kind_of(argv[0], cFed)) {
        FED(argv[0], arg);
        *fed = *arg;
    } else {
        check_argc(argc, 1);
        cindex_t dim;
        if (rb_obj_is_kind_of(argv[0], cMatrix)) {
            // infer dimension and start with one dbm_t
            int size = call_size(argv[0]);
            dim = (cindex_t)sqrt(size);
            check_size(size, dim);
            *fed |= matrix2dbm(argv[0], dim);
        } else {
            // read dimension & init fed_t
            dim = check_dim(argv[0]);
            fed->setDimension(dim);
        }
        // read remaining arguments
        for (int i = 1; i < argc; ++i) {
            if (TYPE(argv[i]) == T_ARRAY) {
                int size = NUM2INT(rb_funcall(argv[i], SIZE_ID, 0));
                for (int j = 0; j < size; ++j) {
                    add_matrix(fed, dim, rb_ary_entry(argv[i], j));
                }
            } else if (argv[i] != Qnil) {
                add_matrix(fed, dim, argv[i]);
            }
        }
    }
    return self;
}

static VALUE cFed_tos(VALUE self)  // to string
{
    FED(self, fed);
    char number[24];
    string str("Fed(");
    cindex_t dim = fed->getDimension();
    snprintf(number, sizeof(number), "%u", dim);
    str += number;
    str += fed->size() > 1 ? ") {[\n" : ") {";
    bool isFirst = true;
    for (fed_t::const_iterator dbm = fed->begin(); !dbm.null(); ++dbm) {
        const raw_t* m = dbm();
        if (m) {
            snprintf(number, sizeof(number), "%cmatrix\\\n", isFirst ? ' ' : ',');
            isFirst = false;
            str += number;
            for (cindex_t i = 0; i < dim; ++i) {
                for (cindex_t j = 0; j < dim; ++j) {
                    raw_t c = m[i * dim + j];
                    if (c == dbm_LS_INFINITY) {
                        str += "\t<INF";
                    } else {
                        snprintf(number, sizeof(number), "\t%s%d", dbm_rawIsStrict(c) ? "<" : "<=", dbm_raw2bound(c));
                        str += number;
                    }
                }
                str += " \\\n";
            }
        }
    }
    str += fed->size() > 1 ? "]}" : "}";
    return rb_str_new2(str.c_str());
}

static VALUE cFed_toa(VALUE self)  // to array
{
    FED(self, fed);
    VALUE result = rb_ary_new();
    cindex_t matSize = fed->getDimension();
    matSize *= matSize;
    for (fed_t::const_iterator iter = fed->begin(); !iter.null(); ++iter) {
        const raw_t* dbm = iter();
        if (dbm) {
            VALUE arr = rb_ary_new2(matSize);
            for (size_t i = 0; i < matSize; ++i) {
                int bound = dbm_raw2bound(dbm[i]);
                bool isStrict = dbm_rawIsStrict(dbm[i]);
                rb_ary_store(arr, i, rb_funcall(cConstraint, NEW_ID, 2, INT2FIX(bound), isStrict ? Qtrue : Qfalse));
            }
            rb_ary_push(result, rb_funcall(cMatrix, NEW_ID, 1, arr));
        }
    }
    return result;
}

static VALUE cFed_zero(VALUE self, VALUE arg)
{
    cindex_t dim = check_dim(arg);
    fed_t* result = new fed_t(dim);
    result->setZero();
    return FED2VALUE(result);
}

static VALUE cFed_init(VALUE self, VALUE arg)
{
    cindex_t dim = check_dim(arg);
    fed_t* result = new fed_t(dim);
    result->setInit();
    return FED2VALUE(result);
}

static VALUE cFed_random(VALUE self, VALUE arg)
{
    cindex_t dim = check_dim(arg);
    int n = 1 + rand() % 5;
    fed_t* result = new fed_t(dim);
    while (n) {
        dbm_t tmp(dim);
        dbm_generate(tmp.getDBM(), dim, 100);
        *result |= tmp;
        --n;
    }
    return FED2VALUE(result);
}

static VALUE cFed_setRelationCache(VALUE self, VALUE arg)
{
    // Reset the cache.
    delete relationCache;
    relationCache = NULL;

    if (arg == Qtrue) {
        relationCache = new RelationCache;
    }
    return self;
}

static VALUE cFed_size(VALUE self)
{
    FED(self, fed);
    return INT2FIX(fed->size());
}

static VALUE cFed_dim(VALUE self)
{
    FED(self, fed);
    return INT2FIX(fed->getDimension());
}

static VALUE cFed_setDimension(VALUE self, VALUE arg) { SELF_OP(setDimension(check_dim(arg))); }

static VALUE cFed_copy(VALUE self) { return cFed_new(1, &self, cFed); }

static VALUE cFed_isEmpty(VALUE self)
{
    FED(self, fed);
    return fed->isEmpty() ? Qtrue : Qfalse;
}

static VALUE cFed_isUnbounded(VALUE self)
{
    FED(self, fed);
    return fed->isUnbounded() ? Qtrue : Qfalse;
}

static VALUE cFed_relation(VALUE self, VALUE arg)
{
    FED(self, fed);
    uintptr_t rel;
    FED_REL_OP(rel = relationCache->caller(fed)->exactRelation, rel = fed->exactRelation);
    return RELATION2VALUE(rel);
}

static VALUE cFed_constrainClock(VALUE self, VALUE clk, VALUE val)
{
    FED(self, fed);
    cindex_t clock = check_clock(clk, fed->getDimension());
    int value = NUM2INT(val);
    if (value < 0) {
        rb_raise(rb_eRangeError, "clock cannot be negative");
    }
    fed->constrain(clock, value);
    return self;
}

static VALUE cFed_constrain(int argc, VALUE* argv, VALUE self)
{
    check_argc(argc, 3);
    FED(self, fed);
    cindex_t dim = fed->getDimension();
    cindex_t i = check_index(argv[0], dim);
    cindex_t j = check_index(argv[1], dim);
    int bound;
    bool strict;
    if (rb_obj_is_kind_of(argv[2], cConstraint)) {
        bound = CONSTRAINT2BOUND(argv[2]);
        strict = CONSTRAINT2STRICT(argv[2]);
    } else if (argc != 4) {
        rb_raise(rb_eArgError, "wrong number of arguments (%d instead of 4)", argc);
        // Avoid warnings
        bound = dbm_INFINITY;
        strict = true;
    } else {
        bound = NUM2INT(argv[2]);
        strict = value2bool(argv[3]);
    }
    fed->constrain(i, j, bound, strict);
    return self;
}

static VALUE cFed_const_updateValue(VALUE self, VALUE x, VALUE val)
{
    FED(self, fed);
    cindex_t clock = check_clock(x, fed->getDimension());
    int value = NUM2INT(val);
    CONST_OPX(updateValue(clock, value));
}

static VALUE cFed_self_updateValue(VALUE self, VALUE x, VALUE val)
{
    FED(self, fed);
    cindex_t clock = check_clock(x, fed->getDimension());
    int value = NUM2INT(val);
    SELF_OPX(updateValue(clock, value));
}

static VALUE cFed_const_updateClock(VALUE self, VALUE x, VALUE y)
{
    FED(self, fed);
    cindex_t dim = fed->getDimension();
    cindex_t clkx = check_clock(x, dim);
    cindex_t clky = check_clock(y, dim);
    CONST_OPX(updateClock(clkx, clky));
}

static VALUE cFed_self_updateClock(VALUE self, VALUE x, VALUE y)
{
    FED(self, fed);
    cindex_t dim = fed->getDimension();
    cindex_t clkx = check_clock(x, dim);
    cindex_t clky = check_clock(y, dim);
    SELF_OPX(updateClock(clkx, clky));
}

static VALUE cFed_const_updateIncrement(VALUE self, VALUE x, VALUE inc)
{
    FED(self, fed);
    cindex_t clock = check_clock(x, fed->getDimension());
    int incVal = NUM2INT(inc);
    CONST_OPX(updateIncrement(clock, incVal));
}

static VALUE cFed_self_updateIncrement(VALUE self, VALUE x, VALUE inc)
{
    FED(self, fed);
    cindex_t clock = check_clock(x, fed->getDimension());
    int incVal = NUM2INT(inc);
    SELF_OPX(updateIncrement(clock, incVal));
}

static VALUE cFed_const_update(VALUE self, VALUE x, VALUE y, VALUE val)
{
    FED(self, fed);
    cindex_t dim = fed->getDimension();
    cindex_t clkx = check_clock(x, dim);
    cindex_t clky = check_clock(y, dim);
    int value = NUM2INT(val);
    CONST_OPX(update(clkx, clky, value));
}

static VALUE cFed_self_update(VALUE self, VALUE x, VALUE y, VALUE val)
{
    FED(self, fed);
    cindex_t dim = fed->getDimension();
    cindex_t clkx = check_clock(x, dim);
    cindex_t clky = check_clock(y, dim);
    int value = NUM2INT(val);
    SELF_OPX(update(clkx, clky, value));
}

static VALUE cFed_satisfies(int argc, VALUE* argv, VALUE self)
{
    check_argc(argc, 3);
    FED(self, fed);
    cindex_t dim = fed->getDimension();
    cindex_t i = check_index(argv[0], dim);
    cindex_t j = check_index(argv[1], dim);
    int bound;
    bool strict;
    if (rb_obj_is_kind_of(argv[2], cConstraint)) {
        bound = CONSTRAINT2BOUND(argv[2]);
        strict = CONSTRAINT2STRICT(argv[2]);
    } else if (argc != 4) {
        rb_raise(rb_eArgError, "wrong number of arguments (%d instead of 4)", argc);
        // Avoid warnings
        bound = dbm_INFINITY;
        strict = true;
    } else {
        bound = NUM2INT(argv[2]);
        strict = value2bool(argv[3]);
    }
    return fed->satisfies(i, j, dbm_boundbool2raw(bound, strict)) ? Qtrue : Qfalse;
}

static VALUE cFed_contains(VALUE self, VALUE arg)
{
    if (TYPE(arg) == T_ARRAY) {
        int size = call_size(arg);
        double vec[size];
        FED(self, fed);
        check_dim(size, fed->getDimension());
        value2doublearray(arg, vec, size);
        return fed->contains(vec, size) ? Qtrue : Qfalse;
    }
    rb_raise(rb_eTypeError, "Array of Fixnum or Float expected");
    return Qfalse;
}

static VALUE cFed_possibleBackDelay(VALUE self, VALUE arg)
{
    if (TYPE(arg) != T_ARRAY) {
        rb_raise(rb_eTypeError, "Array of Float expected");
    }
    int size = call_size(arg);
    FED(self, fed);
    check_dim(size, fed->getDimension());
    double vec[size];
    value2doublearray(arg, vec, size);
    return rb_float_new(fed->possibleBackDelay(vec, size));
}

static VALUE cFed_minDelay(VALUE self, VALUE arg)
{
    if (TYPE(arg) != T_ARRAY) {
        rb_raise(rb_eTypeError, "Array of Float expected");
    }
    int size = call_size(arg);
    FED(self, fed);
    check_dim(size, fed->getDimension());
    double vec[size];
    double result;
    value2doublearray(arg, vec, size);
    fed->getMinDelay(vec, size, &result);
    return rb_float_new(result);
}

static VALUE cFed_maxBackDelay(VALUE self, VALUE arg, VALUE max)
{
    if (TYPE(arg) != T_ARRAY) {
        rb_raise(rb_eTypeError, "Array of Float expected");
    }
    int size = call_size(arg);
    FED(self, fed);
    check_dim(size, fed->getDimension());
    double vec[size];
    double result, delay = NUM2DBL(max);
    value2doublearray(arg, vec, size);
    fed->getMaxBackDelay(vec, size, &result, delay);
    return rb_float_new(result);
}

static VALUE cFed_delay(VALUE self, VALUE arg)
{
    if (TYPE(arg) != T_ARRAY) {
        rb_raise(rb_eTypeError, "Array of Float expected");
    }
    int size = call_size(arg);
    FED(self, fed);
    check_dim(size, fed->getDimension());
    double vec[size];
    double min, max;
    value2doublearray(arg, vec, size);
    fed->getDelay(vec, size, &min, &max);
    VALUE result = rb_ary_new2(2);
    rb_ary_store(result, 0, rb_float_new(min));
    rb_ary_store(result, 1, rb_float_new(max));
    return result;
}

static void do_extrapolateMaxBounds(fed_t* fed, VALUE arg)
{
    cindex_t dim = fed->getDimension();
    int vec[dim];
    check_intarray(arg, vec, dim);
    fed->extrapolateMaxBounds(vec);
}
static VALUE cFed_const_extrapolateMaxBounds(VALUE self, VALUE arg) { CONST_DO(do_extrapolateMaxBounds(&result, arg)); }
static VALUE cFed_self_extrapolateMaxBounds(VALUE self, VALUE arg) { SELF_DO(do_extrapolateMaxBounds(fed, arg)); }

static void do_diagonalExtrapolateMaxBounds(fed_t* fed, VALUE arg)
{
    cindex_t dim = fed->getDimension();
    int vec[dim];
    check_intarray(arg, vec, dim);
    fed->diagonalExtrapolateMaxBounds(vec);
}
static VALUE cFed_const_diagonalExtrapolateMaxBounds(VALUE self, VALUE arg)
{
    CONST_DO(do_diagonalExtrapolateMaxBounds(&result, arg));
}
static VALUE cFed_self_diagonalExtrapolateMaxBounds(VALUE self, VALUE arg)
{
    SELF_DO(do_diagonalExtrapolateMaxBounds(fed, arg));
}

static void do_extrapolateLUBounds(fed_t* fed, VALUE low, VALUE up)
{
    cindex_t dim = fed->getDimension();
    int lower[dim], upper[dim];
    check_intarray(low, lower, dim);
    check_intarray(up, upper, dim);
    fed->extrapolateLUBounds(lower, upper);
}
static VALUE cFed_const_extrapolateLUBounds(VALUE self, VALUE low, VALUE up)
{
    CONST_DO(do_extrapolateLUBounds(&result, low, up));
}
static VALUE cFed_self_extrapolateLUBounds(VALUE self, VALUE low, VALUE up)
{
    SELF_DO(do_extrapolateLUBounds(fed, low, up));
}

static void do_diagonalExtrapolateLUBounds(fed_t* fed, VALUE low, VALUE up)
{
    cindex_t dim = fed->getDimension();
    int lower[dim], upper[dim];
    check_intarray(low, lower, dim);
    check_intarray(up, upper, dim);
    fed->diagonalExtrapolateLUBounds(lower, upper);
}
static VALUE cFed_const_diagonalExtrapolateLUBounds(VALUE self, VALUE low, VALUE up)
{
    CONST_DO(do_diagonalExtrapolateLUBounds(&result, low, up));
}
static VALUE cFed_self_diagonalExtrapolateLUBounds(VALUE self, VALUE low, VALUE up)
{
    SELF_DO(do_diagonalExtrapolateLUBounds(fed, low, up));
}

#define DECL_SELF(FUNC) \
    static VALUE cFed_self_##FUNC(VALUE self) { SELF_OP(FUNC()); }

DECL_SELF(setEmpty)
DECL_SELF(intern)
DECL_SELF(setZero)
DECL_SELF(setInit)
DECL_SELF(mergeReduce)
DECL_SELF(convexReduce)
DECL_SELF(partitionReduce)
DECL_SELF(expensiveReduce)
DECL_SELF(expensiveConvexReduce)

#define DECL_FUNC(FUNC)                                              \
    static VALUE cFed_const_##FUNC(VALUE self) { CONST_OP(FUNC()); } \
    DECL_SELF(FUNC)

DECL_FUNC(convexHull)
DECL_FUNC(up)
DECL_FUNC(down)
DECL_FUNC(freeAllUp)
DECL_FUNC(freeAllDown)
DECL_FUNC(relaxUp)
DECL_FUNC(relaxDown)
DECL_FUNC(relaxAll)

#define DECL_OP(NAME, OP)                                                                        \
    static void do_##NAME(fed_t* fed, VALUE arg) { FED_OP(OP); }                                 \
    static VALUE cFed_const_##NAME(VALUE self, VALUE arg) { CONST_DO(do_##NAME(&result, arg)); } \
    static VALUE cFed_self_##NAME(VALUE self, VALUE arg) { SELF_DO(do_##NAME(fed, arg)); }

DECL_OP(convexAdd, *fed +=)
DECL_OP(intersection, *fed &=)
DECL_OP(predt, fed->predt)
DECL_OP(removeIncludedIn, fed->removeIncludedIn)
DECL_OP(union, *fed |=)
DECL_OP(subtract, *fed -=)

#define DECL_TEST(OP) \
    static VALUE cFed_##OP(VALUE self, VALUE arg) { TEST_OP(OP); }

#define DECL_REL_TEST(OP) \
    static VALUE cFed_##OP(VALUE self, VALUE arg) { TEST_REL_OP(OP); }

DECL_TEST(isSubtractionEmpty)
DECL_TEST(intersects)
DECL_REL_TEST(lt)
DECL_REL_TEST(gt)
DECL_REL_TEST(le)
DECL_REL_TEST(ge)
DECL_REL_TEST(eq)

#define DECL_CLOCK_OP(OP)                                     \
    static VALUE cFed_const_##OP(VALUE self, VALUE x)         \
    {                                                         \
        FED(self, fed);                                       \
        cindex_t clock = check_clock(x, fed->getDimension()); \
        CONST_OPX(OP(clock));                                 \
    }                                                         \
    static VALUE cFed_self_##OP(VALUE self, VALUE x) { SELF_OP(OP(check_clock(x, fed->getDimension()))); }

DECL_CLOCK_OP(freeClock)
DECL_CLOCK_OP(freeUp)
DECL_CLOCK_OP(freeDown)
DECL_CLOCK_OP(relaxUpClock)
DECL_CLOCK_OP(relaxDownClock)

// Special up & down with stopped clocks.

static void array2bits(VALUE arg, uint32_t* bits, cindex_t dim)
{
    if (TYPE(arg) != T_ARRAY) {
        rb_raise(rb_eTypeError, "Array of Fixnum expected");
    }
    int size = call_size(arg);
    base_resetBits(bits, bits2intsize(dim));
    for (int i = 0; i < size; ++i) {
        int clock = NUM2INT(rb_ary_entry(arg, i));
        if (clock == 0 || clock >= (int)dim) {
            rb_raise(rb_eArgError, "Invalid clock argument (%d). Clock must be > 0 and < %d.", clock, dim);
        }
        base_setOneBit(bits, clock);
    }
}

static VALUE cFed_const_upStop(VALUE self, VALUE arg)
{
    FED(self, fed);
    cindex_t dim = fed->getDimension();
    uint32_t stopped[bits2intsize(dim)];
    array2bits(arg, stopped, dim);
    fed_t* result = new fed_t(*fed);
    result->upStop(stopped);
    return FED2VALUE(result);
}

static VALUE cFed_self_upStop(VALUE self, VALUE arg)
{
    FED(self, fed);
    cindex_t dim = fed->getDimension();
    uint32_t stopped[bits2intsize(dim)];
    array2bits(arg, stopped, dim);
    fed->upStop(stopped);
    return self;
}

static VALUE cFed_const_downStop(VALUE self, VALUE arg)
{
    FED(self, fed);
    cindex_t dim = fed->getDimension();
    uint32_t stopped[bits2intsize(dim)];
    array2bits(arg, stopped, dim);
    fed_t* result = new fed_t(*fed);
    result->downStop(stopped);
    return FED2VALUE(result);
}

static VALUE cFed_self_downStop(VALUE self, VALUE arg)
{
    FED(self, fed);
    cindex_t dim = fed->getDimension();
    uint32_t stopped[bits2intsize(dim)];
    array2bits(arg, stopped, dim);
    fed->downStop(stopped);
    return self;
}

// Useful macros for generating drawing information.

#define M(I, J)     m[(I)*dim + (J)]
#define BOUND(I, J) dbm_raw2bound(M(I, J))
#define SX(B)       ((int)((B)*sx))
#define SY(B)       ((int)((B)*sy))

static raw_t get_max(const raw_t* dbm, cindex_t dim, cindex_t x, cindex_t y)
{
    raw_t cx = dbm[x * dim];
    raw_t cy = dbm[y * dim];
    if (cx < dbm_LS_INFINITY && cy < dbm_LS_INFINITY)
        return cx > cy ? cx : cy;
    else if (cx < dbm_LS_INFINITY)
        return cx;
    else if (cy < dbm_LS_INFINITY)
        return cy;
    else {
        cx = dbm[x * dim + y];
        cy = dbm[y * dim + x];
        if (cx < 0)
            cx = -cx;
        if (cy < 0)
            cy = -cy;
        if (cx > cy)
            return cx < dbm_LS_INFINITY ? (6 * cx) / 5 : (6 * cy) / 5;
        else if (cy < dbm_LS_INFINITY)
            return (6 * cy) / 5;
        else if (cx < dbm_LS_INFINITY)
            return (6 * cx) / 5;
        else
            return -1;  // don't mess up with useful constraints
    }
}

static void add_line(VALUE arr, int x1, int y1, int x2, int y2)
{
    VALUE line = rb_ary_new2(4);
    rb_ary_store(line, 0, INT2FIX(x1));
    rb_ary_store(line, 1, INT2FIX(y1));
    rb_ary_store(line, 2, INT2FIX(x2));
    rb_ary_store(line, 3, INT2FIX(y2));
    rb_ary_push(arr, line);
}

static void add_point(VALUE arr, int x, int y, int val)
{
    VALUE pt = rb_ary_new2(3);
    rb_ary_store(pt, 0, INT2FIX(x));
    rb_ary_store(pt, 1, INT2FIX(y));
    rb_ary_store(pt, 2, INT2FIX(val));
    rb_ary_push(arr, pt);
}

// return value = [ [ segments1 , polygon , segments2 , info ] ], one for every DBM
// where segments1 are for border lines going to the axes x and y,
// polygon is the set of point to draw the zone, and
// segments2 are for non strict borders of the polygon
// info are [x,y,value] to print value at (x,y)
//
// Computed points (px, py):
//     4______3
//     /      |
//    /      /2
// 5 /      /
//  |      /
//  |_____/
//  0     1

static int height, border;
static double sx, sy;

static VALUE cFed_const_getDrawing(VALUE self, VALUE vborder, VALUE vwidth, VALUE vheight, VALUE vx, VALUE vy)
{
    FED(self, fed);
    cindex_t dim = fed->getDimension();
    border = NUM2INT(vborder);
    int width = NUM2INT(vwidth);
    height = NUM2INT(vheight);
    cindex_t x = check_clock(vx, dim);
    cindex_t y = check_clock(vy, dim);
    raw_t maxf = dbm_LE_ZERO;
    for (fed_t::const_iterator i = fed->begin(); !i.null(); ++i) {
        const raw_t* m = i();
        if (m) {
            raw_t maxi = get_max(m, dim, x, y);
            if (maxi > maxf)
                maxf = maxi;
        }
    }
    if (maxf < 10)
        maxf = 10;
    maxf = (6 * maxf) / 5;
    maxf >>= 1;

    VALUE result = rb_ary_new();
    sx = (double)width / (double)maxf;
    sy = (double)height / (double)maxf;
    if (sx > sy)
        sx = sy;
    if (sy > sx)
        sy = sx;
    int maxx = 1 + (int)((double)width / sx);
    int maxy = 1 + (int)((double)height / sy);
    int px[6], py[6];
    maxf = maxx > maxy ? maxx : maxy;

    for (fed_t::const_iterator i = fed->begin(); !i.null(); ++i) {
        const raw_t* m = i();
        if (m) {
            VALUE datai = rb_ary_new2(4);
            VALUE segm1 = rb_ary_new();
            VALUE poly = rb_ary_new2(6);
            VALUE segm2 = rb_ary_new();
            VALUE info = rb_ary_new();
            rb_ary_store(datai, 0, segm1);
            rb_ary_store(datai, 1, poly);
            rb_ary_store(datai, 2, segm2);
            rb_ary_store(datai, 3, info);
            rb_ary_push(result, datai);

            px[0] = border - SX(BOUND(0, x));
            py[0] = height + SY(BOUND(0, y));
            py[1] = py[0];
            int x0 = M(x, 0) < dbm_LS_INFINITY ? BOUND(x, 0) : maxx;
            if (M(x, y) < dbm_LS_INFINITY) {
                px[1] = border + SX(BOUND(x, y) - BOUND(0, y));
                px[2] = border + SX(x0);
                py[2] = height - SY(x0 - BOUND(x, y));
            } else {
                px[1] = border + width;
                px[2] = px[1];
                py[2] = py[1];
            }
            px[3] = px[2];
            int y0 = M(y, 0) < dbm_LS_INFINITY ? BOUND(y, 0) : maxy;
            if (M(y, x) < dbm_LS_INFINITY) {
                py[3] = height - SY(y0);
                px[4] = border + SX(y0 - BOUND(y, x));
                py[5] = height - SY(BOUND(y, x) - BOUND(0, x));
            } else {
                py[3] = 0;
                px[4] = border;
                py[5] = 0;
            }
            py[4] = py[3];
            px[5] = px[0];

            for (int k = 0; k < 6; ++k) {
                VALUE xy = rb_ary_new2(2);
                rb_ary_store(xy, 0, INT2FIX(px[k]));
                rb_ary_store(xy, 1, INT2FIX(py[k]));
                rb_ary_store(poly, k, xy);
            }

            // dbm[0,x]
#ifdef SHORT_LINES
            add_line(segm1, px[5], py[5], px[5], height);
#else
            add_line(segm1, px[0], 0, px[0], height);
#endif
            add_point(info, px[0], height, -BOUND(0, x));
            if (!dbm_rawIsStrict(M(0, x))) {
                add_line(segm2, px[0], py[0], px[5], py[5]);
            }

            // dbm[x,0]
            if (M(x, 0) < dbm_LS_INFINITY) {
#ifdef SHORT_LINES
                add_line(segm1, px[3], py[3], px[3], height);
#else
                add_line(segm1, px[2], 0, px[2], height);
#endif
                add_point(info, px[2], height, BOUND(x, 0));
                if (!dbm_rawIsStrict(M(x, 0))) {
                    add_line(segm2, px[2], py[2], px[3], py[3]);
                }
            }

            // dbm[0,y]
#ifdef SHORT_LINES
            add_line(segm1, border, py[1], px[1], py[1]);
#else
            add_line(segm1, border, py[0], border + width, py[0]);
#endif
            add_point(info, 0, py[0], -BOUND(0, y));
            if (!dbm_rawIsStrict(M(0, y))) {
                add_line(segm2, px[0], py[0], px[1], py[1]);
            }

            // dbm[y,0]
            if (M(y, 0) < dbm_LS_INFINITY) {
#ifdef SHORT_LINES
                add_line(segm1, border, py[3], px[3], py[3]);
#else
                add_line(segm1, border, py[4], border + width, py[4]);
#endif
                add_point(info, 0, py[4], BOUND(y, 0));
                if (!dbm_rawIsStrict(M(y, 0))) {
                    add_line(segm2, px[3], py[3], px[4], py[4]);
                }
            }

            // dbm[x,y]
            if (M(x, y) < dbm_LS_INFINITY) {
                if (M(x, y) > 0) {
                    int xx = border + SX(BOUND(x, y));
#ifdef SHORT_LINES
                    add_line(segm1, xx, height, px[2], py[2]);
#else
                    add_line(segm1, xx, height, xx + SX(maxf), height - SY(maxf));
#endif
                    add_point(info, xx, height, BOUND(x, y));
                } else {
                    int yy = height + SY(BOUND(x, y));
#ifdef SHORT_LINES
                    add_line(segm1, border, yy, px[2], py[2]);
#else
                    add_line(segm1, border, yy, border + SX(maxf), yy - SY(maxf));
#endif
                    add_point(info, 0, yy, -BOUND(x, y));
                }
                if (!dbm_rawIsStrict(M(x, y))) {
                    add_line(segm2, px[1], py[1], px[2], py[2]);
                }
            }

            // dbm[y,x]
            if (M(y, x) < dbm_LS_INFINITY) {
                if (M(y, x) > 0) {
                    int yy = height - SY(BOUND(y, x));
#ifdef SHORT_LINES
                    add_line(segm1, border, yy, px[4], py[4]);
#else
                    add_line(segm1, border, yy, border + SX(maxf), yy - SY(maxf));
#endif
                    add_point(info, 0, yy, BOUND(y, x));
                } else {
                    int xx = border - SX(BOUND(y, x));
#ifdef SHORT_LINES
                    add_line(segm1, xx, height, px[4], py[4]);
#else
                    add_line(segm1, xx, height, xx + SX(maxf), height - SY(maxf));
#endif
                    add_point(info, xx, height, -BOUND(y, x));
                }
                if (!dbm_rawIsStrict(M(y, x))) {
                    add_line(segm2, px[4], py[4], px[5], py[5]);
                }
            }
        }
    }
    return result;
}

static VALUE cFed_const_getPointDrawing(VALUE self, VALUE pt, VALUE xval, VALUE yval)
{
    FED(self, fed);
    cindex_t dim = fed->getDimension();
    double x = NUM2DBL(rb_ary_entry(pt, check_clock(xval, dim)));
    double y = NUM2DBL(rb_ary_entry(pt, check_clock(yval, dim)));
    VALUE result = rb_ary_new2(2);
    VALUE line1 = rb_ary_new2(4);
    VALUE line2 = rb_ary_new2(4);
    rb_ary_store(result, 0, line1);
    rb_ary_store(result, 1, line2);
    int cx = border + SX(x);
    int cy = height - SY(y);
    rb_ary_store(line1, 0, INT2NUM(cx - 5));
    rb_ary_store(line1, 1, INT2NUM(cy - 5));
    rb_ary_store(line1, 2, INT2NUM(cx + 5));
    rb_ary_store(line1, 3, INT2NUM(cy + 5));
    rb_ary_store(line2, 0, INT2NUM(cx - 5));
    rb_ary_store(line2, 1, INT2NUM(cy + 5));
    rb_ary_store(line2, 2, INT2NUM(cx + 5));
    rb_ary_store(line2, 3, INT2NUM(cy - 5));
    return result;
}

static VALUE cFed_const_getFormula(VALUE self, VALUE arrString)
{
    FED(self, fed);
    cindex_t dim = fed->getDimension();
    if ((int)dim > NUM2INT(rb_funcall(arrString, SIZE_ID, 0)) + 1)  // +1: no ref clock name
    {
        rb_raise(rb_eArgError, "not enough variable names (%d required)", dim - 1);
    }

    char buffer[256];
    string str;
    uint32_t minGraph[bits2intsize(dim * dim)];
    bool isFirst = true;
    char* names[dim - 1];
    for (cindex_t i = 0; i < dim - 1; ++i) {
        names[i] = STR2CSTR(rb_ary_entry(arrString, i));
    }

    for (fed_t::const_iterator dbm = fed->begin(); !dbm.null(); ++dbm) {
        const raw_t* m = dbm();
        if (m) {
            if (!isFirst) {
                str += "|";
            }
            isFirst = false;

            size_t n = dbm_cleanBitMatrix(m, dim, minGraph, dbm_analyzeForMinDBM(m, dim, minGraph));
            if (n == 0) {
                str = "true";
                break;
            }
            str += "(";

            cindex_t i, j;
            bool firstConstraint = true;

            for (i = 0; i < dim; ++i) {
                for (j = 0; j < dim; ++j) {
                    if (base_readOneBit(minGraph, i * dim + j)) {
                        // conjunction of constraints
                        if (!firstConstraint) {
                            str += " & ";
                        }
                        firstConstraint = false;

                        // write constraints i,j
                        if (i == 0)  // don't print ref clock
                        {
                            assert(j != 0);  // 0,0 never part of mingraph
                            if (dbm_addFiniteRaw(m[j], m[j * dim]) == dbm_LE_ZERO) {
                                base_resetOneBit(minGraph, j * dim);
                                // xi == b
                                snprintf(buffer, sizeof(buffer), "%s==%d", names[j - 1], -dbm_raw2bound(m[j]));
                            } else {
                                // -b <= xi
                                snprintf(buffer, sizeof(buffer), "%d%s%s", -dbm_raw2bound(m[j]),
                                         dbm_rawIsStrict(m[j]) ? "<" : "<=", names[j - 1]);
                            }
                        } else {
                            // xi-xj == b ?
                            if (dbm_addFiniteRaw(m[i * dim + j], m[j * dim + i]) == dbm_LE_ZERO) {
                                base_resetOneBit(minGraph, j * dim + i);
                                // xi == xj
                                if (j > 0 && m[i * dim + j] == dbm_LE_ZERO) {
                                    snprintf(buffer, sizeof(buffer), "%s==%s", names[i - 1], names[j - 1]);
                                } else if (j == 0) {
                                    snprintf(buffer, sizeof(buffer), "%s==%d", names[i - 1], dbm_raw2bound(m[i * dim]));
                                } else {
                                    snprintf(buffer, sizeof(buffer), "%s-%s==%d", names[i - 1], names[j - 1],
                                             dbm_raw2bound(m[i * dim + j]));
                                }
                            } else {
                                // xi < xj, xi <= xj
                                if (j > 0 && dbm_raw2bound(m[i * dim + j]) == 0) {
                                    snprintf(buffer, sizeof(buffer), "%s%s%s", names[i - 1],
                                             dbm_rawIsStrict(m[i * dim + j]) ? "<" : "<=", names[j - 1]);
                                } else if (j == 0) {
                                    snprintf(
                                        buffer, sizeof(buffer), "%s%s%d", names[i - 1],
                                        dbm_rawIsStrict(m[i * dim + j]) ? "<" : "<=", dbm_raw2bound(m[i * dim + j]));
                                } else {
                                    snprintf(
                                        buffer, sizeof(buffer), "%s-%s%s%d", names[i - 1],
                                        names[j - 1],  // i and j != 0 by pre-condition
                                        dbm_rawIsStrict(m[i * dim + j]) ? "<" : "<=", dbm_raw2bound(m[i * dim + j]));
                                }
                            }
                        }
                        str += buffer;
                    }
                }
            }
            str += ")";
        }
    }
    if (str.length() == 0) {
        str = "false";
    }
    return rb_str_new2(str.c_str());
}

static void do_changeClocks(fed_t* fed, VALUE arg)
{
    cindex_t newDim = NUM2INT(rb_funcall(arg, SIZE_ID, 0)) + 1;  // +ref clock
    cindex_t mapping[newDim];
    mapping[0] = 0;
    for (cindex_t i = 0; i < newDim - 1; ++i) {
        VALUE v = rb_ary_entry(arg, i);
        mapping[i + 1] = v == Qnil ? ~0 : NUM2INT(v);
    }
    fed->changeClocks(mapping, newDim);
}

static VALUE cFed_const_changeClocks(VALUE self, VALUE arg) { CONST_DO(do_changeClocks(&result, arg)); }

static VALUE cFed_self_changeClocks(VALUE self, VALUE arg) { SELF_DO(do_changeClocks(fed, arg)); }

static VALUE cFed_eql(VALUE self, VALUE arg)
{
    FED(self, fed1);
    FED(arg, fed2);
    if (fed1->size() != fed2->size()) {
        return Qfalse;
    }
    size_t s1, s2;
    dbm_t* f1 = fed2array(*fed1, &s1);
    dbm_t* f2 = fed2array(*fed2, &s2);
    bool eq = base_areEqual(f1, f2, s1);
    delete[] f1;
    delete[] f2;
    return eq ? Qtrue : Qfalse;
}

static VALUE cFed_hash(VALUE self)
{
    FED(self, fed);
    size_t s;
    dbm_t* f = fed2array(*fed, &s);
    uint32_t h = hash_computeU32((uint32_t*)f, s, s);
    delete[] f;
    return INT2NUM(h);
}

#define VALUEFUNC(f)                       ((VALUE(*)(ANYARGS))f)
#define METHOD(CLASS, NAME, FUNC, ARGC)    rb_define_method(CLASS, NAME, VALUEFUNC(FUNC), ARGC)
#define SINGLETON(CLASS, NAME, FUNC, ARGC) rb_define_singleton_method(CLASS, NAME, VALUEFUNC(FUNC), ARGC)

extern "C" {
#ifdef MINGW32
__declspec(dllexport)
#endif
    void Init_UDBM_lib()
{
    RUBY_UDBM_VERSION = rb_str_new2("0.12");
    UDBM_VERSION = rb_str_new2("2.0.8");
    NEW_ID = rb_intern("new");
    SIZE_ID = rb_intern("size");
    TO_A_ID = rb_intern("to_a");
    BOUND_ID = rb_intern("bound");
    STRICT_QID = rb_intern("strict?");

    mUDBM = rb_define_module("UDBM");
    rb_define_const(mUDBM, "RUDBM_VERSION", RUBY_UDBM_VERSION);
    rb_define_const(mUDBM, "UDBM_VERSION", UDBM_VERSION);
    cMatrix = rb_define_class_under(mUDBM, "Matrix", rb_cObject);
    cConstraint = rb_define_class_under(mUDBM, "Constraint", rb_cObject);

    cRelation = rb_define_class_under(mUDBM, "Relation", rb_cObject);
    rb_define_const(cRelation, rDifferent, RELATION2VALUE(base_DIFFERENT));
    rb_define_const(cRelation, rSubset, RELATION2VALUE(base_SUBSET));
    rb_define_const(cRelation, rSuperset, RELATION2VALUE(base_SUPERSET));
    rb_define_const(cRelation, rEqual, RELATION2VALUE(base_EQUAL));
    SINGLETON(cRelation, "new", cRelation_new, 1);
    METHOD(cRelation, "to_i", cRelation_toi, 0);
    METHOD(cRelation, "to_s", cRelation_tos, 0);
    METHOD(cRelation, "&", cRelation_and, 1);
    METHOD(cRelation, "<=", cRelation_weaker, 1);

    cFed = rb_define_class_under(mUDBM, "Fed", rb_cObject);
    SINGLETON(cFed, "new", cFed_new, -1);
    SINGLETON(cFed, "zero", cFed_zero, 1);
    SINGLETON(cFed, "init", cFed_init, 1);
    SINGLETON(cFed, "random", cFed_random, 1);
    SINGLETON(cFed, "relation_cache", cFed_setRelationCache, 1);
    METHOD(cFed, "initialize", cFed_initialize, -1);
    METHOD(cFed, "to_s", cFed_tos, 0);
    METHOD(cFed, "to_a", cFed_toa, 0);
    METHOD(cFed, "size", cFed_size, 0);
    METHOD(cFed, "dim", cFed_dim, 0);
    METHOD(cFed, "set_dim!", cFed_setDimension, 1);
    METHOD(cFed, "copy", cFed_copy, 0);
    METHOD(cFed, "empty?", cFed_isEmpty, 0);
    METHOD(cFed, "unbounded?", cFed_isUnbounded, 0);
    METHOD(cFed, "empty!", cFed_self_setEmpty, 0);
    METHOD(cFed, "intern!", cFed_self_intern, 0);
    METHOD(cFed, "zero!", cFed_self_setZero, 0);
    METHOD(cFed, "init!", cFed_self_setInit, 0);
    METHOD(cFed, "relation", cFed_relation, 1);
    METHOD(cFed, "convex_hull", cFed_const_convexHull, 0);
    METHOD(cFed, "convex_hull!", cFed_self_convexHull, 0);
    METHOD(cFed, "+", cFed_const_convexAdd, 1);
    METHOD(cFed, "convex_add!", cFed_self_convexAdd, 1);
    METHOD(cFed, "constrain_clock!", cFed_constrainClock, 2);
    METHOD(cFed, "constrain!", cFed_constrain, -1);
    METHOD(cFed, "&", cFed_const_intersection, 1);
    METHOD(cFed, "intersection!", cFed_self_intersection, 1);
    METHOD(cFed, "intersects?", cFed_intersects, 1);
    METHOD(cFed, "up", cFed_const_up, 0);
    METHOD(cFed, "up!", cFed_self_up, 0);
    METHOD(cFed, "up_stop", cFed_const_upStop, 1);
    METHOD(cFed, "up_stop!", cFed_self_upStop, 1);
    METHOD(cFed, "down", cFed_const_down, 0);
    METHOD(cFed, "down!", cFed_self_down, 0);
    METHOD(cFed, "down_stop", cFed_const_downStop, 1);
    METHOD(cFed, "down_stop!", cFed_self_downStop, 1);
    METHOD(cFed, "free_clock", cFed_const_freeClock, 1);
    METHOD(cFed, "free_clock!", cFed_self_freeClock, 1);
    METHOD(cFed, "free_up", cFed_const_freeUp, 1);
    METHOD(cFed, "free_up!", cFed_self_freeUp, 1);
    METHOD(cFed, "free_down", cFed_const_freeDown, 1);
    METHOD(cFed, "free_down!", cFed_self_freeDown, 1);
    METHOD(cFed, "free_all_up", cFed_const_freeAllUp, 0);
    METHOD(cFed, "free_all_up!", cFed_self_freeAllUp, 0);
    METHOD(cFed, "free_all_down", cFed_const_freeAllDown, 0);
    METHOD(cFed, "free_all_down!", cFed_self_freeAllDown, 0);
    METHOD(cFed, "update_value", cFed_const_updateValue, 2);
    METHOD(cFed, "update_value!", cFed_self_updateValue, 2);
    METHOD(cFed, "update_clock", cFed_const_updateClock, 2);
    METHOD(cFed, "update_clock!", cFed_self_updateClock, 2);
    METHOD(cFed, "update_increment", cFed_const_updateIncrement, 2);
    METHOD(cFed, "update_increment!", cFed_self_updateIncrement, 2);
    METHOD(cFed, "update", cFed_const_update, 3);
    METHOD(cFed, "update!", cFed_self_update, 3);
    METHOD(cFed, "satisfies?", cFed_satisfies, -1);
    METHOD(cFed, "relax_up", cFed_const_relaxUp, 0);
    METHOD(cFed, "relax_up!", cFed_self_relaxUp, 0);
    METHOD(cFed, "relax_down", cFed_const_relaxDown, 0);
    METHOD(cFed, "relax_down!", cFed_self_relaxDown, 0);
    METHOD(cFed, "relax_up_clock", cFed_const_relaxUpClock, 1);
    METHOD(cFed, "relax_up_clock!", cFed_self_relaxUpClock, 1);
    METHOD(cFed, "relax_down_clock", cFed_const_relaxDownClock, 1);
    METHOD(cFed, "relax_down_clock!", cFed_self_relaxDownClock, 1);
    METHOD(cFed, "relax_all", cFed_const_relaxAll, 0);
    METHOD(cFed, "relax_all!", cFed_self_relaxAll, 0);
    METHOD(cFed, "subtraction_empty?", cFed_isSubtractionEmpty, 1);
    METHOD(cFed, "|", cFed_const_union, 1);
    METHOD(cFed, "union!", cFed_self_union, 1);
    METHOD(cFed, "-", cFed_const_subtract, 1);
    METHOD(cFed, "subtract!", cFed_self_subtract, 1);
    METHOD(cFed, "merge_reduce!", cFed_self_mergeReduce, 0);
    METHOD(cFed, "convex_reduce!", cFed_self_convexReduce, 0);
    METHOD(cFed, "partition_reduce!", cFed_self_partitionReduce, 0);
    METHOD(cFed, "expensive_reduce!", cFed_self_expensiveReduce, 0);
    METHOD(cFed, "expensive_convex_reduce!", cFed_self_expensiveConvexReduce, 0);
    METHOD(cFed, "predt", cFed_const_predt, 1);
    METHOD(cFed, "predt!", cFed_self_predt, 1);
    METHOD(cFed, "remove_included_in", cFed_const_removeIncludedIn, 1);
    METHOD(cFed, "remove_included_in!", cFed_self_removeIncludedIn, 1);
    METHOD(cFed, "<", cFed_lt, 1);
    METHOD(cFed, ">", cFed_gt, 1);
    METHOD(cFed, "<=", cFed_le, 1);
    METHOD(cFed, ">=", cFed_ge, 1);
    METHOD(cFed, "==", cFed_eq, 1);
    METHOD(cFed, "contains?", cFed_contains, 1);
    METHOD(cFed, "possible_back_delay", cFed_possibleBackDelay, 1);
    METHOD(cFed, "min_delay", cFed_minDelay, 1);
    METHOD(cFed, "max_back_delay", cFed_maxBackDelay, 2);
    METHOD(cFed, "delay", cFed_delay, 1);
    METHOD(cFed, "extrapolate_max_bounds", cFed_const_extrapolateMaxBounds, 1);
    METHOD(cFed, "extrapolate_max_bounds!", cFed_self_extrapolateMaxBounds, 1);
    METHOD(cFed, "diagonal_extrapolate_max_bounds", cFed_const_diagonalExtrapolateMaxBounds, 1);
    METHOD(cFed, "diagonal_extrapolate_max_bounds!", cFed_self_diagonalExtrapolateMaxBounds, 1);
    METHOD(cFed, "extrapolate_lu_bounds", cFed_const_extrapolateLUBounds, 2);
    METHOD(cFed, "extrapolate_lu_bounds!", cFed_self_extrapolateLUBounds, 2);
    METHOD(cFed, "diagonal_extrapolate_lu_bounds", cFed_const_diagonalExtrapolateLUBounds, 2);
    METHOD(cFed, "diagonal_extrapolate_lu_bounds!", cFed_self_diagonalExtrapolateLUBounds, 2);
    METHOD(cFed, "drawing", cFed_const_getDrawing, 5);
    METHOD(cFed, "point_drawing", cFed_const_getPointDrawing, 3);
    METHOD(cFed, "formula", cFed_const_getFormula, 1);
    METHOD(cFed, "change_clocks", cFed_const_changeClocks, 1);
    METHOD(cFed, "change_clocks!", cFed_self_changeClocks, 1);
    METHOD(cFed, "eql?", cFed_eql, 1);
    METHOD(cFed, "hash", cFed_hash, 0);
}
}

// Extra: Implementation of the relation cache.

bool RelationCache::lt(fed_t& arg) { return exactRelation(arg) == base_SUBSET; }

bool RelationCache::gt(fed_t& arg) { return exactRelation(arg) == base_SUPERSET; }

bool RelationCache::le(fed_t& arg)
{
    assert(fed);
    entry_t* e = get_entry(*fed, arg);
    if (!(e->type & LE)) {
        ensure_le(*fed, arg, e);
    }
    return (e->rel & base_SUBSET) != 0;
}

bool RelationCache::ge(fed_t& arg)
{
    assert(fed);
    entry_t* e = get_entry(*fed, arg);
    if (!(e->type & GE)) {
        ensure_ge(*fed, arg, e);
    }
    return (e->rel & base_SUPERSET) != 0;
}

bool RelationCache::eq(fed_t& arg) { return exactRelation(arg) == base_EQUAL; }

uintptr_t RelationCache::exactRelation(fed_t& arg)
{
    assert(fed);
    entry_t* e = get_entry(*fed, arg);
    switch (e->type) {
    case LE: ensure_ge(*fed, arg, e); break;
    case GE: ensure_le(*fed, arg, e); break;
    case RELATION: break;
    default: e->type = RELATION; e->rel = fed->exactRelation(arg);
    }
    return e->rel;
}

void RelationCache::ensure_le(const fed_t& fed1, const fed_t& fed2, entry_t* e)
{
    bool le = fed1.le(fed2);
    e->type |= LE;
    if (le) {
        e->rel = (uintptr_t)(e->rel | base_SUBSET);
    }
}

void RelationCache::ensure_ge(const fed_t& fed1, const fed_t& fed2, entry_t* e)
{
    bool ge = fed1.ge(fed2);
    e->type |= GE;
    if (ge) {
        e->rel = (uintptr_t)(e->rel | base_SUPERSET);
    }
}

RelationCache::entry_t* RelationCache::get_entry(fed_t& f1, fed_t& f2)
{
    fed_pair_t feds;
    feds.fed1 = fed2array(f1, &feds.size1);
    feds.fed2 = fed2array(f2, &feds.size2);

    // HACK: dbm_t* matches uint32_t* because dbm_t == idbm_t*
    uint32_t hashValue = hash_computeU32((uint32_t*)feds.fed2, feds.size2,
                                         hash_computeU32((uint32_t*)feds.fed1, feds.size1, feds.size1 + feds.size2));
    entry_t** atBucket = table.getAtBucket(hashValue);
    entry_t* bucket;
    for (bucket = *atBucket; bucket != NULL; bucket = bucket->getNext()) {
        if (bucket->info == hashValue && feds.size1 == bucket->feds.size1 && feds.size2 == bucket->feds.size2 &&
            base_areEqual(feds.fed1, bucket->feds.fed1, feds.size1) &&
            base_areEqual(feds.fed2, bucket->feds.fed2, feds.size2)) {
            delete[] feds.fed1;
            delete[] feds.fed2;
            return bucket;
        }
    }
    bucket = new entry_t;
    bucket->info = hashValue;
    bucket->feds = feds;
    bucket->type = 0;
    bucket->rel = base_DIFFERENT;
    bucket->link(atBucket);
    table.incBuckets();
    return bucket;
}

dbm_t* fed2array(fed_t& f, size_t* s)
{
    f.intern();
    size_t size = f.size();
    *s = size;
    dbm_t* arr = new dbm_t[size];
    dbm_t* a = arr;
    for (fed_t::const_iterator i = f.begin(); !i.null(); ++i, ++a) {
        *a = *i;  // Reference counting as well.
    }
    std::sort(arr, arr + size);
    return arr;
}
