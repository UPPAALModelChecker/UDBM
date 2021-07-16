#! /usr/bin/env ruby
# $Id: extconf-windows.rb,v 1.1 2005/11/05 21:43:04 adavid Exp $

require 'mkmf'

# This is a manually tweaked extconf.rb for
# - mingw32-g++ as the g++ compiler
# - Ruby installed in c:/ruby
# - libudbm compiled and installed in /usr/local/uppaal

# There is a problem with path, so inline the result
# of calls to udbm-config.
$LIBPATH << "c:/cygwin/usr/local/uppaal/lib"
$CFLAGS = "-Wall -O2 " + `udbm-config --cflags`
$CC = "mingw32-gcc"
$CXX = "mingw32-g++"
$DLDFLAGS = ""

# That's the dll used by Ruby and where to get libudbm.a.
$LIBS = "/ruby/bin/msvcrt-ruby18.dll -L/cygwin/usr/local/uppaal/lib -ludbm"

create_makefile("UDBM_lib")

# Fix the crap. Ruby was compiled with cl, which gives
# a number of problems we fix here.
m=IO.readlines("Makefile").each { |l|
 l.gsub!("$(CC) -shared","$(CXX) -shared")
 l.gsub!("$(CLEANFILES)","$(OBJS)")
 l.gsub!("-Tp","-Tp ")
 l.gsub!("-Tc","-Tc ")
 l.gsub!("CPPFLAGS = ","CPPFLAGS = -Ic:/cygwin/usr/local/uppaal/include ")
 l.gsub!("CC = cl -nologo","CC = mingw32-gcc")
 l.gsub!("LDSHARED = cl -nologo -LD","LDSHARED = mingw32-g++ -shared")
 l.gsub!("LIBS = $(LIBRUBYARG_SHARED)","LIBS = ")
 l.gsub!("-Fe","-o ")
 l.gsub!(".obj",".o")
}

# And more fixes.
File.open("Makefile","w") { |f| f.print(m)
  [ "udbm.rb", "udbm-gtk.rb", "udbm-callback.rb" , "udbm-mdi.rb" , "udbm-sys.rb" ].each { |rb|
    f.print("install-so: $(RUBYARCHDIR)/#{rb}\n$(RUBYARCHDIR)/#{rb}: ${rb}\n\t$(INSTALL_DATA) #{rb} $(RUBYARCHDIR)\n")
  }
}
