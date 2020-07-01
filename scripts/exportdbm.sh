#!/usr/bin/env bash
# Filename: exportdbm.sh
# Export the DBM module.
# $Id: exportdbm.sh,v 1.11 2005/11/04 14:54:47 adavid Exp $

# Initial export

. ./version.sh 0
../scripts/export.sh dbm $DBM_VERSION
HERE=`pwd`

# Temporary directory to unpack & fix

DBMNAME="UPPAAL-dbm-$DBM_VERSION"
TMPDIR=/tmp/$DBMNAME-TMP
rm -rf $TMPDIR
mkdir $TMPDIR

# License header

cd ..
HEADER=`pwd`/scripts/udbm-header.txt

# Unpack

cd ..
TARBALL=`pwd`/$DBMNAME.tgz
cd $TMPDIR
tar zxf $TARBALL

# Fixes

echo "Adding license header..."
for f in `find $DBMNAME \( -name "*.h" -o -name "*.c" -o -name "*.cpp" -o -name "*.y" \)`; do
  cat $HEADER $f > foo
  mv foo $f
done

cd $DBMNAME/modules
./autogen.sh
rm -rf dbm/tests/{xfed,xdbm}.c dbm/{swig,ruby}

cd ..
mv scripts/cflags-udbm.txt scripts/cflags.txt

cd ..
find . -name '.svn' | xargs rm -rf

# Pack & clean

echo "Updating tarball..."
tar zcf $TARBALL $DBMNAME
cd $HERE
rm -rf $TMPDIR
