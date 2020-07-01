#!/usr/bin/env bash
#
# Filename: gendoc.sh
#
# Generate documentation, given a directory
# containing several configuration files
#
# $1: source parent directory (contains modules and scripts)
# $2: documentation directory (generated)
#
# $Id: gendoc.sh,v 1.9 2005/07/18 22:01:34 adavid Exp $

#if not absolute path
if [ -z `echo $1 | grep "^/"` ]; then
    there=`pwd`/$1
else
    there=$1
fi

sourcedir=$there/modules
doxyfile=$there/scripts/Doxyfile.in
version=$there/modules/version.sh
getmodules=$there/scripts/getmodules.sh
indexhtml=$there/scripts/index.html.in
doxydir=$2

# echo to stderr
echo_err() { echo $1 1>&2; }

# Work-around: read revision first.
cd "$there/modules" || { echo_err "Cound not enter $there/modules"; exit 1; }
export REVISION=`svn info | grep Revision | sed s/Revision:/rev./g`

test -f "$doxyfile"   || { echo_err "Input $doxyfile not found!"   ; exit 1; }
test -x "$getmodules" || { echo_err "Script getmodules.sh missing!"; exit 1; }
test -x "$version"    || { echo_err "Script version.sh missing!"   ; exit 1; }
install -d $doxydir   || { echo_err "Could not create $doxydir"    ; exit 1; }
cd $doxydir
cp $indexhtml index.html || { echo_err "Could not setup index.html"; exit 1; }
modules=`$getmodules $sourcedir/include`

# Pass 1

for i in $modules; do
    tags="$tags Tags.$i"
done
rm -f $tags
for i in $modules; do
    install -d $i
    tagfiles=`echo $tags | sed "s:\<Tags\.$i\>::g"`
    cat $doxyfile | $version | \
	sed -e "s:@MODULE_NAME@:$i:g" \
	    -e "s:@SOURCE_DIR@:$sourcedir:g" \
	    -e "s:@TAGFILES@:$tagfiles:g" > Doxyfile.$i
    echo "<li><a href=\"$i/html/index.html\">$i</a>" >> index.html
    echo "Pass 1: $i"
    doxygen Doxyfile.$i 2>/dev/null
done
echo "</ul></body></html>" >> index.html

# Pass 2

flag() {
    echo
    echo "******* $1 *******"
    echo
}

echo > doxygen.err
for i in $modules; do
    echo "Pass 2: $i"
    flag $i >> doxygen.err
    doxygen Doxyfile.$i 2>> doxygen.err
done

# Cross reference

fix_links() {
    for html in *.html; do
	cat $html | perl -e 'while(<STDIN>) { if ( $_ =~ /(\<code\>\#include )"(\w+)\/(\w+)\.h"(\<\/code\>\<br\>)/ ) { print "$1\"<a class=\"el\" href=\"../../$2/html/$3_8h-source.html\">$2/$3.h</a>\"$4"; } else { print $_; } }' > $html.tmp
	mv $html.tmp $html
    done
}

# Pass 3

for i in $modules; do
    taglist="$taglist -l Tags.$i@../../$i/html"
done
for i in $modules; do
    if [ -d $i/html ]; then
	echo "Pass 3: $i"
	cd $i/html
	if [ -x $i/html/installdox ]; then
	    ./installdox -q `echo $taglist | sed "s:-l Tags\.$i@\.\./\.\./$i/html::g"`
	fi
	fix_links
	cd ../..
    fi
done
