# Skip global definitions if recursive call

ifeq (0,${MAKELEVEL})
export BISON := bison
export LEX   := flex
export GPERF := gperf
export TOPMFLAGS := --no-print-directory
export GFLAGS := -C -E -t -L C++ -c
export SRCDIR := /home/ulrik/CLionProjects/uppaal/server/modules
export DSTDIR := /home/ulrik/CLionProjects/uppaal/server/modules
export CC  := gcc
export CXX := g++
export AR  := 
export STRIP := 
export XML_CONFIG := 
export CFLAGS :=  -DBOOST_DISABLE_THREADS $(XFLAGS)
export CXXFLAGS :=  -DBOOST_DISABLE_THREADS -std=c++17 $(XFLAGS)
endif

# Setup (compile parser, include lexer, generated=.cc)

MODULE     := $(shell basename `pwd`)
MODULESRC  := $(SRCDIR)/$(MODULE)
MODULEDST  := $(DSTDIR)/$(MODULE)
TARGET     := $(DSTDIR)/lib/lib$(MODULE).a
LEXER      := $(wildcard $(MODULESRC)/*.l $(MODULESRC)/*.gperf)
PARSER     := $(wildcard $(MODULESRC)/*.y)
PARSEROBJS := $(patsubst %.y,%.o,$(notdir $(PARSER)))
GENERATED  := $(patsubst %.l,%.cc,$(patsubst %.y,%.cc,$(patsubst %.gperf,%.cc,$(notdir $(LEXER) $(PARSER)))))
IGNORE     := $(shell if [ -f $(MODULESRC)/.ignore ] ; then cat $(MODULESRC)/.ignore; fi)
SOURCES    := $(filter-out $(patsubst %,$(MODULESRC)/%,$(IGNORE)), $(wildcard $(MODULESRC)/*.cpp $(MODULESRC)/*.c))
OBJS       := $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(notdir $(SOURCES)))) $(PARSEROBJS)
INC        := -I$(MODULEDST) -I$(DSTDIR)/include -I$(MODULESRC) -I$(SRCDIR)/include

# Fix flags for MacOSX
DEPCFLAGS  := $(CFLAGS:-isysroot=)
DEPCFLAGS  := $(DEPCFLAGS:/Developer/SDKs/MacOSX10.5.sdk=)

# Cancel the target if there is no object

ifeq (,$(strip $(OBJS)))
TARGET :=
endif

# Local override of definitions

-include $(MODULESRC)/makefile.def

# Common rules

default : $(MODULE_DEPEND) .depend
	@$(MAKE) allok $(TOPMFLAGS)

all : $(MODULE_DEPEND) $(GENERATED) $(TARGET)

allok : $(GENERATED) $(TARGET)

gen : $(MODULE_DEPEND) $(GENERATED)

dep: .depend

#cleandep:
#	@rm -f $(MODULEDST)/.depend

show_cflags :
ifeq (0,${MAKELEVEL})
	@echo "Compiling $(MODULE) using $(CXXFLAGS)"
endif

.depend: show_cflags $(GENERATED) $(SOURCES) makedep

makedep:
#	@echo "makedepend : $(MODULE)/.depend"
	@echo "# DO NOT DELETE" > $(MODULEDST)/.depend
	@makedepend -f- -o.o -- $(DEPCFLAGS) -- -Y $(INC) $(SOURCES) $(PARSER) 2>/dev/null | sed "s:^$(MODULESRC)/::g" > $(MODULEDST)/.depend

.PRECIOUS: $(OBJS) $(GENERATED)

clean :
	@echo "------- clean $(MODULE)"
	@if [ -d tests ]; then $(MAKE) -C tests clean $(TOPMFLAGS); fi
	@rm -f *.o $(GENERATED) $(TARGET) *~ *.pdb *.il *.stackdump *.core core

# Rules for *nix

%.a : $(OBJS)
	@echo "$(AR) rcs: $(notdir $@)"
	@$(AR) rcs $@ $(OBJS)

%.o : $(MODULESRC)/%.cpp
	@echo "$(CXX) $($@-FLAGS): $(MODULE)/$@"
	@$(CXX) -o $@ -c $< $(CXXFLAGS) $($@-FLAGS) $(INC)

%.o : %.cc
	@echo "$(CXX) $($@-FLAGS): $(MODULE)/$@"
	@$(CXX) -o $@ -c $< $(CXXFLAGS) $($@-FLAGS) $(INC)

%.o : $(MODULESRC)/%.c
	@echo "$(CC) $($@-FLAGS): $(MODULE)/$@"
	@$(CC) -o $@ -c $< $(CFLAGS) $($@-FLAGS) $(INC)

# Generic rules to generate source (gperf,lex,bison)

%.cc : $(MODULESRC)/%.gperf
	@echo "$(GPERF) $(GFLAGS) $($@-FLAGS): $(MODULE)/$@"
	@$(GPERF) $(GFLAGS) $($@-FLAGS) $< > $@

%.cc : $(MODULESRC)/%.l
	@echo "$(LEX) $($@-FLAGS): $(MODULE)/$@"
	@$(LEX) $($@-FLAGS) -o$@ $<

%.cc : $(MODULESRC)/%.y
	@echo "$(BISON) -v $($@-FLAGS): $(MODULE)/$@"
	@$(BISON) -v $($@-FLAGS) -o $@ $<

# Rule to respect module dependency

%.rule :
	@if [ -d $(DSTDIR)/$* ]; then \
	 echo "======= make $*"; \
	 $(MAKE) -C $(DSTDIR)/$* $(TOPMFLAGS) MODULE=$*; \
	else echo "Error: no module $*"; exit 1; fi

# Additional rules

-include $(MODULESRC)/makefile.rules

# Dependencies (after make dep)

-include $(MODULEDST)/.depend

