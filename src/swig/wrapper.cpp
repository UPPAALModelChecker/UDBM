// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-
////////////////////////////////////////////////////////////////////
//
// Filename : wrapper.cpp
//
// This file is a part of the UPPAAL toolkit.
// Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
// All right reserved.
//
// $Id: wrapper.cpp,v 1.1 2005/09/17 16:37:23 adavid Exp $
//
///////////////////////////////////////////////////////////////////

#include "wrapper.h"

#include "dbm/fed.h"

#include <new>
#include <stdexcept>
#include <string>
#include <cmath>
#include <cstring>

namespace udbm
{
    int DBM::write(char* s, int size, const raw_t* m, int dim)
    {
        int len = 0;
        for (int i = 0; i < dim; ++i) {
            for (int j = 0; j < dim; ++j) {
                raw_t c = m[i * dim + j];
                if (c == dbm_LS_INFINITY) {
                    strncpy(s + len, "<inf\t", size);
                } else {
                    snprintf(s + len, size, "%s%d\t", (dbm_rawIsStrict(c) ? "<" : "<="), dbm_raw2bound(c));
                }
                int l = strlen(s + len);
                size -= l;
                len += l;
            }
            strncpy(s + len, "\\\n", size);
            size -= 2;
            len += 2;
        }
        return len;
    }

    void DBM::setDBM(dbm::dbm_t& wdbm, const DBMMatrix& arg)

    {
        // infer dimension & check array
        int size = arg.size();
        int dim = (int)sqrt((double)size);
        if (dim * dim != size) {
            throw InvalidDBMMatrix();
        }
        if (dim < 1) {
            throw InvalidDimension();
        }
        if (dim > 0xffff) {
            throw DimensionTooLarge();
        }
        // set dimension
        wdbm.setDimension(dim);
        // fill in the matrix
        raw_t* dbm = wdbm.getDBM();
        const int* argData = arg.getMatrix();
        for (int i = 0; i < size; ++i) {
            dbm[i] = argData[i];
        }
        for (int i = 0; i < dim; ++i) {
            // clocks positive, thank you
            if (dbm[i] > dbm_LE_ZERO) {
                dbm[i] = dbm_LE_ZERO;
            }
            // fix diagonal
            if (dbm[i * dim + i] > dbm_LE_ZERO) {
                dbm[i * dim + i] = dbm_LE_ZERO;
            }
        }
        // invariant: a dbm_t is always "closed"
        if (!dbm_close(dbm, dim)) {
            wdbm.setEmpty();
        }
    }

    FedArray& FedArray::operator | (const DBMMatrix& m
    {
        if (matrices->size() == 0) {
            matrixSize = m.size();
        }
        if (matrixSize != m.size()) {
            throw IncompatibleDBM();
        }
        matrices.addPointer(new DBMMatrix(m));
        return *this;
    }

}  // namespace udbm
