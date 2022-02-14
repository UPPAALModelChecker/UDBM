// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-
//
// Filename : wrapper.h
//
// Wrapper from the DBM library to SWIG.
//
// This file is a part of the UPPAAL toolkit.
// Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
// All right reserved.
//
// $Id: wrapper.h,v 1.1 2005/09/17 16:37:23 adavid Exp $
//
///////////////////////////////////////////////////////////////////

#ifndef DBM_RUBY_WRAPPER_H
#define DBM_RUBY_WRAPPER_H

// This is for swig
#ifndef INCLUDE_BASE_RELATION_H
typedef int relation_t;
#endif

#include <memory>

namespace udbm
{
    // Exceptions

    class IndexOutOfRange
    {};
    class EmptyDBM
    {};
    class InvalidDimension
    {};
    class DimensionTooLarge
    {};
    class ForbiddenMinusInfinity
    {};
    class InvalidDBMMatrix
    {};
    class OutOfMemory
    {};
    class IncompatibleDBM
    {};
    class IncompatibleFed
    {};
    class IncompatibleDBMVector
    {};
    class IncompatibleDBMPoint
    {};
    class InvalidBoundValue
    {};
    class IllegalFirstValue
    {};
    class FatalError
    {};

#define CHECKD(A)    if (    dim() != (A).dim()
#define CHECKSD(A, B) if ((A).dim() != (B).dim()
#define CHECKF(A)    if (    dim() != (A).dim()
#define CHECKSF(A, B) if ((A).dim() != (B).dim()

    // Class declared that are visible for the user:

    class Constraint;  // wraps constraints of the form <x or <=x
    class DBMMatrix;   // helper class to construct DBMs
    class FedArray;    // helper class to construct federations
    class DBM;         // user's DBM in the target language
    class Fed;         // user's federation in the target language
    class DBMVector;   // int vectors for DBMs, 1st element always == 0
    class DBMPoint;    // & similarly double vectors

    // To reduce the awful high number of copies we have.
    // Resizable referenced arrays of scalar & pointers.

    template <class T>
    class PointerAS;
    template <class P>
    class PointerAP;

    template <class T>
    class ZArrayScal : public std::enable_shared_from_this<ZArrayScal<T>>
    {
        friend class PointerAS<T>;

    public:
        // size of the array
        int size() const { return dataSize; }

        // simple reading
        const T get(int index) const
        {
            if (index < 0 || index >= size()) {
                throw IndexOutOfRange();
            }
            return data[index];
        }

        T* begin() { return data; }  // caution!
        const T* begin() const { return data; }
        const T* end() const { return data + size(); }

        virtual ~ZArrayScal()
        {
            if (data) {
                free(data);
            }
        }

        // write: check size & if it is mutable, may copy itself
        std::shared_ptr<ZArrayScal<T>> setScalar(int index, T val)
        {
            assert(index >= 0);
            std::shared_ptr<ZArrayScal<T>> arr = ensureMutableScalars(index + 1);
            arr->data[index] = val;
            return arr;
        }

        // add an element
        std::shared_ptr<ZArrayScal<T>> addScalar(T val) { return setScalar(size(), val); }

        // check mutable & size
        std::shared_ptr<ZArrayScal<T>> ensureMutableScalars(int size)
        {
            int oldSize = dataSize;
            /*            if (base::Object::isMutable())
                        {
                            if (size > oldSize)
                            {
                                data = (T*) realloc(data, size*sizeof(T));
                                if (!data)
                                {
                                    throw OutOfMemory();
                                }
                                memset(data+oldSize, 0, (size-oldSize)*sizeof(T));
                                dataSize = size;
                            }
                            return this;
                        }
                        else // not mutable!
                        {*/
            std::shared_ptr<ZArrayScal<T>> other =
                std::make_shared<ZArrayScal<T>>(oldSize >= size ? oldSize : size, false);
            memcpy(other->data, data, oldSize * sizeof(T));
            if (size > oldSize) {
                memset(other->data, 0, (size - oldSize) * sizeof(T));
            }
            return other;
            //}
        }

        // constructor only via create
        ZArrayScal(int theSize, bool reset
            : dataSize(theSize), data((T*)malloc(theSize*sizeof(T))) {
            if (!data) {
                throw OutOfMemory();
            }
            if (reset) {
                memset(data, 0, theSize * sizeof(T));
            }
        }

        protected:
        int dataSize;
        T *data;
    };

    template <class O>
    class ZArrayPtr : public ZArrayScal<O*>
    {
        friend class PointerAP<O>;

    public:
        std::shared_ptr<ZArrayPtr<O>> setPointer(int index, O* val)
        {
            assert(index >= 0);
            std::shared_ptr<ZArrayPtr<O>> arr = ensureMutablePointers(index + 1);
            arr->data[index] = val;
            return arr;
        }

        std::shared_ptr<ZArrayPtr<O>> addPointer(O* val) { return setPointer(ZArrayScal<O*>::dataSize, val); }

        std::shared_ptr<ZArrayPtr<O>> ensureMutablePointers(int size)
        {
            int oldSize = ZArrayScal<O*>::dataSize;
            /*if (base::Object::isMutable())
            {
                if (size > oldSize)
                {
                    ZArrayScal<O*>::data = (O**) realloc(ZArrayScal<O*>::data, size*sizeof(O*));
                    if (!ZArrayScal<O*>::data)
                    {
                        throw OutOfMemory();
                    }
                    memset(ZArrayScal<O*>::data+oldSize, 0, (size-oldSize)*sizeof(O*));
                    ZArrayScal<O*>::dataSize = size;
                }
                return this;
            }
            else // not mutable!
            {*/
            O** thisData = ZArrayScal<O*>::data;
            std::shared_ptr<ZArrayPtr<O>> other =
                std::make_shared<ZArrayPtr<O>>(oldSize >= size ? oldSize : size, false);
            int i;
            for (i = 0; i < oldSize; ++i) {
                other->data[i] = thisData[i] ? new O(*thisData[i]) : NULL;
            }
            for (; i < size; ++i) {
                other->data[i] = NULL;
            }
            return other;
            //}
        }

        ZArrayPtr(int theSize, bool reset
            : ZArrayScal<O*>(theSize, reset) {}
    
        virtual ~ZArrayPtr() {
            O** data = ZArrayScal<O*>::data;
            int dataSize = ZArrayScal<O*>::dataSize;
            for (int i = 0; i < dataSize; ++i) {
                if (data[i]) {
                    delete data[i];
                }
            }
        }
    };

    // Pointer to resizable arrays.

    template <class S>
    class PointerAS : public std::enable_shared_from_this<PointerAS<S>>
    {
        std::shared_ptr<ZArrayScal<S>> ptr;

    public:
        static std::shared_ptr<ZArrayScal<S>> create(int size, bool reset = true)
        {
            return std::make_shared<ZArrayScal<S>>(size, reset);
        }
        PointerAS(const std::shared_ptr<ZArrayScal<S>>& ptr): ptr(ptr) {}

        void addScalar(S val)
        {
            assert(ptr);
            ptr = ptr->addScalar(val);
        }
        void setScalar(int index, S val)
        {
            assert(ptr);
            ptr = ptr->setScalar(index, val);
        }
        void enlarge(int size)
        {
            assert(ptr);
            ptr = ptr->ensureMutableScalars(size);
        }
        S get(int index) const
        {
            assert(ptr);
            return ptr->get(index);
        }
        auto& operator->() const { return ptr; }
    };

    template <class P>
    class PointerAP
    {
        std::shared_ptr<ZArrayPtr<P>> ptr;

    public:
        static std::shared_ptr<ZArrayPtr<P>> create(int size, bool reset = true)
        {
            return std::make_shared<ZArrayPtr<P>>(size, reset);
        }
        PointerAP(const std::shared_ptr<ZArrayPtr<P>>& ptr): ptr(ptr) {}

        void addPointer(P* val)
        {
            assert(ptr);
            ptr = ptr->addPointer(val);
        }
        void setPointer(int index, P* val)
        {
            assert(ptr);
            ptr = ptr->setPointer(index, val);
        }
        P* get(int index) const
        {
            assert(ptr);
            return ptr->get(index);
        }
        auto& operator->() const { return ptr; }
    };

    // Wrap a constraint=bound + strictness, special for ruby.
    // We could store internally a raw_t, which is, an int, but
    // but but, in ruby an int looses its lower bit for internal
    // coding, which fits perfectly with constraint bounds but
    // not with raw_t because we need a full int. So don't bother
    // and use a bound and a bool for strictness.

    class Constraint
    {
    public:
        // Default: <inf
        Constraint(): str(NULL), cBound(dbm_INFINITY), cStrict(true) {}

        // Default with a bound: <=bound, except if infinity
        Constraint(int aBound
            : str(NULL) {
            setConstraint(aBound, false); }

        // Normal constructor, ignore strictness in case of infinity
        Constraint(int aBound, bool aStrict
            : str(NULL) {
            setConstraint(aBound, aStrict); }

        Constraint(const Constraint& arg)
            : str(NULL), cBound(arg.cBound), cStrict(arg.cStrict) {}

        Constraint& operator = (const Constraint& arg) {
            if (str) {
                delete[] str;
                str = NULL;
            }
            cBound = arg.cBound;
            cStrict = arg.cStrict;
            return *this;
        }

        ~Constraint() {
            if (str) {
                delete[] str;
            } }
        
        Constraint& setConstraint(int aBound, bool aStrict) {
            setBound(aBound);
            setStrict(aStrict);
            return *this;
        }

        // read access

        int getBound()  const {
            return cBound; }
        bool isStrict() const {
            return cStrict; }

        // checked write access

        Constraint& setBound(int aBound) {
            if (aBound == -dbm_INFINITY) {
                throw ForbiddenMinusInfinity();
            }
            if (aBound < -dbm_INFINITY || aBound > dbm_INFINITY) {
                throw InvalidBoundValue();
            }
            cBound = aBound;
            return *this;
        }

        Constraint& setStrict(bool aStrict) {
            cStrict = aStrict || cBound == dbm_INFINITY;
            return *this;
        }

        // very useful for ruby
        char* to_s() {
            if (!str) {
                str = new char[16];
            }
            if (cBound != dbm_INFINITY) {
                snprintf(str, 16, "%s%d", cStrict ? "<" : "<=", cBound);
            } else {
                strcpy(str, "<inf");
            }
            return str;
        }
        
        // General comparison method.
        int cmp(const Constraint& arg) {
            raw_t me = dbm_boundbool2raw(cBound, cStrict);
            raw_t other = dbm_boundbool2raw(arg.cBound, arg.cStrict);
            if (me < other) {
                return -1;
            } else if (me == other) {
                return 0;
            } else {
                return 1;
            }
        }
        
        // Special addition between constraints.
        Constraint operator + (const Constraint& arg) const {
            raw_t sum = dbm_addRawRaw(dbm_boundbool2raw(cBound, cStrict), dbm_boundbool2raw(arg.cBound, arg.cStrict));
            return Constraint(dbm_raw2bound(sum), dbm_rawIsStrict(sum));
        }
        
        // Same as *this + arg.neg, but avoid intermediate allocations.
        Constraint operator - (const Constraint& arg) const {
            if (arg.cBound == dbm_INFINITY) {
                throw ForbiddenMinusInfinity();
            }
            raw_t sum = dbm_addRawRaw(dbm_boundbool2raw(cBound, cStrict), dbm_boundbool2raw(-arg.cBound, !arg.cStrict));
            return Constraint(dbm_raw2bound(sum), dbm_rawIsStrict(sum));
        }
        
        // Constraint negation.
        Constraint neg() const {
            if (cBound == dbm_INFINITY) {
                throw ForbiddenMinusInfinity();
            }
            return Constraint(-cBound, !cStrict);
        }
        
    private:
        char *str;   // we manage our strings to avoid leaks
        int cBound;  // constraint bound
        bool cStrict;// constraint strictness flag
    };

    // Class used only to construct Federations with an easy syntax.
    class FedArray
    {
        typedef PointerAP<DBMMatrix> matrixptr_t;

    public:
        FedArray(
            : matrixSize(0), matrices(matrixptr_t::create(0)) {}

        FedArray(const DBMMatrix& m 
            : matrixSize(0), matrices(matrixptr_t::create(0)) {
            *this | m; }

        // Add DBM matrices: this is a complete misuse of
        // +, it should be += *but* += cannot always be defined
        // as we wish, e.g. like in ruby.

        FedArray& operator | (const DBMMatrix& m;

        int size() const {
            return matrices->size(); }
        const DBMMatrix& get(int index) const {
            return *matrices->get(index); }

    private:
        int matrixSize;
        matrixptr_t matrices;
    };

    // Class used only to construct a DBM with an easy syntax.
    class DBMMatrix
    {
        typedef PointerAS<int> matrix_t;

    public:
        DBMMatrix(
            : matrix(matrix_t::create(0)) {}
        
        // Add constraints
        DBMMatrix& operator < (int bound) {
            return add(bound, true);
        }
        DBMMatrix& operator <= (int bound) {
            return add(bound, false);
        }
        DBMMatrix& add(int bound, bool strict) {
            matrix.addScalar(dbm_boundbool2raw(bound, strict || bound == dbm_INFINITY));
            return *this;
        }

        // Add another matrix -> FedArray
        FedArray operator | (const DBMMatrix& m) {
            return FedArray(*this) | m;
        }
        
        // Number of constraints
        int size() const {
            return matrix->size(); }
        
        // This returns a raw_t in fact, not supposed to be visible
        // to the user but we need to read constraints.
        int get(int index) const {
            return matrix->get(index); }

        // Direct access to the matrix.
        const int* getMatrix() const {
            return matrix->begin(); }

    private:
        matrix_t matrix;
    };

    // An arrays with the invariant '1st element == 0'
    class DBMVector
    {
        typedef PointerAS<int> ivec_t;

    public:
        DBMVector(
            : str(NULL), vec(ivec_t::create(1)) {}

        DBMVector(const DBMVector& arg
            : str(NULL), vec(arg.vec) {}

        DBMVector(int n
            : str(NULL), vec(ivec_t::create(n)) {
            if (n < 1) {
                throw InvalidDimension();
            }
        }

        DBMVector& operator = (const DBMVector& arg) {
            if (str) {
                delete[] str;
                str = NULL;
            }
            vec = arg.vec;
            return *this;
        }

        ~DBMVector() {
            if (str) {
                delete[] str;
            } }

        int size() const {
            return vec->size(); }
        int get(int i) const {
            return vec->get(i); }
        const int* begin() const {
            return vec->begin(); }
        const int* end() const {
            return vec->end(); }

        // set bound elements, except the 1st
        DBMVector& set(int i, int v) {
            if (i <= 0)  // may not change 0
            {
                throw IndexOutOfRange();
            }
            if (v <= -dbm_INFINITY || v > dbm_INFINITY) {
                throw InvalidBoundValue();
            }
            vec.setScalar(i, v);
            return *this;
        }

        // Add elements on-the-fly
        DBMVector& operator << (int v) {
            if (v <= -dbm_INFINITY || v > dbm_INFINITY) {
                throw InvalidBoundValue();
            }
            vec.addScalar(v);
            return *this;
        }

        // Concatenation, skip 1st element
        DBMVector& operator << (const DBMVector& a) {
            int n = a.size();
            vec.enlarge(size() + n - 1);  // reduce reallocations
            for (int j = 1; j < n; ++j)   // jump 1st element of a
            {
                *this << a.get(j);
            }
            return *this;
        }

        // useful for ruby
        char* to_s() {
            if (str) {
                delete[] str;
            }
            int nb = size();
            int asize = nb * 12 + 11;
            str = new char[asize];
            strncpy(str, "DBMVector(", asize);
            int len = 10;
            asize -= len;
            const int* data = vec->begin();
            for (int i = 0; i < nb; ++i) {
                snprintf(str + len, asize, i > 0 ? ",%d" : "%d", data[i]);
                int l = strlen(str + len);
                len += l;
                asize -= l;
            }
            strncpy(str + len, ")", asize);
            return str;
        }

    private:
        char *str; // manage our own char* to avoid leaks
        ivec_t vec;
    };

    // An arrays with the invariant 1st element == 0
    class DBMPoint
    {
        typedef PointerAS<double> dvec_t;

    public:
        DBMPoint(
            : str(NULL), vec(dvec_t::create(1)) {}

        DBMPoint(const DBMPoint& arg
            : str(NULL), vec(arg.vec) {}

        DBMPoint(int n
            : str(NULL), vec(dvec_t::create(n)) {
            if (n < 1) {
                throw InvalidDimension();
            }
        }

        DBMPoint(const double *val, int n
            : str(NULL), vec(dvec_t::create(n)) {
            if (n < 1) {
                throw InvalidDimension();
            }
            if (fabs(val[0]) > 1e-200)  // test != 0.0 fails, 'feature' with double
            {
                throw IllegalFirstValue();
            }
            memcpy(vec->begin(), val, n * sizeof(double));
        }

        DBMPoint& operator = (const DBMPoint& arg) {
            if (str) {
                delete[] str;
                str = NULL;
            }
            vec = arg.vec;
            return *this;
        }

        ~DBMPoint() {
            if (str) {
                delete[] str;
            } }

        int size() const {
            return vec->size(); }
        double get(int i) const {
            return vec->get(i); }
        const double* begin() const {
            return vec->begin(); }
        const double* end() const {
            return vec->end(); }

        // set bound elements, except the 1st
        DBMPoint& set(int i, double v) {
            if (i <= 0)  // may not change 0
            {
                throw IndexOutOfRange();
            }
            if (v <= -dbm_INFINITY || v > dbm_INFINITY) {
                throw InvalidBoundValue();
            }
            vec.setScalar(i, v);
            return *this;
        }

        // Add elements on-the-fly
        DBMPoint& operator << (int v) {
            return *this << (double)v;
        }
        DBMPoint& operator << (double v) {
            if (v <= -dbm_INFINITY || v > dbm_INFINITY) {
                throw InvalidBoundValue();
            }
            vec.addScalar(v);
            return *this;
        }

        // Concatenation, skip 1st element
        DBMPoint& operator << (const DBMPoint& a) {
            int n = a.size();
            vec.enlarge(size() + n - 1);  // reduce reallocations
            for (int j = 1; j < n; ++j)   // jump 1st element of a
            {
                *this << a.get(j);
            }
            return *this;
        }
        DBMPoint& operator << (const DBMVector& a) {
            int n = a.size();
            vec.enlarge(size() + n - 1);  // reduce reallocations
            for (int j = 1; j < n; ++j)   // jump 1st element of a
            {
                *this << a.get(j);
            }
            return *this;
        }

        // useful for ruby
        char* to_s() {
            if (str) {
                delete[] str;
            }
            int nb = size();
            int asize = nb * 30 + 10;
            str = new char[asize];
            strncpy(str, "DBMPoint(", asize);
            int len = 9;
            asize -= len;
            const double* data = vec->begin();
            for (int i = 0; i < nb; ++i) {
                snprintf(str + len, asize, i > 0 ? ",%.3f" : "%.3f", data[i]);
                int l = strlen(str + len);
                len += l;
                asize -= l;
            }
            strncpy(str + len, ")", asize);
            return str;
        }

    private:
        char *str; // manage our own char* to avoid leaks
        dvec_t vec;
    };

    // Wrap dbm_t, only high level methods & operators.
    class DBM
    {
        friend class Fed;

    public:
        DBM(int d
            : str(NULL), wdbm(d) {
            if (d < 1) {
                throw InvalidDimension();
            }
            if (d > 0xffff) {
                throw DimensionTooLarge();
            }
        }

        DBM(const DBM& arg)
            : str(NULL), wdbm(arg.wdbm) {}

        DBM(const DBMMatrix& arg
            : str(NULL) {
            setDBM(wdbm, arg); }

        DBM& operator = (const DBM& arg) {
            if (str) {
                delete[] str;
                str = NULL;
            }
            wdbm = arg.wdbm;
            return *this;
        }

        // Deallocate own managed string!
        ~DBM() {
            if (str) {
                delete[] str;
            } }
        
        // Special for ruby.
        char* to_s() {
            if (str) {
                delete[] str;
            }
            int d = dim();
            const raw_t* m = wdbm();
            if (m) {
                int size = allocSize(d);
                str = new char[size];
                // prefix
                snprintf(str, size, "DBM(%d) { matrix\\\n", d);
                int len = strlen(str);
                size -= len;
                // matrix
                int mlen = write(str + len, size, m, d);
                len += mlen;
                size -= mlen;
                // suffix
                strncpy(str + len, "}", size);
                assert(size - 1 >= 0);
            } else {
                str = new char[20];
                snprintf(str, 20, "DBM(%d) {}", d);
            }
            return str;
        }
        
        // Accessing a constraint in a high level manner.
        Constraint read(int i, int j) const {
            int d = wdbm.getDimension();
            if (i < 0 || j < 0 || i >= d || j >= d) {
                throw IndexOutOfRange();
            }
            const raw_t* m = wdbm();
            if (!m) {
                throw EmptyDBM();
            }
            raw_t c = m[i * d + j];
            return Constraint(dbm_raw2bound(c), dbm_rawIsStrict(c));
        }

        // For convenience: Generate DBMs.

        static DBM init(int dim) {
            if (dim < 1) {
                throw InvalidDimension();
            }
            if (dim > 0xffff) {
                throw DimensionTooLarge();
            }
            dbm::dbm_t tmp(dim);
            tmp.setInit();
            return DBM(tmp);
        }
        static DBM zero(int dim) {
            if (dim < 1) {
                throw InvalidDimension();
            }
            if (dim > 0xffff) {
                throw DimensionTooLarge();
            }
            dbm::dbm_t tmp(dim);
            tmp.setZero();
            return DBM(tmp);
        }

        // Wrapped methods & operators, see dbm_t

        int  dim()         const {
            return wdbm.getDimension(); }
        bool isEmpty()     const {
            return wdbm.isEmpty(); }
        DBM  copy()        const {
            return DBM(wdbm); }
        bool isUnbounded() const {
            return wdbm.isUnbounded(); }

        void setEmpty() {
            wdbm.setEmpty(); }
        void intern()   {
            if (str) {
                delete[] str;
                str = NULL;
            }
            wdbm.intern(); }
        void setZero()  {
            wdbm.setZero(); }
        void setInit()  {
            wdbm.setInit(); }

        relation_t relation(const DBM& arg) const {
            CHECKD(arg);
            return wdbm.relation(arg.wdbm);
        }
        relation_t relation(const Fed& arg)      const ;
        relation_t exactRelation(const DBM& arg) const {
            CHECKD(arg);
            return wdbm.exactRelation(arg.wdbm);
        }
        relation_t exactRelation(const Fed& arg) const ;

        DBM operator + (const DBM& arg) const {
            CHECKD(arg);
            return DBM(dbm::dbm_t(wdbm) += arg.wdbm);
        }
        DBM operator + (const Fed& arg) const ;
        DBM& convexHull(const DBM& arg) {
            CHECKD(arg);
            wdbm += arg.wdbm;
            return *this;
        }
        DBM& convexHull(const Fed& arg;            

        bool constrainClock(int clk, int value) {
            return wdbm.constrain(clk, value); }
        bool constrain(int i, int j, Constraint& c) {
            return constrain(i, j, c.getBound(), c.isStrict()); }
        bool constrain(int i, int j, int bound, bool strict) {
            return wdbm.constrain(i, j, bound, strict); }

        DBM operator & (const DBM& arg) {
            CHECKD(arg);
            return DBM(dbm::dbm_t(wdbm) &= arg.wdbm);
        }
        Fed operator & (const Fed& arg;
        bool intersects(const DBM& arg) const {
            CHECKD(arg);
            return wdbm.intersects(arg.wdbm);
        }
        bool intersects(const Fed& arg) const ;
        DBM& intersectionWith(const DBM& arg) {
            CHECKD(arg);
            wdbm &= arg.wdbm;
            return *this;
        }
        //DBM& intersectionWith(const Fed& arg) not possible!

        DBM& applyUp()               {
            wdbm.up();
            return *this; }
        DBM& applyDown()             {
            wdbm.down();
            return *this; }
        DBM& applyFreeClock(int clk) {
            wdbm.freeClock(clk);
            return *this; }
        DBM& applyFreeUp(int clk)    {
            wdbm.freeUp(clk);
            return *this; }
        DBM& applyFreeDown(int clk)  {
            wdbm.freeDown(clk);
            return *this; }
        DBM& applyFreeAllUp()        {
            wdbm.freeAllUp();
            return *this; }
        DBM& applyFreeAllDown()      {
            wdbm.freeAllDown();
            return *this; }

        DBM& updateValue(int x, int v)          {
            wdbm.updateValue(x, v);
            return *this; }
        DBM& updateClock(int x, int y)          {
            wdbm.updateClock(x, y);
            return *this; }
        DBM& updateIncrement(int x, int v)      {
            wdbm.updateIncrement(x, v);
            return *this; }
        DBM& updateGeneral(int x, int y, int v) {
            wdbm.update(x, y, v);
            return *this; }

        bool satisfies(int i, int j, int bound, bool strict) const {
            return wdbm.satisfies(i, j, dbm_boundbool2raw(bound, strict)); }
        bool satisfies(int i, int j, const Constraint& c)    const {
            return satisfies(i, j, c.getBound(), c.isStrict()); }

        DBM& relaxUp()        {
            wdbm.relaxUp();
            return *this; }
        DBM& relaxDown()      {
            wdbm.relaxDown();
            return *this; }
        DBM& relaxUp(int k)   {
            wdbm.relaxUpClock(k);
            return *this; }
        DBM& relaxDown(int k) {
            wdbm.relaxDownClock(k);
            return *this; }
        DBM& relaxAll()       {
            wdbm.relaxAll();
            return *this; }

        bool isSubtractionEmpty(const DBM& arg) const {
            CHECKD(arg);
            return wdbm.isSubtractionEmpty(arg.wdbm);
        }
        bool isSubtractionEmpty(const Fed& arg) const ;

        // Relation operator: the problem is that we
        // cannot define a 'cmp' method like for Constraint
        // because DBMs are not necesseraly comparable!
        // Relations in the sense of set inclusion:

        bool operator <  (const DBM& arg) const {
            CHECKD(arg);
            return wdbm < arg.wdbm; }
        bool operator >  (const DBM& arg) const {
            CHECKD(arg);
            return wdbm > arg.wdbm; }
        bool operator <= (const DBM& arg) const {
            CHECKD(arg);
            return wdbm <= arg.wdbm; }
        bool operator >= (const DBM& arg) const {
            CHECKD(arg);
            return wdbm >= arg.wdbm; }
        bool operator == (const DBM& arg) const {
            CHECKD(arg);
            return wdbm == arg.wdbm; }
        bool operator <  (const Fed& arg) const ;
        bool operator >  (const Fed& arg) const ;
        bool operator <= (const Fed& arg) const ;
        bool operator >= (const Fed& arg) const ;
        bool operator == (const Fed& arg) const ;

        // Relations in the sense of DBM inclusion.

        bool isIncludedIn(const DBM& arg)         const {
            CHECKD(arg);
            return wdbm <= arg.wdbm; }
        bool isIncludedIn(const Fed& arg)         const ;
        bool isStrictlyIncludedIn(const DBM& arg) const {
            CHECKD(arg);
            return wdbm < arg.wdbm; }
        bool isStrictlyIncludedIn(const Fed& arg) const ;

        // Interaction with DBMVector and DBMPoint

        bool contains(const DBMVector& v) const {
            if (dim() != v.size()) {
                throw IncompatibleDBMVector();
            }
            return wdbm.contains(v.begin(), v.size());
        }
        bool contains(const DBMPoint& v) const {
            if (dim() != v.size()) {
                throw IncompatibleDBMPoint();
            }
            return wdbm.contains(v.begin(), v.size());
        }
        DBM& extrapolateMaxBounds(const DBMVector& v) {
            if (dim() != v.size()) {
                throw IncompatibleDBMVector();
            }
            wdbm.extrapolateMaxBounds(v.begin());
            return *this;
        }
        DBM& diagonalExtrapolateMaxBounds(const DBMVector& v) {
            if (dim() != v.size()) {
                throw IncompatibleDBMVector();
            }
            wdbm.diagonalExtrapolateMaxBounds(v.begin());
            return *this;
        }
        DBM& extrapolateLUBounds(const DBMVector& l, const DBMVector& u) {
            if (dim() != l.size() || dim() != u.size()) {
                throw IncompatibleDBMVector();
            }
            wdbm.extrapolateLUBounds(l.begin(), u.begin());
            return *this;
        }
        DBM& diagonalExtrapolateLUBounds(const DBMVector& l, const DBMVector& u) {
            if (dim() != l.size() || dim() != u.size()) {
                throw IncompatibleDBMVector();
            }
            wdbm.diagonalExtrapolateLUBounds(l.begin(), u.begin());
            return *this;
        }
        DBMPoint getPoint() const {
            int d = dim();
            dbm::DoubleValuation val(d);
            wdbm.getValuation(val.begin(), d);
            return DBMPoint(val.begin(), d);
        }

    private:
        // max int = 2147483647 = 10 char + sign + <= + space = 14
        // matrix = dim*dim*14 + 2*dim (2 char for end of line \\\n)
        // prefix 'DBM(x) { matrix\\\n' = 16 + max dim 64534 (5) = 21
        // suffix '}' = 1
        static int allocSize(int dim) {
            return dim * (dim * 14 + 2) + 22; }

        // write a DBM in a large enough string
        static int write(char *s, int size, const raw_t *m, int dim);

        // init a dbm_t from a DBMMatrix
        static void setDBM(dbm::dbm_t& wdbm, const DBMMatrix& arg)
            ;

        DBM(const dbm::dbm_t& arg)
            : str(NULL), wdbm(arg) {}

        char *str;       // we manage our strings, otherwise leaks
        dbm::dbm_t wdbm; // wrapped dbm_t
    };

    // Wrap fed_t, only high level methods & operators.
    class Fed
    {
        friend class DBM;

    public:
        Fed(int d
            : str(NULL), wfed(d) {
            if (d <= 0) {
                throw InvalidDimension();
            }
        }

        Fed(const DBM& arg)
            : str(NULL), wfed(arg.wdbm) {}

        Fed(const Fed& arg)
            : str(NULL), wfed(arg.wfed) {}

        Fed& operator = (const Fed& arg) {
            if (str) {
                delete[] str;
                str = NULL;
            }
            wfed = arg.wfed;
            return *this;
        }
                
        // Deallocate own managed string!
        ~Fed() {
            if (str) {
                delete[] str;
            } }
        
        // Add DBMs
        Fed& add(const FedArray& arr) {
            int n = arr.size();
            for (int i = 0; i < n; ++i) {
                add(arr.get(i));
            }
            return *this;
        }
        Fed& add(const DBMMatrix &m) {
            dbm::dbm_t tmp;
            DBM::setDBM(tmp, m);
            if ((int)tmp.getDimension() != dim()) {
                throw IncompatibleDBM();
            }
            wfed |= tmp;
            return *this;
        }
        Fed& add(const DBM& arg) {
            CHECKD(arg);
            wfed |= arg.wdbm;
            return *this;
        }
        Fed& add(const Fed& arg) {
            CHECKF(arg);
            wfed |= arg.wfed;
            return *this;
        }

        // Special for ruby.
        char* to_s() {
            if (str) {
                delete[] str;
            }
            int fedSize = size();
            if (fedSize) {
                int asize = allocSize(fedSize, dim());
                str = new char[asize];
                // prefix
                snprintf(str, asize, "Fed(%d) %s", dim(), fedSize > 1 ? "{[\n" : "{");
                int len = strlen(str);
                asize -= len;
                // list of DBMs
                bool isFirst = true;
                int d = dim();
                for (dbm::fed_t::const_iterator i = wfed.begin(); !i.null(); ++i) {
                    // in principle no empty DBM
                    const raw_t* m = i();
                    if (m) {
                        // DBM prefix
                        strncpy(str + len, isFirst ? " matrix\\\n" : ",matrix\\\n", asize);
                        len += 9;
                        asize -= 9;
                        // matrix
                        int mlen = DBM::write(str + len, asize, m, d);
                        len += mlen;
                        asize -= mlen;
                        isFirst = false;
                    } else {
                        fprintf(stderr, "Warning: Empty DBM found in Fed!\n");
                    }
                }
                // suffix
                strncpy(str + len, fedSize > 1 ? "]}" : "}", asize);
                assert(asize - 2 >= 0);
            } else {
                str = new char[20];
                snprintf(str, 20, "Fed(%d) {}", dim());
            }
            return str;
        }
        
        // For convenience: Generate Feds.

        static Fed init(int dim) {
            if (dim < 1) {
                throw InvalidDimension();
            }
            if (dim > 0xffff) {
                throw DimensionTooLarge();
            }
            dbm::fed_t tmp(dim);
            tmp.setInit();
            return Fed(tmp);
        }
        static Fed zero(int dim) {
            if (dim < 1) {
                throw InvalidDimension();
            }
            if (dim > 0xffff) {
                throw DimensionTooLarge();
            }
            dbm::fed_t tmp(dim);
            tmp.setZero();
            return Fed(tmp);
        }

        // Wrapped methods & operators, see fed_t

        int size()         const {
            return wfed.size(); }
        int dim()          const {
            return wfed.getDimension(); }
        bool isEmpty()     const {
            return wfed.isEmpty(); }
        Fed copy()         const {
            return Fed(wfed); }
        bool isUnbounded() const {
            return wfed.isUnbounded(); }

        void setEmpty() {
            wfed.setEmpty(); }
        void intern()   {
            if (str) {
                delete[] str;
                str = NULL;
            }
            wfed.intern(); }
        void setZero()  {
            wfed.setZero(); }
        void setInit()  {
            wfed.setInit(); }

        relation_t relation(const DBM& arg) const {
            CHECKD(arg);
            return wfed.relation(arg.wdbm);
        }
        relation_t relation(const Fed& arg) const {
            CHECKF(arg);
            return wfed.relation(arg.wfed);
        }
        relation_t exactRelation(const DBM& arg) const {
            CHECKD(arg);
            return wfed.exactRelation(arg.wdbm);
        }
        relation_t exactRelation(const Fed& arg) const {
            CHECKF(arg);
            return wfed.exactRelation(arg.wfed);
        }

        Fed operator + (const DBM& arg) const {
            CHECKD(arg);
            return Fed(dbm::fed_t(wfed) += arg.wdbm);
        }
        Fed operator + (const Fed& arg) const {
            CHECKF(arg);
            return Fed(dbm::fed_t(wfed) += arg.wfed);
        }
        Fed& convexHull(const DBM& arg) {
            CHECKD(arg);
            wfed += arg.wdbm;
            return *this;
        }
        Fed& convexHull(const Fed& arg) {
            CHECKF(arg);
            wfed += arg.wfed;
            return *this;
        }
        Fed& convexHull() {
            wfed.convexHull();
            return *this; }

        bool constrainClock(int clk, int value) {
            return wfed.constrain(clk, value); }
        bool constrain(int i, int j, Constraint& c) {
            return constrain(i, j, c.getBound(), c.isStrict()); }
        bool constrain(int i, int j, int bound, bool strict) {
            return wfed.constrain(i, j, bound, strict); }

        Fed operator & (const DBM& arg) {
            CHECKD(arg);
            return Fed(dbm::fed_t(wfed) &= arg.wdbm);
        }
        Fed operator & (const Fed& arg) {
            CHECKF(arg);
            return Fed(dbm::fed_t(wfed) &= arg.wfed);
        }
        bool intersects(const DBM& arg) const {
            CHECKD(arg);
            return wfed.intersects(arg.wdbm);
        }
        bool intersects(const Fed& arg) const {
            CHECKF(arg);
            return wfed.intersects(arg.wfed);
        }
        Fed& intersectionWith(const DBM& arg) {
            CHECKD(arg);
            wfed &= arg.wdbm;
            return *this;
        }
        Fed& intersectionWith(const Fed& arg) {
            CHECKF(arg);
            wfed &= arg.wfed;
            return *this;
        }
        bool has(const DBM& arg) const {
            CHECKD(arg);
            return wfed.has(arg.wdbm);
        }

        Fed& applyUp()               {
            wfed.up();
            return *this; }
        Fed& applyDown()             {
            wfed.down();
            return *this; }
        Fed& applyFreeClock(int clk) {
            wfed.freeClock(clk);
            return *this; }
        Fed& applyFreeUp(int clk)    {
            wfed.freeUp(clk);
            return *this; }
        Fed& applyFreeDown(int clk)  {
            wfed.freeDown(clk);
            return *this; }
        Fed& applyFreeAllUp()        {
            wfed.freeAllUp();
            return *this; }
        Fed& applyFreeAllDown()      {
            wfed.freeAllDown();
            return *this; }

        Fed& updateValue(int x, int v)          {
            wfed.updateValue(x, v);
            return *this; }
        Fed& updateClock(int x, int y)          {
            wfed.updateClock(x, y);
            return *this; }
        Fed& updateIncrement(int x, int v)      {
            wfed.updateIncrement(x, v);
            return *this; }
        Fed& updateGeneral(int x, int y, int v) {
            wfed.update(x, y, v);
            return *this; }

        bool satisfies(int i, int j, int bound, bool strict) const {
            return wfed.satisfies(i, j, dbm_boundbool2raw(bound, strict)); }
        bool satisfies(int i, int j, const Constraint& c) const {
            return satisfies(i, j, c.getBound(), c.isStrict()); }

        Fed& relaxUp()        {
            wfed.relaxUp();
            return *this; }
        Fed& relaxDown()      {
            wfed.relaxDown();
            return *this; }
        Fed& relaxUp(int k)   {
            wfed.relaxUpClock(k);
            return *this; }
        Fed& relaxDown(int k) {
            wfed.relaxDownClock(k);
            return *this; }
        Fed& relaxAll()       {
            wfed.relaxAll();
            return *this; }

        bool isSubtractionEmpty(const DBM& arg) const {
            CHECKD(arg);
            return wfed.isSubtractionEmpty(arg.wdbm);
        }
        bool isSubtractionEmpty(const Fed& arg) const {
            CHECKF(arg);
            return wfed.isSubtractionEmpty(arg.wfed);
        }

        Fed operator | (const DBM& arg) const {
            CHECKD(arg);
            return Fed(dbm::fed_t(wfed) |= arg.wdbm);
        }
        Fed operator | (const Fed& arg) const {
            CHECKF(arg);
            return Fed(dbm::fed_t(wfed) |= arg.wfed);
        }
        Fed& unionWith(const DBM& arg) {
            CHECKD(arg);
            wfed |= arg.wdbm;
            return *this;
        }
        Fed& unionWith(const Fed& arg) {
            CHECKF(arg);
            wfed |= arg.wfed;
            return *this;
        }

        Fed operator - (const DBM& arg) const {
            CHECKD(arg);
            return Fed(dbm::fed_t(wfed) -= arg.wdbm);
        }
        Fed operator - (const Fed& arg) const {
            CHECKF(arg);
            return Fed(dbm::fed_t(wfed) -= arg.wfed);
        }
        Fed& subtract(const DBM& arg) {
            CHECKD(arg);
            wfed -= arg.wdbm;
            return *this;
        }
        Fed& subtract(const Fed& arg) {
            CHECKF(arg);
            wfed -= arg.wfed;
            return *this;
        }

        Fed& mergeReduce()     {
            wfed.mergeReduce();
            return *this; }
        Fed& partitionReduce() {
            wfed.partitionReduce();
            return *this; }

        Fed& applyPredt(const DBM& bad) {
            CHECKD(bad);
            wfed.predt(bad.wdbm);
            return *this;
        }
        Fed& applyPredt(const Fed& bad) {
            CHECKF(bad);
            wfed.predt(bad.wfed);
            return *this;
        }

        Fed& removeIncludedIn(const DBM& arg) {
            CHECKD(arg);
            wfed.removeIncludedIn(arg.wdbm);
            return *this;
        }
        Fed& removeIncludedIn(const Fed& arg) {
            CHECKF(arg);
            wfed.removeIncludedIn(arg.wfed);
            return *this;
        }

        // Relation operator: the *big* problem is that we
        // cannot define a 'cmp' method like for Constraint
        // because DBMs are not necesseraly comparable!
        // Relations in the sense of set inclusion:

        bool operator <  (const DBM& arg) const {
            CHECKD(arg);
            return wfed.lt(arg.wdbm); }
        bool operator >  (const DBM& arg) const {
            CHECKD(arg);
            return wfed.gt(arg.wdbm); }
        bool operator <= (const DBM& arg) const {
            CHECKD(arg);
            return wfed.le(arg.wdbm); }
        bool operator >= (const DBM& arg) const {
            CHECKD(arg);
            return wfed.ge(arg.wdbm); }
        bool operator == (const DBM& arg) const {
            CHECKD(arg);
            return wfed.eq(arg.wdbm); }
        bool operator <  (const Fed& arg) const {
            CHECKF(arg);
            return wfed.lt(arg.wfed); }
        bool operator >  (const Fed& arg) const {
            CHECKF(arg);
            return wfed.gt(arg.wfed); }
        bool operator <= (const Fed& arg) const {
            CHECKF(arg);
            return wfed.le(arg.wfed); }
        bool operator >= (const Fed& arg) const {
            CHECKF(arg);
            return wfed.ge(arg.wfed); }
        bool operator == (const Fed& arg) const {
            CHECKF(arg);
            return wfed.eq(arg.wfed); }

        // Relations in the sense of DBM inclusion.

        bool isIncludedIn(const DBM& arg)         const {
            CHECKD(arg);
            return wfed <= arg.wdbm; }
        bool isIncludedIn(const Fed& arg)         const {
            CHECKF(arg);
            return wfed <= arg.wfed; }
        bool isStrictlyIncludedIn(const DBM& arg) const {
            CHECKD(arg);
            return wfed < arg.wdbm; }
        bool isStrictlyIncludedIn(const Fed& arg) const {
            CHECKF(arg);
            return wfed < arg.wfed; }

        // Interaction with DBMVector and DBMPoint

        bool contains(const DBMVector& v) const {
            if (dim() != v.size()) {
                throw IncompatibleDBMVector();
            }
            return wfed.contains(v.begin(), v.size());
        }
        bool contains(const DBMPoint& v) const {
            if (dim() != v.size()) {
                throw IncompatibleDBMPoint();
            }
            return wfed.contains(v.begin(), v.size());
        }
        double possibleBackDelay(const DBMPoint& pt) const {
            if (dim() != pt.size()) {
                throw IncompatibleDBMPoint();
            }
            return wfed.possibleBackDelay(pt.begin(), pt.size());
        }
        Fed& extrapolateMaxBounds(const DBMVector& v) {
            if (dim() != v.size()) {
                throw IncompatibleDBMVector();
            }
            wfed.extrapolateMaxBounds(v.begin());
            return *this;
        }
        Fed& diagonalExtrapolateMaxBounds(const DBMVector& v) {
            if (dim() != v.size()) {
                throw IncompatibleDBMVector();
            }
            wfed.diagonalExtrapolateMaxBounds(v.begin());
            return *this;
        }
        Fed& extrapolateLUBounds(const DBMVector& l, const DBMVector& u) {
            if (dim() != l.size() || dim() != u.size()) {
                throw IncompatibleDBMVector();
            }
            wfed.extrapolateLUBounds(l.begin(), u.begin());
            return *this;
        }
        Fed& diagonalExtrapolateLUBounds(const DBMVector& l, const DBMVector& u) {
            if (dim() != l.size() || dim() != u.size()) {
                throw IncompatibleDBMVector();
            }
            wfed.diagonalExtrapolateLUBounds(l.begin(), u.begin());
            return *this;
        }
        DBMPoint getPoint() const {
            int d = dim();
            dbm::DoubleValuation val(d);
            wfed.getValuation(val.begin(), d);
            return DBMPoint(val.begin(), d);
        }

    private:
        // size per DBM=dim*(dim*14+2)
        // prefix: 'Fed(dim) {[\n' [9+5]
        //        + ' matrix\\n' for 1st [9]
        //        + ',matrix\\n' for others [9]
        // suffix: ']}' [2]
        // total = 16+size*(9+sizeofDBM)
        static int allocSize(int size, int dim) {
            return 16 + size * (9 + dim * (dim * 14 + 2)); }

        Fed(const dbm::fed_t& arg)
            : str(NULL), wfed(arg) {}

        char *str;       // we manage our strings, otherwise leaks
        dbm::fed_t wfed; // wrapped fed_t
    };

    // Implementation of methods that have dependency problems.

    inline bool DBM::operator<(const Fed& arg) const
    {
        CHECKF(arg);
        return wdbm.lt(arg.wfed);
    }
    inline bool DBM::operator>(const Fed& arg) const
    {
        CHECKF(arg);
        return wdbm.gt(arg.wfed);
    }
    inline bool DBM::operator<=(const Fed& arg) const
    {
        CHECKF(arg);
        return wdbm.le(arg.wfed);
    }
    inline bool DBM::operator>=(const Fed& arg) const
    {
        CHECKF(arg);
        return wdbm.ge(arg.wfed);
    }
    inline bool DBM::operator==(const Fed& arg) const
    {
        CHECKF(arg);
        return wdbm.eq(arg.wfed);
    }
    inline bool DBM::isIncludedIn(const Fed& arg) const
    {
        CHECKF(arg);
        return wdbm <= arg.wfed;
    }
    inline bool DBM::isStrictlyIncludedIn(const Fed& arg) const
    {
        CHECKF(arg);
        return wdbm < arg.wfed;
    }
    inline relation_t DBM::relation(const Fed& arg) const
    {
        CHECKF(arg);
        return wdbm.relation(arg.wfed);
    }
    inline relation_t DBM::exactRelation(const Fed& arg) const
    {
        CHECKF(arg);
        return wdbm.exactRelation(arg.wfed);
    }
    inline DBM DBM::operator+(const Fed& arg) const
    {
        CHECKF(arg);
        return DBM(dbm::dbm_t(wdbm) += dbm::fed_t(arg.wfed));
    }
    inline DBM& DBM::convexHull(const Fed& arg
    {
        CHECKF(arg);
        wdbm += arg.wfed;
        return *this;
    }
    inline Fed DBM::operator & (const Fed& arg
    {
        CHECKF(arg);
        return Fed(dbm::fed_t(arg.wfed) &= wdbm);
    }
    inline bool DBM::intersects(const Fed& arg) const 
    {
        CHECKF(arg);
        return wdbm.intersects(arg.wfed);
    }
    inline bool DBM::isSubtractionEmpty(const Fed& arg) const 
    {
        CHECKF(arg);
        return wdbm.isSubtractionEmpty(arg.wfed);
    }
    inline Fed operator - (const DBM& arg1, const DBM& arg2
    {
        CHECKSD(arg1, arg2);
        return Fed(arg1) - arg2;
    }
    inline Fed operator - (const DBM& arg1, const Fed& arg2
    {
        CHECKSF(arg1, arg2);
        return Fed(arg1) - arg2;
    }

    // Don't bother with limitations of certain language on constant naming.

    static inline int inf() {
        return dbm_INFINITY; }
    
    // Constants

    static const Constraint ZERO = Constraint(0);
    static const Constraint INF = Constraint(dbm_INFINITY);

    enum { DIFFERENT = 0,
           SUPERSET = 1,
           SUBSET = 2,
           EQUAL = 3 };

}  // namespace udbm

#endif  // DBM_RUBY_WRAPPER_H
