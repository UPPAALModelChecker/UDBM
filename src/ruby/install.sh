#! /usr/bin/env sh

echo "Windows installation for ruby-udbm:"
RUBYHOME=c:/ruby

FILES="UDBM_lib.so udbm-callback.rb udbm-gtk.rb udbm-mdi.rb udbm-sys.rb udbm.rb"
echo "Ruby install [$RUBYHOME]"
read r
if [ ! -z "$r" ]; then
  RUBYHOME="$r"
fi
RUBYLIB="$RUBYHOME/lib/ruby/1.8/i386-mswin32"
echo "Installing $FILES in $RUBYLIB/"
cp $FILES "$RUBYLIB/"
