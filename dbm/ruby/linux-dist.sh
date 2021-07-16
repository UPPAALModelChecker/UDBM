#! /usr/bin/env bash

VERSION=0.12
DIR=ruby-udbm-$VERSION
ruby extconf.rb
make
rm -rf $DIR
mkdir $DIR $DIR/examples
cp {extconf,udbm*}.rb *.{cpp,so} GPL-LICENSE.TXT README Makefile $DIR/
cp examples/*.rb $DIR/examples/
tar zcvf $DIR.tgz $DIR
rm -rf $DIR

scp $DIR.tgz adavid@marge.cs.aau.dk:.public_html/UDBM/ureleases/
