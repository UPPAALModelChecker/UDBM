require 'mkmf'

# Fix stuff
$LIBS << " -ludbm"
$LIBPATH << `udbm-config --dir`.strip+"/lib"
$CPPFLAGS += " -Wno-unused "+`udbm-config --inc`.strip

# Create Makefile
create_makefile("UDBM")

# Fix more stuff
m=IO.readlines("Makefile").each { |l| l.gsub!("$(CC) -shared","$(CXX) -shared") }
File.open("Makefile","w") { |f| f.puts m }
