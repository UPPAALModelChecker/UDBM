#! /usr/bin/env bash

VERSION=0.12
DIR=ruby-udbm-$VERSION-win
ruby extconf-windows.rb
make
rm -rf $DIR
mkdir $DIR $DIR/examples
cp udbm*.rb *.so *.TXT install.sh $DIR/
cp examples/*.rb $DIR/examples/
zip -9 -r $DIR.zip $DIR
rm -rf $DIR

scp $DIR.zip adavid@marge.cs.aau.dk:.public_html/UDBM/ureleases/
