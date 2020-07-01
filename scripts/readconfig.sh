#!/usr/bin/env bash
#
# Filename: readconfig.sh
#
# Read configuration information.
# Usage:
#  readconfig.sh configfile pattern
#
# $Id: readconfig.sh,v 1.3 2004/03/02 16:10:10 adavid Exp $

if [ ! -z "$2" -a -e "$1" ]; then
    grep $2 $1 | cut -d: -f2
fi
