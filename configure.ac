# Filename: configure.ac.in
#
# Input file to generate configure.ac.
# The @xx@ tags are replaced by proper values.

# Init

AC_INIT([UPPAAL], [4.1.4 (4761)], [bug-uppaal@list.it.uu.se])
AC_CONFIG_AUX_DIR(scripts/)
AC_CONFIG_SRCDIR(version.sh)
AC_SUBST(MODULE_LIBS,[" base.lib dbm.lib debug.lib hash.lib io.lib"])
AC_SUBST(DBM_VERSION,["2.0.8"])
AC_SUBST(TIGA_VERSION,["4.1.4-0.16 (4761)"])

# Set C++ as the language for tests

AC_LANG_CPLUSPLUS

# Detect build and target systems

AC_CANONICAL_HOST
AC_CANONICAL_TARGET

# Get OS. Supported: Cygwin Solaris Linux Darwin
# Cygwin+Solaris ok
# Linux returns GNU/Linux
# Darwin not ok, use uname.
#
# FIXME: Use target_os instead!

AC_MSG_CHECKING([operating system type])
OS=`uname -o 2>/dev/null || uname`
OS=`echo $OS | sed 's:GNU/::g'`
AC_MSG_RESULT([$OS])
AC_SUBST(OS,$OS)
AC_SUBST(LINKFLAGS,$LINKFLAGS)
AC_SUBST(EXTRALIBS,$EXTRALIBS)

# Fix option for boost and merge flags.
CFLAGS="$CFLAGS $CPPFLAGS $CXXFLAGS -DBOOST_DISABLE_THREADS"

# Fix option for libxml under Windows (Cygwin).
if test "$OS" == "Cygwin"; then
  CFLAGS="$CFLAGS -DLIBXML_STATIC"
fi

# Things to generate

AC_CONFIG_HEADERS(include/config.h)
AC_CONFIG_LINKS(.scripts:scripts/)
AC_CONFIG_COMMANDS([lib],[install -d lib])
AC_CONFIG_FILES(makefile:scripts/makefile.in)
AC_CONFIG_FILES(.config:scripts/config.in)
AC_CONFIG_FILES(udbm-config:scripts/udbm-config.in, [chmod +x udbm-config])

# Create symlink from libm.so to /lib/libmso.1 on Solaris such
# that the binaries can be used on Solaris 9 even when compiled
# on Solaris 10.
case "$target_os" in
solaris*)
	AC_CONFIG_COMMANDS([libm],[ln -sf /lib/libm.so.1 lib/libm.so])
	;;
esac

# autogen.sh replaces this with more things to generate

AC_CONFIG_FILES([ base/Makefile:scripts/Makefile.module.in base/tests/Makefile:scripts/Makefile.tests.in dbm/Makefile:scripts/Makefile.module.in dbm/tests/Makefile:scripts/Makefile.tests.in debug/Makefile:scripts/Makefile.module.in debug/tests/Makefile:scripts/Makefile.tests.in hash/Makefile:scripts/Makefile.module.in hash/tests/Makefile:scripts/Makefile.tests.in io/Makefile:scripts/Makefile.module.in io/tests/Makefile:scripts/Makefile.tests.in])

# Check for programs.

AC_PROG_CC
AC_PROG_CXX
AC_PROG_LEX
AC_PROG_YACC
AC_PROG_RANLIB
AC_PROG_INSTALL
AC_PATH_PROG(XML_CONFIG,
             [xml2-config],
	     [AC_MSG_ERROR([cannot find xml2-config])])
AC_PATH_PROG(GPERF, [gperf], [AC_MSG_ERROR([cannot find gperf])])
AC_PATH_PROG(MAKEDEP, [makedepend], [AC_MSG_ERROR([cannot find makedepend])])
AC_PATH_PROG(AR, [ar], [AC_MSG_ERROR([cannot find $AR])])
AC_PATH_PROG(STRIP, [$STRIP], [AC_MSG_ERROR([cannot find $STRIP])])

# Check if xml2-config supports the --static flag
test ! -z "`xml2-config | grep -e --static`" -a ! -z "`echo $LINKFLAGS | grep -e -static`" && XMLSTATIC="--static"
AC_SUBST(XMLSTATIC,$XMLSTATIC)

# Checks for libraries.

# Checks for header files.

AC_HEADER_STDC
AC_CHECK_HEADERS([stdint.h inttypes.h unistd.h errno.h memory.h string.h strings.h sys/wait.h])
AC_CHECK_HEADERS([ext/slist slist sstream strstream])
AC_CHECK_HEADERS([boost/bind.hpp boost/lambda/lambda.hpp boost/lambda/bind.hpp],,
                [AC_MSG_ERROR([boost header files not found])])

# Other customized checks, see acinclude.m4
# One customized check is wrong for icc

UA_CXX_SLIST
UA_CXX_STREAMBUF

# Define INTEL_ARCH on Intel architectures

case $host_cpu in
i?86|x86_64)
	AC_DEFINE(INTEL_ARCH, 1, [Define to 1 when on Intel architecture])
	;;
*)
	AC_DEFINE(INTEL_ARCH, 0)
	;;
esac

# Add version information

case $CXX in
g++*)
	AC_DEFINE_UNQUOTED(COMPILER_STRING,
			   "Compiled with g++-`$CXX -dumpversion | tr -d '\r'` $CFLAGS",
		           [A compiler identification string])
	;;
icc*)
	AC_DEFINE_UNQUOTED(COMPILER_STRING,
			   "Compiled with icc-`$CXX --version` $CFLAGS")
	;;
*)
	AC_DEFINE_UNQUOTED(COMPILER_STRING, "Compiled with $CXX $CFLAGS")
	;;
esac

AC_DEFINE(VERSION, "4.1.4 (4761)", [The UPPAAL version number])
AC_DEFINE(PACKAGE_MONTH, "July", [The release month])
AC_DEFINE(PACKAGE_YEAR, "2020", [The release year])
AC_DEFINE(TIGA_VERSION, "4.1.4-0.16 (4761)", [Tiga version])
AC_DEFINE(DBM_VERSION, "2.0.8", [Version of the DBM library])

# Checks for typedefs, structures, and compiler characteristics.

AC_C_CONST
AC_C_CHAR_UNSIGNED

# Checks for library functions.

AC_CHECK_FUNCS(index)

# Output the crap

AC_OUTPUT
