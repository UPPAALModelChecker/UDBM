#!/usr/bin/env bash
#
# Filename: getmodules.sh
#
# Get all module names from a given
# directory. Ignored: include lib autom4te.cache
# and empty modules.
# It is possible to print with
# a given suffix.
# Usage:
#   getmodules.sh dir [suffix]
#
# $Id: getmodules.sh,v 1.5 2005/07/09 18:37:56 adavid Exp $

# dir=$1
# suffix=$2

if [ ! -z "$1" -a -d "$1" ]; then
  cd $1
  output=
  for i in `ls`; do
    api=`ls $i`
    if [ -d "$i" \
	 -a "$i" != "scripts" \
	 -a "$i" != "include" \
	 -a "$i" != "lib" \
	 -a "$i" != "autom4te.cache" \
	 -a ! -z "$api" ]
    then
      output="$output $i$2"
    fi
  done
  echo $output
fi
