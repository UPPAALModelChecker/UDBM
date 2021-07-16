#!/usr/bin/env ruby
# $Id: extconf.rb,v 1.10 2005/11/05 21:23:08 adavid Exp $

require 'mkmf'

$LIBS << " -ludbm"
$LIBPATH << `udbm-config --dir`.strip+"/lib"
$CPPFLAGS += " " + `udbm-config --inc`.strip + " " + `udbm-config --cflags`.strip
$CFLAGS = "-Wall -O2"

create_makefile("UDBM_lib")

m=IO.readlines("Makefile").each { |l| l.gsub!("$(CC) -shared","$(CXX) -shared") }
File.open("Makefile","w") { |f| f.puts(m)
  [ "udbm.rb", "udbm-gtk.rb", "udbm-callback.rb" , "udbm-mdi.rb" , "udbm-sys.rb" ].each { |rb|
    f.print("install-so: $(RUBYARCHDIR)/#{rb}\n$(RUBYARCHDIR)/#{rb}: ${rb}\n\t$(INSTALL_DATA) #{rb} $(RUBYARCHDIR)\n")
  }
}
