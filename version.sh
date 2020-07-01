#! /bin/sh
###############################################################################
#
# Filename: version.sh
#
# This file is a part of the UPPAAL toolkit.
# Copyright (c) 1995 - 2000, Uppsala University and Aalborg University.
# All right reserved.
#
# File containing the current version of UPPAAL.
#
# $Id: version.sh,v 1.12 2005/11/21 19:18:14 behrmann Exp $
#
###############################################################################

REVISION=4761

if [ -n "$LANG" ] ; then unset LANG ; fi
if [ -z "$REVISION" ]; then
    if [ -d .svn ]; then
	REVISION=`svn info | grep Revision | sed s/Revision:/rev./g`
    else
	# assume git svn repository:
	REVISION=`git svn info | grep Revision | sed s/Revision:/rev./g`
    fi
fi
UPPAAL_VERSION="4.1.4 ($REVISION)"
TIGA_VERSION="4.1.4-0.16 ($REVISION)"
DBM_VERSION="2.0.8"
RELEASE_MONTH=`date "+%B"`
RELEASE_YEAR=`date "+%Y"`

SED='sed -e "s/@UPPAAL_VERSION@/$UPPAAL_VERSION/g" \
         -e "s/@DBM_VERSION@/$DBM_VERSION/g" \
         -e "s/@RELEASE_MONTH@/$RELEASE_MONTH/g" \
         -e "s/@RELEASE_YEAR@/$RELEASE_YEAR/g" \
         -e "s/@TIGA_VERSION@/$TIGA_VERSION/g"'

if [ "$1" != "0" ]; then
 if [ -z "$2" ]; then
   if [ -z "$1" ]; then
     eval "$SED"
   else
     eval "$SED" < $1
   fi
 else
   eval "$SED" < $1 > $2
 fi
fi
