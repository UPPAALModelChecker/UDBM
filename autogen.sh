#! /bin/sh

CONFIG=scripts

# List of directories + files in scripts to setup

MLIST=`$CONFIG/getmodules.sh .`
ac="AC_CONFIG_FILES(["
for i in $MLIST; do
  MLIBS="$MLIBS $i.lib"
  ac="$ac $i/Makefile:$CONFIG/Makefile.module.in $i/tests/Makefile:$CONFIG/Makefile.tests.in"
done
ac="$ac])"

# Prepare configure.ac

cat $CONFIG/configure.ac.in | sed -e "s,@MODULE_SETUP@,$ac,g" -e "s,@MODULE_LIST@,$MLIBS,g" | ./version.sh > configure.ac

# Autogen the proper configuration files

aclocal
autoheader
autoconf
