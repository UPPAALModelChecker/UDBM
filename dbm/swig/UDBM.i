%module UDBM
%{
#include <string.h>
#include <math.h>
#include <string>
#include <stdexcept>

#include "dbm/fed.h"
#include <new>
#include "wrapper.h"
%}
%include "wrapper.h"
%constant udbm::Constraint ZERO = udbm::ZERO;
%constant udbm::Constraint INF = udbm::INF;
