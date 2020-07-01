#!/usr/bin/env bash
#
# Filename: export.sh
#
# Export a given module.
# Must be executed in the source directory.
# Usage: export.sh module_name
#
# $Id: export.sh,v 1.11 2005/08/04 14:37:15 adavid Exp $

HERE=`pwd`/

if [ -z "$1" ]; then
  echo "Usage: $0 module_name"
  exit 1
fi

prefix=UPPAAL-$1

if [ ! -z "$2" ]; then
  prefix="$prefix-$2"
fi

# Clean-up source

if [ -f makefile ]; then
  make cleanall
fi
rm -f *{,/*,/*/*}{~,.o,cc} ../scripts/*~

# Print 1 name per line
# to call sort

function echodep()
{
    for a in $dep; do
	echo $a
    done
}

# Generate dependencies

function gendeps()
{
    dep=
    for m in $*; do
	mdep=`grep '#include' $m/*.c $m/*.cpp $m/tests/*.c $m/tests/*.cpp include/$m/*.h $m/*.y $m/*.l 2>/dev/null | grep -v '<' | cut -d\" -f2 | cut -d/ -f1 | grep -v '\.'`
	dep="$dep $mdep"
    done
    deps=`echodep | sort -u`
    #echo "$* NEED $deps"
}

# Fixpoint of dependencies

fixdeps=
gendeps $1
while [ "$fixdeps" != "$deps" ]; do
    fixdeps=$deps
    gendeps $deps
done

# Documentation for these modules - current pb with doxygen.

#echo "Generating doc:"
#../scripts/gendoc.sh .. ../documentation/doxygen

# Go back to root and change name

cd ../..
mv server $prefix

# Files to take

function findfiles()
{
    for i in $*; do
        find $i \( -name '.svn' -o -name '.cvsignore' -o -name '*~' \) -prune -o -print
    done
}

function adddir()
{
    if [ -d $1 ]; then
	files="$files `findfiles $1`"
    else
	echo "Warning: $1 not found"
    fi
}

# Files to export

#docindex=$prefix/documentation/doxygen/index.htm
files=""
#files="$docindex"
modulefiles="DBM-LICENSE.TXT README acinclude.m4 setup.sh version.sh autogen.sh"
for f in $modulefiles; do
    files="$files $prefix/modules/$f"
done
files="$files `findfiles $prefix/compilers $prefix/scripts`"

# Limited index (.htm to avoid conflicting with index.html)

#echo "<html><head><title>UPPAAL server documentation</title></head><body><center><h1>UPPAAL Modules</h1></center><ul>" > $docindex

echo "Exported modules for $1:"
for m in $deps; do
    echo "$m"
    adddir $prefix/modules/$m
    adddir $prefix/modules/include/$m
#    adddir $prefix/documentation/doxygen/$m
#    echo "<li><a href=\"$m/html/index.html\">$m</a>" >> $docindex
done
#echo "</ul></body></html>" >> $docindex

#echo "Cleaning documentation"
#for html in `find $prefix/documentation/doxygen -name "*.html"`; do
# echo Cleaning $html
#  sed -i -e "s@$HERE/@@g" $html
#done

# Archives

#echo "Generating $prefix.zip"
#echo "$files" | xargs zip -9 $prefix.zip > /dev/null
echo "Generating $prefix.tgz"
echo "$files" | xargs tar zcf $prefix.tgz

# Restore name

mv $prefix server
