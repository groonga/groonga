AC_PREREQ(2.59)
m4_define([groonga_version], m4_include(base_version))
AC_INIT([groonga], groonga_version, [groonga@razil.jp])
AC_CONFIG_MACRO_DIR([m4])
AM_CONFIG_HEADER(config.h)

GRN_VERSION_MAJOR=`echo groonga_version | cut -d. -f1`
AC_SUBST(GRN_VERSION_MAJOR)
GRN_VERSION_MINOR=`echo groonga_version | cut -d. -f2`
AC_SUBST(GRN_VERSION_MINOR)
GRN_VERSION_MICRO=`echo groonga_version | cut -d. -f3`
AC_SUBST(GRN_VERSION_MICRO)

GRN_VERSION_RC=`echo groonga_version | sed -e 's/\./,/g'`
AC_SUBST(GRN_VERSION_RC)

AM_INIT_AUTOMAKE([foreign tar-pax subdir-objects])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

PACKAGE_LABEL=Groonga
AC_SUBST(PACKAGE_LABEL)
AC_DEFINE_UNQUOTED(PACKAGE_LABEL, ["$PACKAGE_LABEL"], [Label of package])

# for Autoconf 2.60 or earlier.
if test -z "${datarootdir}"; then
   datarootdir="\${prefix}/share"
   AC_SUBST(datarootdir)
fi

# for Autoconf 2.59 or earlier.
if test -z "${docdir}"; then
   docdir="\${datarootdir}/doc/\${PACKAGE_TARNAME}"
   AC_SUBST(docdir)
fi

AC_CANONICAL_HOST
AC_DEFINE_UNQUOTED(HOST_CPU, ["$host_cpu"], [host CPU])
AC_DEFINE_UNQUOTED(HOST_OS, ["$host_os"], [host OS])

AC_MSG_CHECKING([for native Win32])
case "$host_os" in
  mingw*)
    os_win32=yes
    ;;
  *)
    os_win32=no
    ;;
esac
AC_MSG_RESULT([$os_win32])

AC_MSG_CHECKING([for some Win32 platform])
case "$host_os" in
  mingw*|cygwin*)
    platform_win32=yes
    ;;
  *)
    platform_win32=no
    ;;
esac
AC_MSG_RESULT([$platform_win32])

AM_CONDITIONAL(OS_WIN32, test "$os_win32" = "yes")
AM_CONDITIONAL(PLATFORM_WIN32, test "$platform_win32" = "yes")

AC_MSG_CHECKING([for NetBSD.])
case "$host_os" in
  netbsd*)
    netbsd=yes
    ;;
  *)
    netbsd=no
    ;;
esac
AC_MSG_RESULT([$netbsd])

AC_MSG_CHECKING([for Solaris.])
case "$host_os" in
  solaris*)
    solaris=yes
    ;;
  *)
    solaris=no
    ;;
esac
AC_MSG_RESULT([$solaris])

AC_C_BIGENDIAN
AC_PROG_CXX
m4_ifdef([AX_CXX_COMPILE_STDCXX_17],
         [AX_CXX_COMPILE_STDCXX_17([ext], [optional])])
if test "$HAVE_CXX17" != "1"; then
  m4_ifdef([AX_CXX_COMPILE_STDCXX_11],
           [AX_CXX_COMPILE_STDCXX_11([ext], [optional])])
fi
AC_PROG_CC
m4_ifdef([AC_PROG_CC_C99],
         [AC_PROG_CC_C99])
AM_PROG_CC_C_O
m4_ifdef([PKG_PROG_PKG_CONFIG],
         [PKG_PROG_PKG_CONFIG([0.19])
          m4_pattern_allow(PKG_CONFIG_LIBDIR)])

AC_MSG_CHECKING([for clang])
if test "$CC" = "clang"; then
  CLANG=yes
else
  CLANG=no
fi
AC_MSG_RESULT([$CLANG])

AC_DEFUN([CHECK_CFLAG], [
  AC_MSG_CHECKING([if gcc supports $1])
  old_CFLAGS=$CFLAGS
  flag=`echo '$1' | sed -e 's,^-Wno-,-W,'`
  CFLAGS="$CFLAGS $flag -Werror"
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([])],
    [check_cflag=yes],
    [check_cflag=no])
  CFLAGS="$old_CFLAGS"
  if test "x$check_cflag" = "xyes"; then
    CFLAGS="$CFLAGS $1"
  fi
  AC_MSG_RESULT([$check_cflag])
])

AC_DEFUN([CHECK_CXXFLAG], [
  AC_MSG_CHECKING([if g++ supports $1])
  old_CXXFLAGS=$CXXFLAGS
  flag=`echo '$1' | sed -e 's,^-Wno-,-W,'`
  CXXFLAGS="$CXXFLAGS $flag -Werror"
  AC_LANG_PUSH([C++])
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([])],
    [check_cxxflag=yes],
    [check_cxxflag=no])
  AC_LANG_POP([C++])
  CXXFLAGS="$old_CXXFLAGS"
  if test "x$check_cxxflag" = "xyes"; then
    CXXFLAGS="$CXXFLAGS $1"
  fi
  AC_MSG_RESULT([$check_cxxflag])
])

AC_DEFUN([CHECK_BUILD_FLAG], [
  CHECK_CFLAG([$1])
  CHECK_CXXFLAG([$1])
])

AC_DEFUN([REMOVE_CXXFLAG], [
  AC_MSG_CHECKING([whether g++ option $1 is needed to be removed])
  if echo "$CXXFLAGS" | grep -q -- "$1"; then
    CXXFLAGS=`echo "$CXXFLAGS" | sed -e 's,$1,,'`
    remove_cxxflag=yes
  else
    remove_cxxflag=no
  fi
  AC_MSG_RESULT([$remove_cxxflag])
])

TEST_CFLAGS=""
TEST_CXXFLAGS=""
NO_STRICT_ALIASING_CFLAGS=""
NO_FLOAT_EQUAL_CFLAGS=""
NO_BAD_FUNCTION_CAST_CFLAGS=""
if test "$GCC" = "yes"; then
  CHECK_BUILD_FLAG([-Wall])
  if test "x$check_cflag" = "xno"; then
    CHECK_BUILD_FLAG([-W])
  fi
  CHECK_BUILD_FLAG([-Wno-unused-but-set-variable]) # FIXME: enable it.
  CHECK_CFLAG([-Wno-pointer-sign])
  CHECK_CFLAG([-Wno-declaration-after-statement])

  CHECK_BUILD_FLAG([-Wformat])
  CHECK_BUILD_FLAG([-Wno-format-truncation])
  CHECK_BUILD_FLAG([-Wstrict-aliasing=2])
  if test "x$check_cflag" = "xyes"; then
    NO_STRICT_ALIASING_CFLAGS="-fno-strict-aliasing"
  fi
  CHECK_BUILD_FLAG([-Wdisabled-optimization])
  CHECK_BUILD_FLAG([-Wfloat-equal])
  if test "x$check_cflag" = "xyes"; then
    NO_FLOAT_EQUAL_CFLAGS="-Wno-float-equal"
  fi
  CHECK_BUILD_FLAG([-Wpointer-arith])
  CHECK_CFLAG([-Wbad-function-cast])
  if test "x$check_cflag" = "xyes"; then
    NO_BAD_FUNCTION_CAST_CFLAGS="-Wno-bad-function-cast"
  fi
#  if test "$CLANG" = "no"; then
#   CHECK_BUILD_FLAG([-Wcast-align])
#  fi
#  CHECK_BUILD_FLAG([-Wredundant-decls])
#  CHECK_BUILD_FLAG([-Wunsafe-loop-optimizations])
#  CHECK_BUILD_FLAG([-Wunreachable-code])
#  CHECK_BUILD_FLAG([-Wswitch-enum])
#  CHECK_BUILD_FLAG([-Wshadow])
#  CHECK_BUILD_FLAG([-Wconversion])
  CHECK_BUILD_FLAG([-Wwrite-strings])
#  CHECK_BUILD_FLAG([-Winline])
  CHECK_BUILD_FLAG([-Wmissing-field-initializers])
  if test "x$check_cflag" = "xyes"; then
    NO_MISSING_FIELD_INITIALIZES_CFLAGS="-Wno-missing-field-initializers"
  fi
  CHECK_BUILD_FLAG([-Wsign-compare])

  CHECK_CXXFLAG([-fexceptions])
  CHECK_CXXFLAG([-fimplicit-templates])
fi
AC_SUBST(TEST_CFLAGS)
AC_SUBST(TEST_CXXFLAGS)
AC_SUBST(NO_STRICT_ALIASING_CFLAGS)
AC_SUBST(NO_FLOAT_EQUAL_CFLAGS)
AC_SUBST(NO_BAD_FUNCTION_CAST_CFLAGS)
AC_SUBST(NO_MISSING_FIELD_INITIALIZES_CFLAGS)

LT_INIT([dlopen win32-dll])
LT_LANG([Windows Resource])
LT_OUTPUT

LT_CURRENT=0
LT_REVISION=0
LT_AGE=0
LT_VERSION_INFO="\$(LT_CURRENT):\$(LT_REVISION):\$(LT_AGE)"
AC_SUBST(LT_CURRENT)
AC_SUBST(LT_REVISION)
AC_SUBST(LT_AGE)
AC_SUBST(LT_VERSION_INFO)

GRN_DLL_FILENAME="libgroonga-\$(LT_CURRENT).dll"
AC_SUBST(GRN_DLL_FILENAME)

if test "$srcdir/version.sh"; then
  . "$srcdir/version.sh"
  AC_SUBST(GRN_VERSION)
fi

if test "$GCC" = "yes"; then
  AC_DEFINE(_GNU_SOURCE, [1], [Define to 1 if you use GCC.])
fi

if test "$netbsd" = "yes"; then
  AC_DEFINE(_NETBSD_SOURCE, [1], [Define to 1 if you are on NetBSD.])
fi

if test "$solaris" = "yes"; then
  AC_DEFINE(_XPG4_2, [1],
            [Define to 1 for msghdr.msg_control if you are on Solaris.])
  AC_DEFINE(__EXTENSIONS__, [1],
            [Define to 1 for singal.h with _XPG4_2 if you are on Solaris.])
fi

# For debug
AC_ARG_ENABLE(debug,
  [AS_HELP_STRING([--enable-debug],
                  [use debug flags (default=no)])],
  [grn_debug="$enableval"],
  [grn_debug="no"])
if test "x$grn_debug" != "xno"; then
  grn_debug="yes"
  if test "$CLANG" = "yes"; then
    CFLAGS="$CFLAGS -O0 -g"
    CXXFLAGS="$CXXFLAGS -O0 -g"
  elif test "$GCC" = "yes"; then
    CFLAGS="$CFLAGS -O0 -g3"
    CXXFLAGS="$CXXFLAGS -O0 -g3"
  fi
fi
AC_SUBST(grn_debug)

# For optimization
AC_ARG_ENABLE(optimize,
  [AS_HELP_STRING([--enable-optimize],
                  [use optimize flags (default=no)])],
  [grn_optimize="$enableval"],
  [grn_optimize="no"])
if test "x$grn_optimize" != "xno"; then
  grn_optimize="yes"
  if test "$CLANG" = "yes"; then
    CFLAGS="$CFLAGS -O3 -march=native"
    CXXFLAGS="$CXXFLAGS -O3 -march=native"
  elif test "$GCC" = "yes"; then
    CFLAGS="$CFLAGS -O3 -march=native"
    CXXFLAGS="$CXXFLAGS -O3 -march=native"
  fi
fi

AC_ARG_ENABLE(windows-back-trace,
  [AS_HELP_STRING([--enable-windows-back-trace],
                  [enable back trace on Windows (default=yes)])],
  [grn_with_windows_back_trace="$enableval"],
  [grn_with_windows_back_trace="yes"])
if test "x$grn_with_windows_back_trace" = "yes"; then
  AC_DEFINE(GRN_WITH_WINDOWS_BACK_TRACE, [1], [enable back trace on Windows])
fi

AC_SEARCH_LIBS(log, m, [], [AC_MSG_ERROR("No libm found")])
AC_MSG_CHECKING([for fpclassify])
AC_LINK_IFELSE(
  [AC_LANG_PROGRAM(
    [#include <math.h>],
    [if (fpclassify(0.0)) {return 0;}]
   )],
  [
    AC_DEFINE(HAVE_FPCLASSIFY, [1], [use fpclassify])
    AC_MSG_RESULT(yes)
  ],
  [
    AC_LINK_IFELSE(
      [AC_LANG_PROGRAM(
        [#define _ISOC99_SOURCE
         #include <math.h>],
        [if (fpclassify(0.0)) {return 0;}]
       )],
      [
        AC_DEFINE(_ISOC99_SOURCE, [1], [Define to 1 for fpclassify])
        AC_DEFINE(HAVE_FPCLASSIFY, [1], [use fpclassify with _ISOC99_SOURCE])
        AC_MSG_RESULT(yes)
      ],
      [
        AC_MSG_RESULT(no)
      ])
  ])

m4_include(build/ac_macros/check_headers.m4)
m4_include(build/ac_macros/check_functions.m4)

AC_SEARCH_LIBS(backtrace, execinfo,
               [AC_DEFINE(HAVE_BACKTRACE, [1],
                          [Define to 1 if you have the `backtrace' function.])])
AC_SEARCH_LIBS(clock_gettime, rt,
               [AC_DEFINE(HAVE_CLOCK_GETTIME, [1], [use clock_gettime])])
AC_SYS_LARGEFILE
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_CHECK_SIZEOF(off_t)
AC_CHECK_MEMBERS(
  [struct tm.tm_gmtoff],
  ,
  ,
  [
#include <time.h>
  ])

# MAP_HUGETLB
AC_ARG_ENABLE(map-hugetlb,
  [AS_HELP_STRING([--enable-map-hugetlb],
                  [use MAP_HUGETLB. [default=no]])],
  ,
  [enable_map_hugetlb="no"])
if test "x$enable_map_hugetlb" != "xno"; then
  AC_MSG_CHECKING([for MAP_HUGETLB])
  AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM(
       [
#ifdef HAVE_SYS_MMAN_H
#  include <sys/mman.h>
#endif /* HAVE_SYS_MMAN_H */
       ],
       [MAP_HUGETLB;]
     )],
    [
     AC_DEFINE(USE_MAP_HUGETLB, [1], [use MAP_HUGETLB])
     AC_MSG_RESULT(yes)
    ],
    [
     AC_MSG_RESULT(no)
     AC_MSG_ERROR("MAP_HUGETLB isn't available.")
    ]
  )
fi

# log path
AC_ARG_WITH(log_path,
  [AS_HELP_STRING([--with-log-path=PATH],
    [specify groonga log path.])],
  grn_log_path="$withval",
  grn_log_path="\$(localstatedir)/log/\$(PACKAGE_NAME)/\$(PACKAGE_NAME).log")
AC_SUBST(grn_log_path)

# default encoding
AC_ARG_WITH(default_encoding,
  [AS_HELP_STRING([--with-default-encoding=ENCODING],
    [specify groonga default encoding(euc_jp/sjis/utf8/latin1/koi8r/none)])],
  GRN_DEFAULT_ENCODING="$withval",
  GRN_DEFAULT_ENCODING="utf8")
AC_DEFINE_UNQUOTED(GRN_DEFAULT_ENCODING, "$GRN_DEFAULT_ENCODING", "specified default encoding")

# default match escalation threshold
AC_ARG_WITH(match_escalation_threshold,
  [AS_HELP_STRING([--with-match-escalation-threshold=NUMBER],
    [specify groonga default match escalation threshold])],
  GRN_DEFAULT_MATCH_ESCALATION_THRESHOLD="$withval",
  GRN_DEFAULT_MATCH_ESCALATION_THRESHOLD="0")
AC_DEFINE_UNQUOTED(GRN_DEFAULT_MATCH_ESCALATION_THRESHOLD, $GRN_DEFAULT_MATCH_ESCALATION_THRESHOLD, "specified match escalation threshold")

# default DB key management algorithm
AC_ARG_WITH(default_db_key,
  [AS_HELP_STRING([--with-default-db-key=ALGORITHM],
    [specify groonga default DB key (pat/dat/auto)])],
  GRN_DEFAULT_DB_KEY="$withval",
  GRN_DEFAULT_DB_KEY="auto")
AC_DEFINE_UNQUOTED(GRN_DEFAULT_DB_KEY, "$GRN_DEFAULT_DB_KEY",
                   "specified default DB key management algorithm")

# DANGER!!!: stack size
GRN_STACK_SIZE=1024
AC_ARG_WITH(stack_size,
  [AS_HELP_STRING([--with-stack-size=SIZE],
    [DANGER!!!
     This option specifies stack size. (default=$GRN_STACK_SIZE)
     Normally, you should not use this option.])],
  GRN_STACK_SIZE="$withval")
AC_DEFINE_UNQUOTED(GRN_STACK_SIZE, [$GRN_STACK_SIZE], [stack size])

# lock timeout
GRN_LOCK_TIMEOUT=900000
AC_ARG_WITH(lock_timeout,
  [AS_HELP_STRING([--with-lock-timeout=N],
    [This option specifies how many times Groonga tries to acquire a lock.
     Each try waits --with-lock-wait-time nanoseconds to acquire a lock.
     It means that Groonga gives up after
     (--with-lock-wait-time * --with-lock-timeout) nanoseconds.
     (default=$GRN_LOCK_TIMEOUT)])],
  GRN_LOCK_TIMEOUT="$withval")
AC_DEFINE_UNQUOTED(GRN_LOCK_TIMEOUT,
                   [$GRN_LOCK_TIMEOUT],
                   [lock timeout])

# lock wait time
GRN_LOCK_WAIT_TIME_NANOSECOND=1000000
AC_ARG_WITH(lock_wait_time,
  [AS_HELP_STRING([--with-lock-wait-time=NANOSECONDS],
    [This option specifies wait time in nanosecond to acquire a lock.
     (default=$GRN_LOCK_WAIT_TIME_NANOSECOND)])],
  GRN_LOCK_WAIT_TIME_NANOSECOND="$withval")
AC_DEFINE_UNQUOTED(GRN_LOCK_WAIT_TIME_NANOSECOND,
                   [$GRN_LOCK_WAIT_TIME_NANOSECOND],
                   [lock wait time in nanosecond])

if test "$os_win32" != "yes"; then
  AC_CHECK_HEADERS(pthread.h)
  AC_SEARCH_LIBS(pthread_create, pthread,
                 [],
                 [AC_MSG_ERROR("No libpthread found")])
  AC_CHECK_FUNCS(pthread_mutexattr_setpshared)
  AC_CHECK_FUNCS(pthread_condattr_setpshared)
fi
AC_SEARCH_LIBS(gethostbyname, nsl)
AC_SEARCH_LIBS(socket, socket)
AC_SEARCH_LIBS(dlopen, dl)

# nfkc
AC_ARG_ENABLE(nfkc,
  [AS_HELP_STRING([--enable-nfkc],
    [use nfkc based utf8 normalization. [default=yes]])],,
  [enable_nfkc="yes"])
if test "x$enable_nfkc" = "xyes"; then
  AC_DEFINE(GRN_WITH_NFKC, [1], [compile with nfkc.c])
fi

# coverage
m4_ifdef([AC_CHECK_COVERAGE], [AC_CHECK_COVERAGE])
GENHTML_OPTIONS="--title 'groonga Code Coverage'"

# microyield
AC_MSG_CHECKING([whether enable uyield])
AC_ARG_ENABLE(uyield,
  [AS_HELP_STRING([--enable-uyield],
    [build for detecting race conditions. [default=no]])],
  ,
  [enable_uyield="no"])
AC_MSG_RESULT($enable_uyield)

# exact-alloc-count
AC_MSG_CHECKING([whether enable exact-alloc-count])
AC_ARG_ENABLE(exact-alloc-count,
  [AS_HELP_STRING([--enable-exact-alloc-count],
    [atomic counting for memory alloc count. [default=yes]])],,
  [enable_exact_alloc_count="yes"])
if test "x$enable_exact_alloc_count" != "xno"; then
  AC_DEFINE(USE_EXACT_ALLOC_COUNT, [1], [alloc_count with atomic])
fi
AC_MSG_RESULT($enable_exact_alloc_count)

# memory debug
AC_MSG_CHECKING([whether debug memory management])
AC_ARG_ENABLE(memory-debug,
  [AS_HELP_STRING([--enable-memory-debug],
    [debug memory management. [default=no]])],
  ,
  [enable_memory_debug="no"])
AC_MSG_RESULT([$enable_memory_debug])

if test "x$enable_memory_debug" = "xyes"; then
  AC_DEFINE(USE_MEMORY_DEBUG, [1],
            [Define to 1 if you enable debuging memory management])
fi

# epoll/kqueue/poll/select check
AC_CHECK_HEADER(sys/epoll.h, [
  AC_CHECK_FUNC(epoll_create, [
    AC_TRY_RUN([
#include <sys/epoll.h>
int main(int argc, char **argv) { return (epoll_create(16) < 0); }
    ],
    [
      have_epoll="yes"
      AC_DEFINE(USE_EPOLL, [1], [use epoll])
    ]
    )
  ])
])

if test "x$have_epoll" != "xyes"; then
  AC_CHECK_HEADER(sys/event.h, [
    AC_CHECK_FUNC(kevent, [
      have_kqueue="yes"
      AC_DEFINE(USE_KQUEUE, [1], [use kqueue])
    ])
  ])
  if test "x$have_kqueue" != "xyes"; then
    AC_CHECK_HEADER(poll.h, [
      AC_CHECK_FUNC(poll, [
        have_poll="yes"
        AC_DEFINE(USE_POLL, [1], [use poll])
      ])
    ])
    if test "x$have_poll" != "xyes"; then
      if test "$os_win32" = "yes"; then
        AC_CHECK_HEADER(winsock2.h, [have_select="yes"])
      else
        AC_CHECK_FUNC(select, [
          have_select="yes"
          AC_CHECK_HEADERS(sys/select.h)
        ])
      fi
      if test "x$have_select" = "xyes"; then
        AC_DEFINE(USE_SELECT, [1], [use select])
      else
        AC_MSG_ERROR([epoll/kqueue/poll/select is missing.])
      fi
    fi
  fi
fi

# check MSG_MORE defined
AC_MSG_CHECKING([whether MSG_MORE defined])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([
#include <sys/types.h>
#include <sys/socket.h>

int main(int argc, char **argv)
{
  return MSG_MORE;
}
    ])],
    [
      AC_MSG_RESULT(yes)
      AC_DEFINE(USE_MSG_MORE, [1], [use MSG_MORE])
    ],
    [
      AC_MSG_RESULT(no)
    ])

# check MSG_NOSIGNAL defined
AC_MSG_CHECKING([whether MSG_NOSIGNAL defined])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([
#include <sys/types.h>
#include <sys/socket.h>

int main(int argc, char **argv)
{
  return MSG_NOSIGNAL;
}
    ])],
    [
      AC_MSG_RESULT(yes)
      AC_DEFINE(USE_MSG_NOSIGNAL, [1], [use MSG_NOSIGNAL])
    ],
    [
      AC_MSG_RESULT(no)
    ])

# MinGW
if test "$os_win32" = "yes"; then
  WINDOWS_LDFLAGS="-mwindows"
  WINDOWS_LIBS="-ladvapi32 -lws2_32 -ldbghelp -lpsapi"
else
  WINDOWS_LDFLAGS=
  WINDOWS_LIBS=
fi
AC_SUBST(WINDOWS_LDFLAGS)
AC_SUBST(WINDOWS_LIBS)

# groonga binary path
GROONGA="${ac_pwd}/src/groonga"
AC_SUBST(GROONGA)

# groonga-benchmark binary path
GROONGA_BENCHMARK="${ac_pwd}/src/groonga-benchmark"
AC_SUBST(GROONGA_BENCHMARK)

# groonga-suggest-create-dataset binary path
GROONGA_SUGGEST_CREATE_DATASET="${ac_pwd}/src/suggest/groonga-suggest-create-dataset"
AC_SUBST(GROONGA_SUGGEST_CREATE_DATASET)

# groonga-mruby binary path
GROONGA_MRUBY="${ac_pwd}/src/groonga-mruby"
AC_SUBST(GROONGA_MRUBY)

# check Cutter with GLib support if available
REQUIRED_MINIMUM_CUTTER_VERSION=1.1.6
REQUIRED_MINIMUM_CPPCUTTER_VERSION=1.2.0
m4_ifdef([AC_CHECK_GCUTTER],
         [AC_CHECK_GCUTTER(>= $REQUIRED_MINIMUM_CUTTER_VERSION)],
         [cutter_use_cutter="no"])
m4_ifdef([AC_CHECK_CPPCUTTER],
         [AC_CHECK_CPPCUTTER(>= $REQUIRED_MINIMUM_CPPCUTTER_VERSION)],
         [cutter_use_cppcutter="no"])

AM_CONDITIONAL([WITH_CUTTER], [test "$cutter_use_cutter" = "yes"])
AM_CONDITIONAL([WITH_CPPCUTTER], [test "$cutter_use_cppcutter" = "yes"])
if test "$cutter_use_cutter" = "yes"; then
  AC_DEFINE(GRN_WITH_CUTTER, 1, [Define to 1 if you use Cutter])
fi

# check for benchmark
AC_ARG_ENABLE(benchmark,
  [AS_HELP_STRING([--disable-benchmark],
    [don't build benchmark programs.])],,
  [enable_benchmark="yes"])
if test "x$enable_benchmark" = "xno"; then
  benchmark_available="no"
else
  REQUIRED_GLIB_VERSION=2.14.0
  m4_ifdef([AM_PATH_GLIB_2_0],
           [AM_PATH_GLIB_2_0($REQUIRED_GLIB_VERSION,
                             [benchmark_available="yes"],
                             [benchmark_available="no"],
                             [gobject gthread])],
           [benchmark_available="no"])
  AC_MSG_CHECKING(for benchmark availablity)
  AC_MSG_RESULT($ac_benchmark_available)
fi
if test "$benchmark_available" = "yes"; then
  AC_DEFINE(GRN_WITH_BENCHMARK, 1, [Define to 1 if benchamrk is available])
fi
AM_CONDITIONAL([WITH_BENCHMARK], [test "$benchmark_available" = "yes"])

# check Ruby for HTTP test
ac_cv_ruby_available="no"
AC_ARG_WITH([ruby],
            AS_HELP_STRING([--with-ruby=PATH],
                           [Ruby interpreter path (default: no)]),
            [RUBY="$withval"],
            [RUBY="no"])

if test "x$RUBY" = "xno"; then
  RUBY=
else
  if test "x$RUBY" = "xyes"; then
    AC_PATH_PROGS(RUBY,
                  [ dnl
                    ruby3.0 ruby30 dnl
                    ruby2.7 ruby27 dnl
                    ruby2.6 ruby26 dnl
                    ruby2.5 ruby25 dnl
                    ruby2.4 ruby24 dnl
                    ruby2.3 ruby23 dnl
                    ruby2.2 ruby22 dnl
                    ruby2.1 ruby21 dnl
                    ruby dnl
                  ],
                  ruby-not-found)
    if test "$RUBY" != "ruby-not-found"; then
      ruby_version="`$RUBY --version`"
      if echo "$ruby_version" | grep -q -- 'ruby \(2\|3\)\.'; then
        ac_cv_ruby_available="yes"
      else
        AC_MSG_WARN([$RUBY isn't Ruby 2.0 or later ($ruby_version)])
      fi
    fi
  else
    ruby_not_found_warning_message="$RUBY is not found."
    case "$RUBY" in
    /*)
      AC_CHECK_FILE([$RUBY],
                    [ac_cv_ruby_available="yes"],
                    [AC_MSG_WARN([$ruby_not_found_warning_message])
                     RUBY="$RUBY-not-found"])
      ;;
    *)
      ruby_not_found="$RUBY-not-found"
      AC_PATH_PROGS(RUBY, "$RUBY", "$ruby_not_found")
      if test "$RUBY" = "$ruby_not_found"; then
        AC_MSG_WARN([$ruby_not_found_warning_message])
      else
        ac_cv_ruby_available="yes"
      fi
      ;;
    esac
  fi
fi
AC_SUBST(RUBY)
AM_CONDITIONAL([WITH_RUBY], [test "$ac_cv_ruby_available" = "yes"])

AM_CONDITIONAL([WITH_UNIT_TEST],
               [test "$cutter_use_cutter" = "yes" -o \
                     "$ac_cv_ruby_available" = "yes"])

AM_CONDITIONAL([WITH_COMMAND_TEST],
               [test "$ac_cv_ruby_available" = "yes"])

# check Lemon for generating .c and .h files from .y file
lemon_available="no"
AC_ARG_WITH([lemon],
            AS_HELP_STRING([--with-lemon=PATH],
                           [Lemon path (default: auto)]),
            [LEMON="$withval"],
            [: ${LEMON:=auto}])

if test "$LEMON" = "no"; then
  LEMON=
else
  if test "$LEMON" = "auto"; then
    AC_PATH_PROGS(LEMON, [lemon], none)
    if test "$LEMON" != "none"; then
      lemon_available="yes"
    fi
  elif test "$LEMON" = "yes"; then
    AC_PATH_PROGS(LEMON, [lemon], none)
    if test "$LEMON" = "none"; then
      AC_MSG_WARN([lemon is not found. Disable .y compilation.])
    else
      lemon_available="yes"
    fi
  else
    AC_CHECK_FILE([$LEMON],
                  [lemon_available="yes"],
                  [AC_MSG_WARN([$LEMON is not found. Disable .y compilation.])])
  fi
fi
AC_SUBST(LEMON)
AM_CONDITIONAL([WITH_LEMON], [test "$lemon_available" = "yes"])

# libedit
libedit_available="no"
AC_ARG_WITH([libedit],
            AS_HELP_STRING([--with-libedit],
                           [Support line editing (default: auto)]),
            [with_libedit="$withval"],
            [: ${with_libedit:=auto}])

if test "${with_libedit}" = "no"; then
  LIBEDIT_CFLAGS=
  LIBEDIT_LIBS=
else
  if test "${with_libedit}" = "yes" -o "${with_libedit}" = "auto"; then
    LIBEDIT_CFLAGS=
    LIBEDIT_LIBS=
    m4_ifdef([PKG_CHECK_MODULES], [
      PKG_CHECK_MODULES([LIBEDIT],
			[libedit >= 3.0],
			[],
			[LIBEDIT_LIBS="-ledit"])
    ])
  else
    LIBEDIT_CFLAGS="-I${with_libedit}/include"
    LIBEDIT_LIBS="-L${with_libedit}/lib -ledit"
  fi
  CFLAGS_SAVE="${CFLAGS}"
  LDFLAGS_SAVE="${LDFLAGS}"
  CFLAGS="${LIBEDIT_CFLAGS} ${CFLAGS}"
  LDFLAGS="${LIBEDIT_LIBS} ${LDFLAGS}"
  AC_SEARCH_LIBS(el_wline,
                 ,
                 [libedit_available="yes"],
                 [libedit_available="no"])
  if test "${libedit_available}" = "yes"; then
    AC_CHECK_HEADERS(histedit.h,
                     [],
                     [libedit_available="no"])
  fi
  CFLAGS="${CFLAGS_SAVE}"
  LDFLAGS="${LDFLAGS_SAVE}"
  if test "${libedit_available}" = "yes"; then
    AC_DEFINE(GRN_WITH_LIBEDIT, 1, [Define to 1 if libedit works])
  else
    LIBEDIT_CFLAGS=
    LIBEDIT_LIBS=
    if test "${with_libedit}" != "auto"; then
      AC_MSG_ERROR("No libedit found")
    fi
  fi
fi
AC_SUBST(LIBEDIT_CFLAGS)
AC_SUBST(LIBEDIT_LIBS)

# zlib
AC_ARG_WITH(zlib,
  [AS_HELP_STRING([--with-zlib],
    [Support data compression by zlib. [default=auto]])],
  [with_zlib="$withval"],
  [with_zlib="auto"])
GRN_WITH_ZLIB=no
if test "x$with_zlib" = "xno"; then
  ZLIB_CFLAGS=
  ZLIB_LIBS=
  ZLIB_LIB_DIR=
else
  if test "${with_zlib}" = "yes" -o "${with_zlib}" = "auto"; then
    ZLIB_CFLAGS=
    ZLIB_LIBS=
    m4_ifdef([PKG_CHECK_MODULES], [
      PKG_CHECK_MODULES([ZLIB],
			[zlib],
			[GRN_WITH_ZLIB=yes],
			[GRN_WITH_ZLIB=no])
      if test "${GRN_WITH_ZLIB}" = "yes"; then
        _PKG_CONFIG(ZLIB_LIB_DIR, [variable=libdir], [zlib])
        ZLIB_LIB_DIR="${pkg_cv_ZLIB_LIB_DIR}"
      fi
    ],
    [
      CFLAGS_SAVE="${CFLAGS}"
      LDFLAGS_SAVE="${LDFLAGS}"
      AC_SEARCH_LIBS(compress, z,
                     [
                       GRN_WITH_ZLIB=yes
                       ZLIB_LIBS=-lz
		       # TODO
                       ZLIB_LIB_DIR=/usr/lib
                     ],
                     [GRN_WITH_ZLIB=no])
      CFLAGS="${CFLAGS_SAVE}"
      LDFLAGS="${LDFLAGS_SAVE}"
    ])
  else
    GRN_WITH_ZLIB=yes
    ZLIB_CFLAGS="-I${with_zlib}/include"
    ZLIB_LIB_DIR="${with_zlib}/lib"
    ZLIB_LIBS="-L${ZLIB_LIB_DIR} -lz"
  fi
  if test "${GRN_WITH_ZLIB}" = "yes"; then
    AC_DEFINE(GRN_WITH_ZLIB, [1],
              [Support data compression by zlib.])
  else
    if test "${with_zlib}" = "yes"; then
      AC_MSG_ERROR("No zlib found")
    fi
  fi
fi
AC_SUBST(ZLIB_CFLAGS)
AC_SUBST(ZLIB_LIBS)
AC_SUBST(ZLIB_LIB_DIR)
AC_SUBST(GRN_WITH_ZLIB)

# LZ4
AC_ARG_WITH(lz4,
  [AS_HELP_STRING([--with-lz4],
    [Support data compression by LZ4. [default=auto]])],
  [with_lz4="$withval"],
  [with_lz4="auto"])
if test "x$with_lz4" != "xno"; then
  m4_ifdef([PKG_CHECK_MODULES], [
    PKG_CHECK_MODULES([LIBLZ4],
                      [liblz4],
                      [GRN_WITH_LZ4=yes],
                      [GRN_WITH_LZ4=no])
  ],
  [GRN_WITH_LZ4=no])
  if test "$GRN_WITH_LZ4" = "yes"; then
    AC_DEFINE(GRN_WITH_LZ4, [1],
              [Support data compression by LZ4.])
  else
    if test "x$with_lz4" != "xauto"; then
      AC_MSG_ERROR("No liblz4 found")
    fi
  fi
else
  GRN_WITH_LZ4=no
fi

# Zstandard
AC_ARG_WITH(zstd,
  [AS_HELP_STRING([--with-zstd],
    [Support data compression by Zstandard. [default=auto]])],
  [with_zstd="$withval"],
  [with_zstd="auto"])
if test "x$with_zstd" != "xno"; then
  m4_ifdef([PKG_CHECK_MODULES], [
    PKG_CHECK_MODULES([LIBZSTD],
                      [libzstd],
                      [GRN_WITH_ZSTD=yes],
                      [GRN_WITH_ZSTD=no])
  ],
  [GRN_WITH_ZSTD=no])
  if test "$GRN_WITH_ZSTD" = "yes"; then
    AC_DEFINE(GRN_WITH_ZSTD, [1],
              [Support data compression by Zstandard.])
  else
    if test "x$with_zstd" != "xauto"; then
      AC_MSG_ERROR("No libzstd found")
    fi
  fi
else
  GRN_WITH_ZSTD=no
fi

# LuaJIT
AC_ARG_WITH(luajit,
  [AS_HELP_STRING([--with-luajit],
    [Support LuaJIT. [default=auto]])],
  [with_luajit="$withval"],
  [with_luajit="auto"])
if test "x$with_luajit" != "xno"; then
  m4_ifdef([PKG_CHECK_MODULES], [
    PKG_CHECK_MODULES([LUAJIT],
                      [luajit],
                      [GRN_WITH_LUAJIT=yes],
                      [GRN_WITH_LUAJIT=no])
    ],
    [GRN_WITH_LUAJIT=no])
  if test "$GRN_WITH_LUAJIT" = "yes"; then
    AC_DEFINE(GRN_WITH_LUAJIT, [1],
              [Support LuaJIT.])
  else
    if test "x$with_luajit" != "xauto"; then
      AC_MSG_ERROR("No LuaJIT found")
    fi
  fi
else
  GRN_WITH_LUAJIT=no
fi
AC_SUBST(LUAJIT_CFLAGS)
AC_SUBST(LUAJIT_LIBS)

# jemalloc
AC_ARG_WITH(jemalloc,
  [AS_HELP_STRING([--with-jemalloc],
    [Use jemalloc for memory allocation. [default=no]])],
  [with_jemalloc="$withval"],
  [with_jemalloc="no"])
jemalloc_available="no"
if test "x$with_jemalloc" != "xno"; then
  if test "x$with_jemalloc" != "xyes"; then
    LDFLAGS="-L$with_jemalloc $LDFLAGS"
  fi
  AC_SEARCH_LIBS(malloc_conf, jemalloc,
                 [jemalloc_available="yes"],
                 [AC_MSG_ERROR("No libjemalloc found")])
fi

# Apache Arrow
AC_ARG_ENABLE(apache-arrow,
  [AS_HELP_STRING([--disable-apache-arrow],
    [disable Apache Arrow support. [default=auto-detect]])],
  [enable_apache_arrow="$enableval"],
  [enable_apache_arrow="auto"])
if test "x$enable_apache_arrow" != "xno"; then
  m4_ifdef([PKG_CHECK_MODULES], [
    PKG_CHECK_MODULES([APACHE_ARROW],
                      [arrow >= 3.0.0],
                      [GRN_WITH_APACHE_ARROW=yes],
                      [GRN_WITH_APACHE_ARROW=no])
  ],
  [GRN_WITH_APACHE_ARROW=no])
  if test "x$GRN_WITH_APACHE_ARROW" = "xyes"; then
    AC_DEFINE(GRN_WITH_APACHE_ARROW, [1], [Enable Apache Arrow support.])
  else
    if test "x$enable_apache_arrow" = "xyes"; then
      AC_MSG_ERROR("No Apache Arrow found")
    fi
  fi
else
  GRN_WITH_APACHE_ARROW=no
fi
AM_CONDITIONAL([GRN_WITH_APACHE_ARROW], [test "$GRN_WITH_APACHE_ARROW" = "yes"])

# RapidJSON
BUNDLED_RAPIDJSON_VERSION=m4_include([bundled_rapidjson_version])
AC_SUBST(BUNDLED_RAPIDJSON_VERSION)

RAPIDJSON_CFLAGS=
AC_ARG_WITH(rapidjson,
  [AS_HELP_STRING([--with-rapidjson],
    [Use RapidJSON. [default=auto]])],
  [with_rapidjson="$withval"],
  [with_rapidjson="auto"])
if test "x$with_rapidjson" != "xno"; then
  if test "x$with_rapidjson" = "xbundled"; then
    RAPIDJSON_CFLAGS="-I\$(top_srcdir)/vendor/rapidjson-${BUNDLED_RAPIDJSON_VERSION}/include"
    GRN_WITH_RAPIDJSON=yes
  else
    m4_ifdef([PKG_CHECK_MODULES], [
      PKG_CHECK_MODULES([RAPIDJSON],
			[RapidJSON],
			[GRN_WITH_RAPIDJSON=yes],
			[GRN_WITH_RAPIDJSON=no])
      ],
      [GRN_WITH_RAPIDJSON=no])
    if test "$GRN_WITH_RAPIDJSON" = "no"; then
      AC_LANG_PUSH([C++])
      AC_CHECK_HEADER(rapidjson/rapidjson.h,
		      [GRN_WITH_RAPIDJSON=yes],
		      [GRN_WITH_RAPIDJSON=no])
      AC_LANG_POP([C++])
    fi
  fi
  if test "$GRN_WITH_RAPIDJSON" = "yes"; then
    AC_DEFINE(GRN_WITH_RAPIDJSON, [1],
              [Support RapidJSON.])
  else
    if test "x$with_rapidjson" != "xauto"; then
      AC_MSG_ERROR("No RapidJSON found")
    fi
  fi
else
  GRN_WITH_RAPIDJSON=no
fi
AC_SUBST(RAPIDJSON_CFLAGS)

# xxHash
AC_ARG_WITH(xxhash,
  [AS_HELP_STRING([--with-xxhash],
    [Support drilldown by hashed key with xxHash. [default=auto]])],
  [with_xxhash="$withval"],
  [with_xxhash="auto"])
if test "x$with_xxhash" != "xno"; then
  m4_ifdef([PKG_CHECK_MODULES], [
    PKG_CHECK_MODULES([LIBXXHASH],
                      [libxxhash >= 0.8.0],
                      [GRN_WITH_XXHASH=yes],
                      [GRN_WITH_XXHASH=no])
    if test "$GRN_WITH_XXHASH" = "no"; then
      PKG_CHECK_MODULES([LIBXXHASH],
                        [libxxhash],
                        [GRN_WITH_XXHASH=yes
                         LIBXXHASH_CFLAGS="${LIBXXHASH_CFLAGS} -DXXH_INLINE_ALL"],
                        [GRN_WITH_XXHASH=no])
    fi
  ],
  [GRN_WITH_XXHASH=no])
  if test "$GRN_WITH_XXHASH" = "yes"; then
    AC_DEFINE(GRN_WITH_XXHASH, [1],
              [Support drilldown by hashed key with xxHash.])
  else
    if test "x$with_xxhash" != "xauto"; then
      AC_MSG_ERROR("No libxxhash found")
    fi
  fi
else
  GRN_WITH_XXHASH=no
fi

# MeCab
# NOTE: MUST be checked last

BUNDLED_MECAB_VERSION=m4_include([bundled_mecab_version])
AC_SUBST(BUNDLED_MECAB_VERSION)
BUNDLED_MECAB_NAIST_JDIC_VERSION=m4_include([bundled_mecab_naist_jdic_version])
AC_SUBST(BUNDLED_MECAB_NAIST_JDIC_VERSION)

AC_ARG_WITH(mecab,
  [AS_HELP_STRING([--with-mecab],
    [use MeCab for morphological analysis. [default=yes]])],
  [with_mecab="$withval"],
  [with_mecab="yes"])
AC_MSG_CHECKING([whether enable MeCab])
AC_MSG_RESULT($with_mecab)
if test "x$with_mecab" = "xyes"; then
  # mecab-config
  AC_ARG_WITH(mecab-config,
    [AS_HELP_STRING([--with-mecab-config=PATH],
      [set mecab-config location. [default=auto-detect]])],
    [if test "$cross_compiling" = "yes"; then
       MECAB_CONFIG="$withval"
     else
       AC_CHECK_FILE("$withval", MECAB_CONFIG="$withval", MECAB_CONFIG=no)
     fi],
    [AC_PATH_PROG(MECAB_CONFIG, mecab-config, no)])
  if test "x$MECAB_CONFIG" = "xno"; then
    with_mecab="no"
  else
    MECAB_CPPFLAGS="-I`$MECAB_CONFIG --inc-dir`"
    MECAB_LDFLAGS="-L`$MECAB_CONFIG --libs-only-L`"
    _SAVE_LIBS="$LIBS"
    _SAVE_LDFLAGS="$LDFLAGS"
    LDFLAGS="$LDFLAGS $MECAB_LDFLAGS"
    AC_SEARCH_LIBS(mecab_new,
                   mecab,
                   [MECAB_LIBS="-lmecab $PTHREAD_LIBS"],
                   [AC_MSG_ERROR("No libmecab found")],
                   $PTHREAD_LIBS)
    LDFLAGS="$_SAVE_LDFLAGS"
    LIBS="$_SAVE_LIBS"
    _SAVE_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="$CPPFLAGS $MECAB_CPPFLAGS"
    AC_CHECK_HEADER(mecab.h, , [AC_MSG_ERROR("No mecab.h found")])
    AC_CHECK_TYPE([mecab_dictionary_info_t],
                  [AC_DEFINE([HAVE_MECAB_DICTIONARY_INFO_T],
                             [1],
                             [Define to 1 if MeCab has the type `mecab_dictionary_info_t'.])],
                  [],
                  [[#include <mecab.h>]])
    CPPFLAGS="$_SAVE_CPPFLAGS"
    AC_SUBST(MECAB_CPPFLAGS)
    AC_SUBST(MECAB_LDFLAGS)
    AC_SUBST(MECAB_LIBS)
  fi
fi
if test "x$with_mecab" = "xyes"; then
  AC_DEFINE(GRN_WITH_MECAB, [1], [use MeCab])
fi
AM_CONDITIONAL(WITH_MECAB, test "x$with_mecab" = "xyes")

# KyTea
REQUIRED_MINIMUM_KYTEA_VERSION=0.4.2
AC_ARG_WITH(kytea,
  [AS_HELP_STRING([--with-kytea],
    [use KyTea for morphological analysis. [default=auto]])],
  [with_kytea="$withval"],
  [with_kytea="auto"])
AC_MSG_CHECKING([whether enable KyTea])
AC_MSG_RESULT($with_kytea)
if test "x$with_kytea" != "xno"; then
  m4_ifdef([PKG_CHECK_MODULES], [
    PKG_CHECK_MODULES([KYTEA],
                      [kytea >= $REQUIRED_MINIMUM_KYTEA_VERSION],
                      [kytea_exists=yes],
                      [kytea_exists=no])
    ],
    [kytea_exists=no])
  if test "$kytea_exists" = "no" -a "x$with_kytea" = "xyes"; then
    AC_MSG_ERROR("No KyTea found.")
  fi
  with_kytea="$kytea_exists"
fi
if test "x$with_kytea" = "xyes"; then
  AC_DEFINE(GRN_WITH_KYTEA, [1], [use KyTea])
fi
AM_CONDITIONAL(WITH_KYTEA, test "x$with_kytea" = "xyes")

# libstemmer
AC_ARG_WITH(libstemmer,
  [AS_HELP_STRING([--with-libstemmer],
    [use libstemmer for stemming. [default=auto]])],
  [with_libstemmer="$withval"],
  [with_libstemmer="auto"])
AC_ARG_WITH(libstemmer-include,
  [AS_HELP_STRING([--with-libstemmer-include],
    [path to libstemmer.h. [default=auto]])])
AC_ARG_WITH(libstemmer-lib,
  [AS_HELP_STRING([--with-libstemmer-lib],
    [path to libstemmer.so. [default=auto]])])
AC_MSG_CHECKING([whether enable libstemmer])
AC_MSG_RESULT($with_libstemmer)
if test "x$with_libstemmer" != "xno"; then
  LIBSTEMMER_CFLAGS=""
  LIBSTEMMER_LDFLAGS=""
  LIBSTEMMER_LIBS=""

  CFLAGS_save="${CFLAGS}"
  LDFLAGS_save="${LDFLAGS}"
  if test "x$with_libstemmer" != "xauto"; then
    if test -z "${with_libstemmer_include}"; then
      with_libstemmer_include="${with_libstemmer}/include"
    fi
    LIBSTEMMER_CFLAGS="-I${with_libstemmer_include}"
    if test -z "${with_libstemmer_lib}"; then
      with_libstemmer_lib="${with_libstemmer}/lib"
    fi
    LIBSTEMMER_LDFLAGS="-L${with_libstemmer_lib}"
    CFLAGS="${CFLAGS} ${LIBSTEMMER_CFLAGS}"
    LDFLAGS="${LDFLAGS} ${LIBSTEMMER_LDFLAGS}"
  fi
  AC_CHECK_HEADERS(libstemmer.h,
                   [libstemmer_exists=yes],
                   [libstemmer_exists=no])
  if test "$libstemmer_exists" = "yes"; then
    AC_CHECK_LIB(stemmer, sb_stemmer_list,
                 [LIBSTEMMER_LIBS="-lstemmer"],
                 [libstemmer_exists=no])
  fi
  CFLAGS="${CFLAGS_save}"
  LDFLAGS="${LDFLAGS_save}"

  if test "$libstemmer_exists" = "no" -a "x$with_libstemmer" != "xauto"; then
    AC_MSG_ERROR("No libstemmer found at ${with_libstemmer_include} and ${with_libstemmer_lib}.")
  fi
  with_libstemmer="$libstemmer_exists"
fi
if test "x$with_libstemmer" = "xyes"; then
  AC_SUBST(LIBSTEMMER_CFLAGS)
  AC_SUBST(LIBSTEMMER_LDFLAGS)
  AC_SUBST(LIBSTEMMER_LIBS)
  AC_DEFINE(GRN_WITH_LIBSTEMMER, [1], [use libstemmer])
fi
AM_CONDITIONAL(WITH_LIBSTEMMER, test "x$with_libstemmer" = "xyes")

# futex check
AC_ARG_ENABLE(futex,
  [AS_HELP_STRING([--enable-futex],
    [use futex. [default=no]])],
  ,
  [enable_futex="no"])
if test "x$enable_futex" != "xno"; then
  AC_CHECK_HEADERS(linux/futex.h sys/syscall.h, [
    AC_DEFINE(USE_FUTEX, [1], [use futex])
  ], [
    AC_MSG_ERROR("linux/futex.h or sys/syscall.h not found")
  ])
fi
AC_MSG_CHECKING([whether enable futex])
AC_MSG_RESULT($enable_futex)

# ZeroMQ
AC_ARG_ENABLE(zeromq,
  [AS_HELP_STRING([--disable-zeromq],
    [Disable ZeroMQ used for suggestion. [default=auto-detect]])],
  [enable_zeromq="$enableval"],
  [enable_zeromq="auto"])
if test "x$enable_zeromq" != "xno"; then
  m4_ifdef([PKG_CHECK_MODULES], [
    PKG_CHECK_MODULES([LIBZMQ],
                      [libzmq],
                      [zeromq_available=yes],
                      [zeromq_available=no])
    ],
    [zeromq_available=no])
  if test "x$zeromq_available" = "xyes"; then
    AC_DEFINE(GRN_WITH_ZEROMQ, [1], [Define to 1 if ZeroMQ is available.])
  else
    if test "x$enable_zeromq" = "xyes"; then
      AC_MSG_ERROR("No ZeroMQ found")
    fi
  fi
fi

# libevent
AC_ARG_WITH(libevent,
  [AS_HELP_STRING([--without-libevent],
    [Disable libevent used for suggestion. [default=auto]])],
  [with_libevent="$withval"],
  [with_libevent="auto"])

# workaround for bundled groonga in MariaDB.
if test "x$with_libevent" = "xbundled"; then
  with_libevent=no
fi

if test "x$with_libevent" != "xno"; then
  if test "x$with_libevent" = "xyes" -o "x$with_libevent" = "xauto"; then
    libevent_cflags=""
    libevent_ldflags="-levent"
  else
    libevent_include_dir="$with_libevent/include"
    libevent_lib_dir="$with_libevent/lib"
    if ! test -d "$libevent_include_dir" -a -d "$libevent_lib_dir"; then
      AC_MSG_ERROR("No libevent found in $with_libevent.")
    fi
    libevent_cflags="-I$libevent_include_dir"
    libevent_ldflags="-L$libevent_lib_dir -levent"
  fi

  _SAVE_CFLAGS="$CFLAGS"
  _SAVE_LDFLAGS="$LDFLAGS"
  _SAVE_LIBS="$LIBS"
  CFLAGS="$CFLAGS $libevent_cflags"
  LDFLAGS="$LDFLAGS $libevent_ldflags"
  AC_SEARCH_LIBS(event_init, event,
                 [libevent_available=yes],
                 [libevent_available=no])
  CFLAGS="$_SAVE_CFLAGS"
  LDFLAGS="$_SAVE_LDFLAGS"
  LIBS="$_SAVE_LIBS"
  if test "$libevent_available" = "yes"; then
    AC_DEFINE(GRN_WITH_LIBEVENT, [1], [Define to 1 if libevent is available.])
    LIBEVENT_CFLAGS="$libevent_cflags"
    LIBEVENT_LIBS="$libevent_ldflags"
  else
    if test "$enable_option_checking" != "no" -a "x$with_libevent" = "xyes"; then
      AC_MSG_ERROR("No libevent found")
    fi
  fi
fi
AC_SUBST(LIBEVENT_CFLAGS)
AC_SUBST(LIBEVENT_LIBS)

# MessagePack
BUNDLED_MESSAGE_PACK_VERSION=m4_include([bundled_message_pack_version])
AC_SUBST(BUNDLED_MESSAGE_PACK_VERSION)

AC_ARG_ENABLE(message-pack,
  [AS_HELP_STRING([--disable-message-pack],
    [Disable MessagePack support. [default=auto-detect]])],
  [enable_message_pack="$enableval"],
  [enable_message_pack="auto"])
if test "x$enable_message_pack" != "xno"; then
  m4_ifdef([PKG_CHECK_MODULES], [
    PKG_CHECK_MODULES([MESSAGE_PACK], [msgpack-c],
      [message_pack_available=yes],
      [
        PKG_CHECK_MODULES([MESSAGE_PACK], [msgpack],
        [message_pack_available=yes],
        [message_pack_available=no])
      ])
    ],
    [message_pack_vailable=no])

  if test "$message_pack_available" = "no"; then
    AC_ARG_WITH(message-pack,
      [AS_HELP_STRING([--with-message-pack],
        [Specify prefix where MessagePack is installed. [default=/usr]])],
      [with_message_pack="$withval"],
      [with_message_pack="/usr"])
    _SAVE_CFLAGS="$CFLAGS"
    _SAVE_LDFLAGS="$LDFLAGS"
    _SAVE_LIBS="$LIBS"
    CFLAGS="$CFLAGS -I$with_message_pack/include"
    LDFLAGS="$LDFLAGS -L$with_message_pack/lib"
    AC_SEARCH_LIBS(msgpack_version, msgpackc,
                   [message_pack_available=yes],
                   [message_pack_available=no])
    CFLAGS="$_SAVE_CFLAGS"
    LDFLAGS="$_SAVE_LDFLAGS"
    LIBS="$_SAVE_LIBS"

    if test "x$message_pack_available" = "xyes"; then
      MESSAGE_PACK_CFLAGS="-I$with_message_pack/include"
      MESSAGE_PACK_LIBS="-L$with_message_pack/lib -lmsgpackc"
    fi
  fi

  if test "x$message_pack_available" = "xyes"; then
    AC_DEFINE(GRN_WITH_MESSAGE_PACK, [1],
              [Define to 1 if MessagePack is available.])
  else
    if test "x$enable_message_pack" = "xyes"; then
      AC_MSG_ERROR("No MessagePack found")
    fi
  fi
fi
AC_SUBST(MESSAGE_PACK_CFLAGS)
AC_SUBST(MESSAGE_PACK_LIBS)

AM_CONDITIONAL([ENABLE_SUGGEST_LEARNER],
               [test "$zeromq_available" = "yes" -a \
                     "$libevent_available" = "yes" -a \
                     "$message_pack_available" = "yes"])

# Check built-in atomic
case "$host" in
  i*86*|x86_64*)
    ;;
  *)
    AC_MSG_CHECKING([for platform which requires libatomic])
    AC_CHECK_LIB(atomic, __atomic_store_8, [ATOMIC_LIBS="-latomic"])
    AC_SUBST(ATOMIC_LIBS)
    ;;
esac

# Document
AC_MSG_CHECKING([whether enable document])
AC_ARG_ENABLE(document,
  [AS_HELP_STRING([--enable-document],
    [enable document generation by Sphinx. [default=auto]])],
  [enable_document="$enableval"],
  [enable_document="auto"])
AC_MSG_RESULT($enable_document)

document_available=no
document_buildable=no
have_built_document=no
if test x"$enable_document" != x"no"; then
  if test -f "$srcdir/doc/build-stamp"; then
    document_available=yes
    have_built_document=yes
  fi

  if test x"$enable_document" = x"yes"; then
    AC_PATH_PROG(SPHINX_BUILD, sphinx-build, [])
    if test -n "$SPHINX_BUILD"; then
      document_available=yes
      document_buildable=yes
      AM_EXTRA_RECURSIVE_TARGETS([markdown text])
    else
      AC_MSG_ERROR([
No sphinx-build found.
Install it and try again.

How to install sphinx-build:

For Debian GNU/Linux based system like Ubuntu:
  % sudo apt-get install -y python-sphinx

For Red Hat based system like CentOS:
  % sudo yum install -y python-pip
  % sudo pip install sphinx

For OS X with Homebrew:
  % brew install python
  % brew install gettext
  % export PATH="`brew --prefix gettext`/bin:\$PATH"
  % pip install sphinx])
    fi
    AC_SUBST(SPHINX_BUILD)
  fi
fi

# Check for misc.
AC_ARG_WITH([cutter-source-path],
            AS_HELP_STRING([--with-cutter-source-path=PATH],
                           [Specify Cutter source path for
                            groonga's release manager.]),
                           [CUTTER_SOURCE_PATH="$withval"])
case "$CUTTER_SOURCE_PATH" in
  ""|/*)
    : # do nothing
    ;;
  *)
    CUTTER_SOURCE_PATH="\$(top_builddir)/${CUTTER_SOURCE_PATH}"
    ;;
esac
AC_SUBST(CUTTER_SOURCE_PATH)

AM_CONDITIONAL([DOCUMENT_AVAILABLE],
               [test "${document_available}" = "yes"])
AC_MSG_CHECKING([whether document available])
AC_MSG_RESULT($document_available)

AM_CONDITIONAL([DOCUMENT_BUILDABLE],
               [test "${document_buildable}" = "yes"])
AC_MSG_CHECKING([whether document buildable])
AC_MSG_RESULT($document_buildable)

AM_CONDITIONAL([HAVE_BUILT_DOCUMENT],
               [test "${have_built_document}" = "yes"])
AC_MSG_CHECKING([whether having built document])
AC_MSG_RESULT($have_built_document)

DOCUMENT_VERSION=groonga_version
DOCUMENT_VERSION_FULL="$GRN_VERSION"
AC_SUBST(DOCUMENT_VERSION)
AC_SUBST(DOCUMENT_VERSION_FULL)

# Munin plugins
AC_MSG_CHECKING([whether install munin plugins])
AC_ARG_WITH(munin-plugins,
  [AS_HELP_STRING([--with-munin-plugins],
    [install Munin plugins. [default=no]])],
  [install_munin_plugins="$withval"],
  [install_munin_plugins="no"])
AC_MSG_RESULT($install_munin_plugins)

AM_CONDITIONAL([INSTALL_MUNIN_PLUGINS],
               [test "${install_munin_plugins}" = "yes"])

# platform
AC_MSG_CHECKING([whether package platform])
AC_ARG_WITH(package-platform,
  [AS_HELP_STRING([--with-package-platform=PLATFORM],
    [install package platform related files. [default=no]
     (supported package platforms: centos, centos6, centos7, fedora)])],
  [package_platform="$withval"],
  [package_platform="no"])
if test "$package_platform" = "centos"; then
  distribution=$(cut -d " " -f 1 /etc/redhat-release | tr "A-Z" "a-z")
  if grep -q Linux /etc/redhat-release; then
    distribution_version=$(cut -d " " -f 4 /etc/redhat-release)
  else
    distribution_version=$(cut -d " " -f 3 /etc/redhat-release)
  fi
  distribution_version=$(echo ${distribution_version} | sed -e 's/\..*$//g')
  package_platform="${package_platform}${distribution_version}"
fi
AC_MSG_RESULT($package_platform)

centos_platform=no
centos_systemd_platform=no
case "${package_platform}" in
  no)
    ;;
  *)
    centos_platform=yes
    centos_systemd_platform=yes
    ;;
esac
AM_CONDITIONAL([CENTOS_PLATFORM],
               [test "${centos_platform}" = "yes"])
AM_CONDITIONAL([CENTOS_SYSTEMD_PLATFORM],
               [test "${centos_systemd_platform}" = "yes"])

# plugins check
relative_pluginsdir_base="\$(PACKAGE)/plugins"
AC_SUBST(relative_pluginsdir_base)
expanded_relative_pluginsdir_base="${PACKAGE}/plugins"
AC_SUBST(expanded_relative_pluginsdir_base)

relative_pluginsdir="lib/\$(relative_pluginsdir_base)"
AC_SUBST(relative_pluginsdir)

pluginsdir="\${libdir}/\$(relative_pluginsdir_base)"
AC_SUBST(pluginsdir)
expanded_pluginsdir="\${libdir}/${expanded_relative_pluginsdir_base}"
AC_SUBST(expanded_pluginsdir)

tokenizer_pluginsdir="\${pluginsdir}/tokenizers"
AC_SUBST(tokenizer_pluginsdir)

query_expander_pluginsdir="\${pluginsdir}/query_expanders"
AC_SUBST(query_expander_pluginsdir)

suggest_pluginsdir="\${pluginsdir}/suggest"
AC_SUBST(suggest_pluginsdir)

ruby_pluginsdir="\${pluginsdir}/ruby"
AC_SUBST(ruby_pluginsdir)

token_filter_pluginsdir="\${pluginsdir}/token_filters"
AC_SUBST(token_filter_pluginsdir)

sharding_pluginsdir="\${pluginsdir}/sharding"
AC_SUBST(sharding_pluginsdir)

function_pluginsdir="\${pluginsdir}/functions"
AC_SUBST(function_pluginsdir)

expression_rewriter_pluginsdir="\${pluginsdir}/expression_rewriters"
AC_SUBST(expression_rewriter_pluginsdir)

AC_MSG_CHECKING(for the suffix of plugin shared libraries)
shrext_cmds=$(./libtool --config | grep '^shrext_cmds=')
eval $shrext_cmds
module=yes eval suffix="$shrext_cmds"
AC_MSG_RESULT($suffix)
if test -z "$suffix"; then
  AC_MSG_ERROR([can't detect plugin suffix])
fi
AC_DEFINE_UNQUOTED(GRN_PLUGIN_SUFFIX, ["$suffix"], "plugin suffix")

# for query expanders
GRN_QUERY_EXPANDER_TSV_RELATIVE_SYNONYMS_FILE_BASE="synonyms.tsv"
GRN_QUERY_EXPANDER_TSV_RELATIVE_SYNONYMS_FILE="etc/${PACKAGE}/${GRN_QUERY_EXPANDER_TSV_RELATIVE_SYNONYMS_FILE_BASE}"
AC_DEFINE_UNQUOTED(GRN_QUERY_EXPANDER_TSV_RELATIVE_SYNONYMS_FILE,
                   ["$GRN_QUERY_EXPANDER_TSV_RELATIVE_SYNONYMS_FILE"],
                   "The relative synonyms file for TSV query expander")
GRN_QUERY_EXPANDER_TSV_SYNONYMS_PATH="`
  eval echo ${sysconfdir}/${PACKAGE}/${GRN_QUERY_EXPANDER_TSV_RELATIVE_SYNONYMS_FILE_BASE}
`"
AC_DEFINE_UNQUOTED(GRN_QUERY_EXPANDER_TSV_SYNONYMS_FILE,
                   ["$GRN_QUERY_EXPANDER_TSV_SYNONYMS_PATH"],
                   "The default synonyms file for TSV query expander")

# for examples
examplesdir="\$(pkgdatadir)/examples"
AC_SUBST(examplesdir)

examples_dictionarydir="\$(examplesdir)/dictionary"
AC_SUBST(examples_dictionarydir)

# for ruby scripts
relative_ruby_scriptsdir_base="\$(PACKAGE)/scripts/ruby"
AC_SUBST(relative_ruby_scriptsdir_base)
relative_ruby_scriptsdir="lib/\$(relative_ruby_scriptsdir_base)"
AC_SUBST(relative_ruby_scriptsdir)
ruby_scriptsdir="\${libdir}/\$(relative_ruby_scriptsdir_base)"
AC_SUBST(ruby_scriptsdir)

# for document root
GRN_DEFAULT_DOCUMENT_ROOT_BASE="html/admin"
GRN_DEFAULT_DOCUMENT_ROOT="\${pkgdatadir}/\${GRN_DEFAULT_DOCUMENT_ROOT_BASE}"
GRN_EXPANDED_DEFAULT_DOCUMENT_ROOT="\${datadir}/${PACKAGE}/${GRN_DEFAULT_DOCUMENT_ROOT_BASE}"
GRN_DEFAULT_RELATIVE_DOCUMENT_ROOT="share/\$(PACKAGE)/\$(GRN_DEFAULT_DOCUMENT_ROOT_BASE)"
AC_SUBST(GRN_DEFAULT_DOCUMENT_ROOT_BASE)
AC_SUBST(GRN_DEFAULT_DOCUMENT_ROOT)
AC_SUBST(GRN_EXPANDED_DEFAULT_DOCUMENT_ROOT)
AC_SUBST(GRN_DEFAULT_RELATIVE_DOCUMENT_ROOT)

# flags for compile groonga
GRN_CFLAGS=""
AC_SUBST(GRN_CFLAGS)
GRN_DEFS=""
GRN_DEFS="$GRN_DEFS -DGRN_DLL_FILENAME=L\\\"\"\$(GRN_DLL_FILENAME)\"\\\""
GRN_DEFS="$GRN_DEFS -DGRN_PLUGINS_DIR=\\\"\"\$(pluginsdir)\"\\\""
GRN_DEFS="$GRN_DEFS -DGRN_RELATIVE_PLUGINS_DIR=\\\"\"\$(relative_pluginsdir)\"\\\""
GRN_DEFS="$GRN_DEFS -DGRN_RUBY_SCRIPTS_DIR=\\\"\"\$(ruby_scriptsdir)\"\\\""
GRN_DEFS="$GRN_DEFS -DGRN_RELATIVE_RUBY_SCRIPTS_DIR=\\\"\"\$(relative_ruby_scriptsdir)\"\\\""
GRN_DEFS="$GRN_DEFS -DGRN_LOG_PATH=\\\"\"\$(grn_log_path)\"\\\""
GRN_DEFS="$GRN_DEFS -DGRN_DEFAULT_DOCUMENT_ROOT=\\\"\"\$(GRN_DEFAULT_DOCUMENT_ROOT)\"\\\""
GRN_DEFS="$GRN_DEFS -DGRN_DEFAULT_RELATIVE_DOCUMENT_ROOT=\\\"\"\$(GRN_DEFAULT_RELATIVE_DOCUMENT_ROOT)\"\\\""
AC_SUBST(GRN_DEFS)
CFLAGS="$CFLAGS $OPT_CFLAGS "
AC_DEFINE_UNQUOTED(CONFIGURE_OPTIONS, "$ac_configure_args", "specified configure options")

# For groonga.org
AC_ARG_WITH(groonga-org-path,
            [AS_HELP_STRING([--with-groonga-org-path=PATH],
              [specify a path of the groonga.org repository to update groonga.org.])],
            [GROONGA_ORG_PATH="$withval"],
            [GROONGA_ORG_PATH=""])
AC_SUBST(GROONGA_ORG_PATH)

# groonga-httpd
NGINX_VERSION=m4_include([nginx_version])
AC_SUBST(NGINX_VERSION)

# groonga-httpd binary path
GROONGA_HTTPD="${ac_pwd}/vendor/nginx-${NGINX_VERSION}/objs/nginx"
AC_SUBST(GROONGA_HTTPD)

AC_ARG_ENABLE(groonga_httpd,
  [AS_HELP_STRING([--enable-groonga-httpd],
    [enable nginx used for groonga-httpd. [default=yes]])],
  [enable_groonga_httpd="$enableval"],
  [enable_groonga_httpd="yes"])
if test "x$enable_groonga_httpd" != "xno"; then
  enable_groonga_httpd="yes"
  AC_CONFIG_SUBDIRS([src/httpd])
else
  enable_groonga_httpd="no"
fi
AM_CONDITIONAL(WITH_GROONGA_HTTPD, test "$enable_groonga_httpd" = "yes")

GROONGA_HTTPD_PID_PATH="`
  test \"$prefix\" = NONE && prefix=/usr/local
  eval echo ${localstatedir}/run/groonga/groonga-httpd.pid
`"
AC_SUBST(GROONGA_HTTPD_PID_PATH)

# mruby
AC_ARG_ENABLE(mruby,
  [AS_HELP_STRING([--enable-mruby],
    [enable mruby. [default=no]])],
  [enable_mruby="$enableval"],
  [enable_mruby="no"])

AC_MSG_CHECKING([whether enable mruby])
if test "x$enable_mruby" != "xyes"; then
  enable_mruby="no"
fi
AC_MSG_RESULT($enable_mruby)

if test "$enable_mruby" = "yes"; then
  if test ! -f "$srcdir/vendor/mruby/mruby_build.timestamp" -a \
          "$ac_cv_ruby_available" != "yes"; then
    AC_MSG_ERROR(--enable-mruby requires --with-ruby)
  fi
  AC_DEFINE(GRN_WITH_MRUBY, [1], [Define to 1 if mruby is enabled.])
  MRUBY_CFLAGS="-I\$(top_srcdir)/vendor/mruby-source/include"
  GRN_WITH_MRUBY="yes"
else
  MRUBY_CFLAGS=""
fi
AC_SUBST(GRN_WITH_MRUBY)
AC_SUBST(MRUBY_CFLAGS)
AM_CONDITIONAL(WITH_MRUBY, test "$enable_mruby" = "yes")

MRUBY_CPPFLAGS="-DMRB_INT64"
AC_SUBST(MRUBY_CPPFLAGS)

# For mruby-file-stat
if test "$os_win32" = "yes"; then
  HAVE_SYS_SYSMACROS_H=no
else
  AC_CHECK_HEADERS(sys/sysmacros.h,
                   [HAVE_SYS_SYSMACROS_H=yes],
                   [HAVE_SYS_SYSMACROS_H=no])
fi
AM_CONDITIONAL(HAVE_SYS_SYSMACROS_H, test "$HAVE_SYS_SYSMACROS_H" = "yes")

# This option is used in vendor/onigmo/configure
AC_ARG_ENABLE(shared-onigmo,
  [AS_HELP_STRING([--enable-shared-onigmo],
    [use Onigmo as shared library instead of static library. [default=no]])],
  [enable_shared_onigmo="$enableval"],
  [enable_shared_onigmo="no"])
AM_CONDITIONAL(WITH_SHARED_ONIGMO, test "$enable_shared_onigmo" = "yes")

AC_ARG_WITH(onigmo,
  [AS_HELP_STRING([--without-onigmo],
    [Don't Use Onigmo. [default=bundled]])],
  [with_onigmo="$withval"],
  [with_onigmo="bundled"])
if test "x$with_onigmo" != "xno"; then
  GRN_WITH_ONIGMO="yes"
  if test "x$with_onigmo" != "xbundled"; then
    m4_ifdef([PKG_CHECK_MODULES], [
      PKG_CHECK_MODULES([ONIGMO], [onigmo],
        [have_onigmo=yes],
        [have_onigmo=no])
      ],
      [have_onigmo=no])
  fi
  if test "x$with_onigmo" = "xsystem" -a "$have_onigmo" = "no"; then
    AC_MSG_ERROR("No Onigmo found")
  fi
  if test "x$with_onigmo" = "xbundled" -o "$have_onigmo" = "no"; then
    AC_CONFIG_SUBDIRS([vendor/onigmo])
    ONIGMO_CFLAGS="-I\$(top_srcdir)/vendor/onigmo-source"
    ONIGMO_LIBS="\$(top_builddir)/vendor/onigmo-source/libonigmo.la"
  fi
  AC_DEFINE(GRN_WITH_ONIGMO, [1], [Use Onigmo.])
else
  GRN_WITH_ONIGMO="no"
fi
AC_SUBST(GRN_WITH_ONIGMO)
AC_SUBST(ONIGMO_CFLAGS)
AC_SUBST(ONIGMO_LIBS)
AM_CONDITIONAL(WITH_BUNDLED_ONIGMO, test "$with_onigmo" != "no" -a "x$have_onigmo" != "xyes")

# PCRE
GRN_WITH_PCRE=no
AC_ARG_WITH(pcre,
  [AS_HELP_STRING([--without-pcre],
    [Don't use PCRE for groonga-httpd. [default=auto-detect]])],
  [with_pcre="$withval"],
  [with_pcre="auto"])
if test "x$with_pcre" != "xno"; then
  m4_ifdef([PKG_CHECK_MODULES], [
    PKG_CHECK_MODULES([PCRE], [libpcre],
      [_PKG_CONFIG(PCRE_LIBS_ONLY_L, [libs-only-L], [libpcre])
       PCRE_LIBS_ONLY_L="$pkg_cv_PCRE_LIBS_ONLY_L"
       GRN_WITH_PCRE=yes],
      [GRN_WITH_PCRE=no])
    ],
    [GRN_WITH_PCRE=no])
  if test "x$with_pcre" = "xyes" -a "$GRN_WITH_PCRE" != "yes"; then
    AC_MSG_ERROR("No PCRE found")
  fi
fi
AC_SUBST(GRN_WITH_PCRE)
AC_SUBST(PCRE_CFLAGS)
AC_SUBST(PCRE_LIBS_ONLY_L)

# SSL
GRN_WITH_SSL=no
AC_ARG_WITH(ssl,
  [AS_HELP_STRING([--without-ssl],
    [Don't use SSL module for groonga-httpd. [default=auto-detect]])],
  [with_ssl="$withval"],
  [with_ssl="auto"])
if test "x$with_ssl" != "xno"; then
  m4_ifdef([PKG_CHECK_MODULES], [
    PKG_CHECK_MODULES([SSL], [libssl],
      [_PKG_CONFIG(SSL_LIBS_ONLY_L, [libs-only-L], [libssl])
       SSL_LIBS_ONLY_L="$pkg_cv_SSL_LIBS_ONLY_L"
       GRN_WITH_SSL=yes],
      [GRN_WITH_SSL=no])
    ],
    [GRN_WITH_SSL=no])
  if test "x$with_ssl" = "xyes" -a "$GRN_WITH_SSL" != "yes"; then
    AC_MSG_ERROR("No SSL found")
  fi
fi
AC_SUBST(GRN_WITH_SSL)
AC_SUBST(SSL_CFLAGS)
AC_SUBST(SSL_LIBS_ONLY_L)

# For package
AC_ARG_WITH(rsync-path,
            [AS_HELP_STRING([--with-rsync-path=PATH],
            [specify rsync path to upload groonga packages.])],
            [RSYNC_PATH="$withval"],
            [RSYNC_PATH="packages@packages.groonga.org:public"])
AC_SUBST(RSYNC_PATH)

AC_ARG_WITH(launchpad-ppa,
            [AS_HELP_STRING([--with-launchpad-ppa=PPA],
            [specify Launchpad Personal Package Archive. [default=groonga-ppa]])],
            [LAUNCHPAD_PPA="$withval"],
            [LAUNCHPAD_PPA="groonga-ppa"])
AC_SUBST(LAUNCHPAD_PPA)

AC_ARG_WITH(launchpad-uploader-pgp-key,
            [AS_HELP_STRING([--with-launchpad-uploader-pgp-key=KEY],
            [specify PGP key UID to upload Groonga packages to Launchpad.])],
            [LAUNCHPAD_UPLOADER_PGP_KEY="$withval"],
            [LAUNCHPAD_UPLOADER_PGP_KEY=""])
AC_SUBST(LAUNCHPAD_UPLOADER_PGP_KEY)

GPG_UID=m4_include(gpg_uid)
AC_SUBST(GPG_UID)

GPG_UID_RSA4096=m4_include(gpg_uid_rsa4096)
AC_SUBST(GPG_UID_RSA4096)

pkgsysconfdir="\${sysconfdir}/$PACKAGE_NAME"
AC_SUBST(pkgsysconfdir)

GRN_CONFIG_PATH="`
  test \"$prefix\" = NONE && prefix=/usr/local
  eval echo ${sysconfdir}/groonga/groonga.conf
`"
AC_DEFINE_UNQUOTED(GRN_CONFIG_PATH, ["$GRN_CONFIG_PATH"],
  [Default command line option configuration file.])

GROONGA_HTTPD_DOCUMENT_ROOT="`
  test \"$prefix\" = NONE && prefix=/usr/local
  eval eval eval echo ${GRN_EXPANDED_DEFAULT_DOCUMENT_ROOT}
`"
AC_SUBST(GROONGA_HTTPD_DOCUMENT_ROOT)

GROONGA_HTTPD_DEFAULT_DATABASE_PATH="`
  test \"$prefix\" = NONE && prefix=/usr/local
  eval eval eval echo ${localstatedir}/lib/${PACKAGE}/db/db
`"
AC_SUBST(GROONGA_HTTPD_DEFAULT_DATABASE_PATH)

AC_CONFIG_FILES([
  Makefile
  benchmark/Makefile
  benchmark/fixtures/Makefile
  benchmark/fixtures/geo-select/Makefile
  benchmark/lib/Makefile
  build/Makefile
  build/cmake_modules/Makefile
  config.sh
  data/Makefile
  data/groonga-httpd.conf
  data/html/Makefile
  data/logrotate.d/Makefile
  data/logrotate.d/centos/Makefile
  data/logrotate.d/centos/groonga-httpd
  data/munin/Makefile
  data/scripts/Makefile
  data/scripts/groonga-httpd-restart
  data/systemd/Makefile
  data/systemd/centos/Makefile
  data/systemd/centos/groonga-httpd.service
  data/systemd/centos/sysconfig/Makefile
  data/tmpfiles.d/Makefile
  doc/Makefile
  doc/locale/Makefile
  doc/locale/en/LC_MESSAGES/Makefile
  doc/locale/en/Makefile
  doc/locale/ja/LC_MESSAGES/Makefile
  doc/locale/ja/Makefile
  examples/Makefile
  examples/dictionary/Makefile
  examples/dictionary/edict/Makefile
  examples/dictionary/eijiro/Makefile
  examples/dictionary/gene95/Makefile
  examples/dictionary/jmdict/Makefile
  groonga-arrow.pc
  groonga-httpd-conf.sh
  groonga.pc
  include/Makefile
  include/groonga/Makefile
  include/groonga/version.h
  lib/Makefile
  lib/dat/Makefile
  lib/metadata.rc
  lib/mrb/Makefile
  lib/mrb/scripts/Makefile
  lib/mrb/scripts/command_line/Makefile
  lib/mrb/scripts/context/Makefile
  lib/mrb/scripts/expression_rewriters/Makefile
  lib/mrb/scripts/expression_tree/Makefile
  lib/mrb/scripts/initialize/Makefile
  lib/mrb/scripts/logger/Makefile
  lib/mrb/scripts/query_logger/Makefile
  lib/proc/Makefile
  packages/Makefile
  packages/apt/debian/groonga-keyring.postrm
  packages/apt/env.sh
  plugins/Makefile
  plugins/functions/Makefile
  plugins/query_expanders/Makefile
  plugins/ruby/Makefile
  plugins/sharding/Makefile
  plugins/suggest/Makefile
  plugins/token_filters/Makefile
  plugins/tokenizers/Makefile
  src/Makefile
  src/httpd/Makefile
  src/suggest/Makefile
  test/Makefile
  test/command/Makefile
  test/unit/Makefile
  test/unit/core/Makefile
  test/unit/core/dat/Makefile
  test/unit/fixtures/Makefile
  test/unit/fixtures/geo/Makefile
  test/unit/fixtures/inverted-index/Makefile
  test/unit/fixtures/plugins/Makefile
  test/unit/fixtures/story/Makefile
  test/unit/fixtures/story/taiyaki/Makefile
  test/unit/fixtures/stress/Makefile
  test/unit/lib/Makefile
  test/unit/story/Makefile
  test/unit/util/Makefile
  tools/Makefile
  vendor/Makefile
  vendor/groonga-log/Makefile
  vendor/lz4/Makefile
  vendor/mecab/Makefile
  vendor/message_pack/Makefile
  vendor/mruby/Makefile
  vendor/onigmo/Makefile
])

AC_OUTPUT

echo "$PACKAGE_NAME $PACKAGE_VERSION configuration:"
echo "-----------------------"
echo "  Compiler:              ${CC}"
echo "  CFLAGS:                ${CFLAGS}"
echo "  CXXFLAGS:              ${CXXFLAGS}"
echo "  Libraries:             ${LIBS}"
echo "  Stack size:            ${GRN_STACK_SIZE}"
echo "  Document:              ${document_available}"
echo "    buildable:           ${document_buildable}"
echo "    built:               ${have_built_document}"
echo "  Munin plugins:         ${install_munin_plugins}"
echo "  Package platform:      ${package_platform}"
echo
echo "Paths:"
echo "  Install path prefix:   ${prefix}"
echo "  Configuration file:    ${GRN_CONFIG_PATH}"
echo

echo "Tokenizers:"
echo "  MeCab:                 $with_mecab"
if test "x$with_mecab" = "xyes"; then
  echo "    CPPFLAGS:            $MECAB_CPPFLAGS"
  echo "    LDFLAGS:             $MECAB_LDFLAGS"
  echo "    LIBS:                $MECAB_LIBS"
fi
echo "  KyTea:                 $with_kytea"
if test "x$with_kytea" = "xyes"; then
  echo "    CFLAGS:              $KYTEA_CFLAGS"
  echo "    LIBS:                $KYTEA_LIBS"
fi
echo

echo "Token filters:"
echo "  libstemmer:            $with_libstemmer"
if test "x$with_libstemmer" = "xyes"; then
  echo "    CFLAGS:              $LIBSTEMMER_CFLAGS"
  echo "    LIBS:                $LIBSTEMMER_LIBS"
fi
echo

echo "Libraries:"
echo "  ZeroMQ:                $zeromq_available"
if test "x$zeromq_available" = "xyes"; then
  echo "    CFLAGS:              ${LIBZMQ_CFLAGS}"
  echo "    LIBS:                ${LIBZMQ_LIBS}"
fi
echo "  libevent:              $libevent_available"
if test "x$libevent_available" = "xyes"; then
  echo "    CFLAGS:              ${LIBEVENT_CFLAGS}"
  echo "    LIBS:                ${LIBEVENT_LIBS}"
fi
echo "  MessagePack:           $message_pack_available"
if test "x$message_pack_available" = "xyes"; then
  echo "    CFLAGS:              ${MESSAGE_PACK_CFLAGS}"
  echo "    LIBS:                ${MESSAGE_PACK_LIBS}"
fi
echo "  libedit:               $libedit_available"
if test "x$libedit_available" = "xyes"; then
  echo "    CFLAGS:              ${LIBEDIT_CFLAGS}"
  echo "    LIBS:                ${LIBEDIT_LIBS}"
fi
echo "  zlib:                  $GRN_WITH_ZLIB"
if test "x$GRN_WITH_ZLIB" = "xyes"; then
  echo "    CFLAGS:              ${ZLIB_CFLAGS}"
  echo "    LIBS:                ${ZLIB_LIBS}"
  echo "    LIB_DIR:             ${ZLIB_LIB_DIR}"
fi
echo "  LZ4:                   $GRN_WITH_LZ4"
if test "x$GRN_WITH_LZ4" = "xyes"; then
  echo "    CFLAGS:              ${LIBLZ4_CFLAGS}"
  echo "    LIBS:                ${LIBLZ4_LIBS}"
fi
echo "  Zstandard:             $GRN_WITH_ZSTD"
if test "x$GRN_WITH_ZSTD" = "xyes"; then
  echo "    CFLAGS:              ${LIBZSTD_CFLAGS}"
  echo "    LIBS:                ${LIBZSTD_LIBS}"
fi
echo "  LuaJIT:                $GRN_WITH_LUAJIT"
if test "x$GRN_WITH_LUAJIT" = "xyes"; then
  echo "    CFLAGS:              ${LUAJIT_CFLAGS}"
  echo "    LIBS:                ${LUAJIT_LIBS}"
fi
echo "  RapidJSON:             $GRN_WITH_RAPIDJSON"
if test "x$GRN_WITH_RAPIDJSON" = "xyes"; then
  echo "    CFLAGS:              ${RAPIDJSON_CFLAGS}"
fi
echo "  Apache Arrow:          $GRN_WITH_APACHE_ARROW"
if test "x$GRN_WITH_APACHE_ARROW" = "xyes"; then
  echo "    CFLAGS:              ${APACHE_ARROW_CFLAGS}"
  echo "    LIBS:                ${APACHE_ARROW_LIBS}"
fi
echo "  xxHash:                $GRN_WITH_XXHASH"
if test "x$GRN_WITH_XXHASH" = "xyes"; then
  echo "    CFLAGS:              ${LIBXXHASH_CFLAGS}"
  echo "    LIBS:                ${LIBXXHASH_LIBS}"
fi
echo "  mruby:                 $enable_mruby"
echo "  jemalloc:              $jemalloc_available"
echo

echo "groonga-httpd:"
echo "  enable:                $enable_groonga_httpd"
if test "$enable_groonga_httpd" = "yes"; then
  echo "  default database path: $GROONGA_HTTPD_DEFAULT_DATABASE_PATH"
  echo "  PCRE:                  $GRN_WITH_PCRE"
  if test "$GRN_WITH_PCRE" = "yes"; then
    echo "    CFLAGS:              $PCRE_CFLAGS"
    echo "    LIBS only -L:        $PCRE_LIBS_ONLY_L"
  fi
  echo "   SSL:                  $GRN_WITH_SSL"
  if test "$GRN_WITH_SSL" = "yes"; then
    echo "    CFLAGS:              $SSL_CFLAGS"
    echo "    LIBS only -L:        $SSL_LIBS_ONLY_L"
  fi
fi
echo

echo "Tools:"
echo "  Sphinx:                ${SPHINX_BUILD}"
echo "  lemon:                 ${LEMON}"
echo "  Ruby:                  ${RUBY}"
echo "  Cutter:                ${CUTTER}"
echo

echo "For packages:"
echo "  rsync path:            ${RSYNC_PATH}"
echo "  Launchpad PGP key:     ${LAUNCHPAD_UPLOADER_PGP_KEY}"
echo "  GPG UID:               ${GPG_UID}"
echo

echo "Now type 'make' to build $PACKAGE_NAME $PACKAGE_VERSION!"
