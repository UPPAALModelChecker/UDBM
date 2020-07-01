#!/usr/bin/env bash
#
# Filename: download.sh
#
# Download, configure, and compile libxml2
# Download mingw
#
# $Id: download.sh,v 1.6 2004/03/02 16:10:10 adavid Exp $

# We need to update this from time to time:

XML=2.6.6
MINGDOWNLOAD=http://heanet.dl.sourceforge.net/sourceforge/mingw/
MINGFILES="binutils-2.13.90-20030111-1.tar.gz gcc-g++-3.3.1-20030804-1.tar.gz w32api-2.4.tar.gz gcc-core-3.3.1-20030804-1.tar.gz mingw-runtime-3.2.tar.gz"

download() {
  if [ ! -f $2 ]; then
    wget --passive-ftp $1$2 
  else
    echo "Skipped downloading $2: file present."
  fi
}

checkbin() {
  if [ -z "`type -path $1`" ]; then
    echo "You need to install wget."
    exit 1
  fi
}

checkbin wget
checkbin tar

if [ "`uname -o 2>/dev/null`" = "Cygwin" ]; then
  echo "*** Download of mingw files ***"
  for f in $MINGFILES; do
    download $MINGDOWNLOAD $f
  done
fi

echo "*** Download, compile, and install of libxml2-$XML ***"

# download and unpack package.
# skip operations whenever possible.

downloadunpack() {
  download $1 $2$3
  if [ ! -d $2 ]; then
    tar zxfv $2$3
  else
    echo "Skipped unpacking $2: directory present."
  fi
}

# prompt for an entry, propose a default value

readval() {
  echo "$1 [$2]:"
  read val
  if [ -z "$val" ]; then
    val=$2
  fi
}

# libxml2

downloadunpack ftp://ftp.gnome.org/pub/GNOME/sources/libxml2/2.6/ libxml2-$XML .tar.gz
cd libxml2-$XML
readval "Path for libxml installation" $HOME/libxml2-$XML
xmlpath=$val

install -d $xmlpath

echo "Configuring libxml2-$XML..."
./configure \
--prefix=$xmlpath \
--disable-dependency-tracking \
--without-html-dir \
--without-zlib \
--without-fexceptions \
--without-python \
--without-threads \
--without-thread-alloc \
--without-history \
--without-ftp \
--without-http \
--without-catalog \
--without-docbook \
--without-xpath \
--without-xptr \
--without-c14n \
--without-xinclude \
--without-iconv \
--without-iso8859x \
--without-schemas \
--without-regexps \
--without-debug \
--without-mem-debug

make
make install
