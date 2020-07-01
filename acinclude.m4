# -*- mode: autoconf; -*-
###############################################################################
#
# Filename: aclocal.m4
#
# This file is a part of the UPPAAL toolkit.
# Copyright (c) 1995 - 2000, Uppsala University and Aalborg University.
# All right reserved.
#
# Local configure macros for UPPAAL.
#
# $Id: acinclude.m4,v 1.1 2005/07/09 18:37:50 adavid Exp $
#
###############################################################################


AC_DEFUN([UA_CXX_STREAMBUF],
[
  AC_CACHE_CHECK([flavour of std::streambuf], ua_cv_cxx_streambuf, 
  [
    AC_LANG_SAVE
    AC_LANG_CPLUSPLUS
    cat > conftest.$ac_ext <<EOF
#include <iostream>
using std::streambuf;

class Test : public std::streambuf
{
   void test(std::streambuf *a) { a->sync(); }
};
EOF

    if AC_TRY_EVAL(ac_compile); then
      ua_cv_cxx_streambuf="old"
    else
      ua_cv_cxx_streambuf="new"
    fi
    AC_LANG_RESTORE
  ])

  if test $ua_cv_cxx_streambuf = old; then
    AC_DEFINE(CXX_OLD_STREAMBUF,1,[Define if the library implementation of streambuf have public interface to override])
  fi
])


AC_DEFUN([UA_CXX_SLIST],
[
  AC_CHECK_HEADER(slist, [AC_DEFINE(HAVE_SLIST,1,[Define if slist is not in ext])],
  [
    AC_CHECK_HEADER(ext/slist,
      [
	AC_DEFINE(HAVE_EXT_SLIST,1,[Define if slist is in ext])
	AC_CACHE_CHECK([slist namespace], ua_cv_cxx_slist_namespace,
        [
  	  AC_LANG_SAVE
          AC_LANG_CPLUSPLUS

          cat > conftest.$ac_ext <<EOF
#include <ext/slist>
__gnu_cxx::slist<int> test;
EOF
          if AC_TRY_EVAL(ac_compile); then
      	    ua_cv_cxx_slist_namespace="__gnu_cxx"
          else
            ua_cv_cxx_slist_namespace=""
          fi
          AC_LANG_RESTORE
        ])

	if test -n $ua_cv_cxx_slist_namespace; then
	  AC_DEFINE_UNQUOTED(CXX_SLIST_NAMESPACE,$ua_cv_cxx_slist_namespace,[Define to the namespace of slist (if not in std)])
	fi
     ])
   ])  
])
