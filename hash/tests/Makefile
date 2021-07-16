# Skip global definitions if recursive call

ifeq (0,${MAKELEVEL})
export BISON := bison
export LEX   := flex
export GPERF := gperf
export TOPMFLAGS := --no-print-directory
export GFLAGS := -C -E -t -L C++ -c
export SRCDIR := /home/ulrik/CLionProjects/uppaal/server/modules
export DSTDIR := /home/ulrik/CLionProjects/uppaal/server/modules
export CC     := gcc
export CXX    := g++
export AR     := 
export STRIP  := 
export XML_CONFIG := 
export CFLAGS :=  -DBOOST_DISABLE_THREADS $(XFLAGS)
export CXXFLAGS :=  -DBOOST_DISABLE_THREADS -std=c++17 $(XFLAGS)
export OS     := Linux
export LINKFLAGS := 
export EXTRALIBS := 
export XMLSTATIC := 
export ORIGIN := $$ORIGIN
endif

ifeq (Cygwin,$(OS))
EXE := .exe
endif

ifeq (Msys,$(OS))
EXE := .exe
endif

ifeq (i586-mingw32msvc-gcc,$(CC))
EXE := .exe
endif

ifeq (amd64-mingw32msvc-gcc,$(CC))
EXE := .exe
endif

ifeq (i686-w64-mingw32-gcc,$(CC))
EXE := .exe
endif

ifeq (x86_64-w64-mingw32-gcc,$(CC))
EXE := .exe
endif


# Setup (compile parser, include lexer, generated=.cc)

MODULETEST := $(shell cd .. ; basename `pwd`)/$(shell basename `pwd`)
TESTSRC    := $(SRCDIR)/$(MODULETEST)
TESTDST    := $(DSTDIR)/$(MODULETEST)
LEXER      := $(wildcard $(TESTSRC)/*.l $(TESTSRC)/*.gperf)
PARSER     := $(wildcard $(TESTSRC)/*.y)
PARSEROBJS := $(patsubst %.y,%.o,$(notdir $(PARSER)))
GENERATED  := $(patsubst %.l,%.cc,$(patsubst %.y,%.cc,$(patsubst %.gperf,%.cc,$(notdir $(LEXER) $(PARSER)))))
IGNORE     := $(shell if [ -f $(TESTSRC)/.ignore ] ; then cat $(TESTSRC)/.ignore; fi)
SOURCES    := $(filter-out $(patsubst %,$(TESTSRC)/%,$(IGNORE)), $(wildcard $(TESTSRC)/*.cpp $(TESTSRC)/*.c))
OBJS       := $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(notdir $(SOURCES))))
TARGET     := $(OBJS:.o=$(EXE))
OBJS       += $(PARSEROBJS)
INC    	   := -I$(TESTDST) -I$(DSTDIR)/include -I$(TESTSRC) -I$(SRCDIR)/include
DEPEND     := $(shell cat $(TESTSRC)/lib.dep)
DEPLIBS    := $(DEPEND:--start-group=)
DEPLIBS    := $(DEPLIBS:--end-group=)
LIBS       := $(addprefix $(DSTDIR)/lib/lib, $(addsuffix .a, $(DEPLIBS)))
LIBRULES   := $(addsuffix .rule, $(DEPLIBS))

# Fix flags for MacOSX
DEPCFLAGS  := $(CFLAGS:-isysroot=)
DEPCFLAGS  := $(DEPCFLAGS:/Developer/SDKs/MacOSX10.5.sdk=)

# Link options depend on Cygwin

ifeq (Cygwin,$(OS))
LINKLIBS += -liberty
endif

# Link options depend on icc options

ifeq (icc-ipo,$(CXX)$(findstring -ipo,$(CFLAGS)))
LINKLIBS := $(addprefix $(DSTDIR)/,$(addsuffix /*.o, $(DEPLIBS)))
else
LINKLIBS := -L$(DSTDIR)/lib $(addprefix -l, $(DEPEND))
LINKLIBS := $(LINKLIBS:-l--start-group=--start-group)
LINKLIBS := $(LINKLIBS:-l--end-group=--end-group)
endif

# Link flags depend depend on Mac OS X

ifeq (Darwin,$(OS))
LINKFLAGS += -Wl,-search_paths_first
endif

# Local override of definitions

-include $(TESTSRC)/makefile.def

# Common rules

default :
	@$(MAKE) lib $(TOPMFLAGS) && $(MAKE) targetsonly $(TOPMFLAGS)

all : $(GENERATED) $(LIBRULES) $(OBJS) $(TARGET)

gen : $(GENERATED)

targetsonly : gen $(TARGET)

lib : dep $(LIBRULES)

dep: .depend

show_cflags:
	@echo "Compiling $(MODULETEST) using $(CFLAGS)"

.depend: show_cflags $(GENERATED) $(SOURCES) makedep

makedep:
#	@echo "makedepend $(CFLAGS): $(MODULETEST)/.depend"
	@echo "# DO NOT DELETE" > $(TESTDST)/.depend
	@makedepend -f- -o.o -- $(DEPCFLAGS) -- -Y $(INC) $(SOURCES) $(PARSER) 2>/dev/null | sed "s:^$(TESTSRC)/::g" > $(TESTDST)/.depend

.PRECIOUS: $(OBJS) $(GENERATED)

clean :
	@echo "------- clean $(MODULETEST)"
	@rm -f *.o $(TARGET) $(GENERATED) $(addsuffix .exe,$(TARGET)) *~ *.pdb *.il *.stackdump *.core core

%.rule :
	@if [ -d $(DSTDIR)/$* ]; then \
	 echo "------- make $*"; \
	 $(MAKE) -C $(DSTDIR)/$* $(TOPMFLAGS) MODULE=$*; \
	else echo "Error: no module $*"; exit 1; fi

# Rules for *nix

%.a : $(notdir $*.rule)

%.o : $(TESTSRC)/%.cpp
	@echo "$(CXX) $($@-FLAGS): $(MODULETEST)/$@"
	@$(CXX) -o $@ -c $< $(CXXFLAGS) $($@-FLAGS) $(INC)

%.o : %.cc
	@echo "$(CXX) $($@-FLAGS): $(MODULETEST)/$@"
	@$(CXX) -o $@ -c $< $(CXXFLAGS) $($@-FLAGS) $(INC)

%.o : $(TESTSRC)/%.c
	@echo "$(CC) $($@-FLAGS): $(MODULETEST)/$@"
	@$(CC) -o $@ -c $< $(CFLAGS) $($@-FLAGS) $(INC)

%$(EXE) : %.o $(LIBS)
	@echo "$(CXX) $(LINKFLAGS) $($@-FLAGS) $($@-LDFLAGS): $(MODULETEST)/$@"
	$(CXX) -o $@ $< $(LINKFLAGS) $($@-FLAGS) $(LINKLIBS) $(EXTRALIBS) $($@-LDFLAGS)
ifeq (,$(findstring -g,$(CFLAGS))$(findstring -pg,$(CFLAGS)))
	@echo "$(STRIP) $@"
	@$(STRIP) $@
endif

# Generic rules to generate source (gperf,lex,bison)

%.cc : $(TESTSRC)/%.gperf
	@echo "$(GPERF) $(GFLAGS) $($@-FLAGS): $(MODULETEST)/$@"
	@$(GPERF) $(GFLAGS) $($@-FLAGS) $< > $@

%.cc : $(TESTSRC)/%.l
	@echo "$(LEX) $($@-FLAGS): $(MODULETEST)/$@"
	@$(LEX) $($@-FLAGS) -o$@ $<

%.cc : $(TESTSRC)/%.y
	@echo "$(BISON) -v $($@-FLAGS): $(MODULETEST)/$@"
	@$(BISON) -v $($@-FLAGS) -o $@ $<

# Additional rules

-include $(TESTSRC)/makefile.rules

# Dependencies (after make dep)

-include $(TESTDST)/.depend
