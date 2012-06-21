AC_PREREQ(2.59)
m4_define([groonga_version], m4_include(base_version))
AC_INIT([groonga], groonga_version, [groonga@razil.jp])
AM_CONFIG_HEADER(config.h)

AM_INIT_AUTOMAKE(tar-pax)

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
AC_PROG_CC
AM_PROG_CC_C_O
m4_ifdef([PKG_PROG_PKG_CONFIG],
         [PKG_PROG_PKG_CONFIG([0.19])
          m4_pattern_allow(PKG_CONFIG_LIBDIR)])

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
if test "$GCC" = "yes"; then
  CHECK_BUILD_FLAG([-Wall])
  CHECK_BUILD_FLAG([-Wextra])
  if test "x$check_cflag" = "xno"; then
    CHECK_BUILD_FLAG([-W])
  fi
  CHECK_BUILD_FLAG([-Wno-unused-but-set-variable]) # FIXME: enable it.
  CHECK_BUILD_FLAG([-Wno-unused-parameter])
  CHECK_BUILD_FLAG([-Wno-sign-compare])
  CHECK_CFLAG([-Wno-pointer-sign])
  CHECK_BUILD_FLAG([-Wno-missing-field-initializers])

  CHECK_BUILD_FLAG([-Wformat=2])
  CHECK_BUILD_FLAG([-Wstrict-aliasing=2])
  if test "x$check_cflag" = "xyes"; then
    NO_STRICT_ALIASING_CFLAGS="-fno-strict-aliasing"
  fi
  CHECK_BUILD_FLAG([-Wdisabled-optimization])
  CHECK_BUILD_FLAG([-Wfloat-equal])
  CHECK_BUILD_FLAG([-Wpointer-arith])
  CHECK_CFLAG([-Wdeclaration-after-statement])
  CHECK_CFLAG([-Wbad-function-cast])
  CHECK_BUILD_FLAG([-Wcast-align])
  CHECK_BUILD_FLAG([-Wredundant-decls])
#  CHECK_BUILD_FLAG([-Wunsafe-loop-optimizations])
#  CHECK_BUILD_FLAG([-Wunreachable-code])
#  CHECK_BUILD_FLAG([-Wswitch-enum])
#  CHECK_BUILD_FLAG([-Wshadow])
#  CHECK_BUILD_FLAG([-Wconversion])
  CHECK_BUILD_FLAG([-Wwrite-strings])
#  CHECK_BUILD_FLAG([-Winline])

  CHECK_CXXFLAG([-fexceptions])
  CHECK_CXXFLAG([-fimplicit-templates])

  CFLAGS_for_source="$CFLAGS"
  CXXFLAGS_for_source="$CXXFLAGS"
  CHECK_BUILD_FLAG([-Wno-clobbered])
  if test "x$check_cflag" = "xyes"; then
    TEST_CFLAGS="-Wno-clobbered"
  fi
  if test "x$check_cxxflag" = "xyes"; then
    TEST_CXXFLAGS="-Wno-clobbered"
  fi
  CFLAGS="$CFLAGS_for_source"
  CXXFLAGS="$CXXFLAGS_for_source"
fi
AC_SUBST(TEST_CFLAGS)
AC_SUBST(TEST_CXXFLAGS)
AC_SUBST(NO_STRICT_ALIASING_CFLAGS)

AC_LIBTOOL_WIN32_DLL
AM_PROG_LIBTOOL
m4_ifdef([LT_OUTPUT], [LT_OUTPUT])

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
  source "$srcdir/version.sh"
  AC_SUBST(GRN_VERSION)
  AC_DEFINE_UNQUOTED(GRN_VERSION, ["$GRN_VERSION"], [groonga version])
fi

AC_CONFIG_FILES([
  Makefile
  build/Makefile
  build/cmake_modules/Makefile
  src/Makefile
  src/suggest/Makefile
  lib/Makefile
  lib/dat/Makefile
  include/Makefile
  include/groonga/Makefile
  plugins/Makefile
  plugins/tokenizers/Makefile
  plugins/suggest/Makefile
  plugins/table/Makefile
  examples/Makefile
  examples/dictionary/Makefile
  examples/dictionary/edict/Makefile
  examples/dictionary/eijiro/Makefile
  examples/dictionary/gene95/Makefile
  examples/dictionary/jmdict/Makefile
  packages/Makefile
  packages/apt/Makefile
  packages/rpm/Makefile
  packages/rpm/centos/Makefile
  packages/rpm/fedora/Makefile
  packages/yum/Makefile
  packages/source/Makefile
  packages/windows/Makefile
  packages/windows/patches/Makefile
  packages/windows/language-files/Makefile
  packages/windows/setup-x64.nsi
  data/Makefile
  data/images/Makefile
  data/images/logo/Makefile
  data/html/Makefile
  data/munin/Makefile
  data/init.d/Makefile
  data/init.d/redhat/Makefile
  data/init.d/redhat/sysconfig/Makefile
  data/systemd/Makefile
  data/systemd/fedora/Makefile
  data/systemd/fedora/sysconfig/Makefile
  tools/Makefile
  doc/Makefile
  doc/locale/Makefile
  doc/locale/en/Makefile
  doc/locale/en/LC_MESSAGES/Makefile
  doc/locale/ja/Makefile
  doc/locale/ja/LC_MESSAGES/Makefile
  test/Makefile
  test/unit/Makefile
  test/unit/lib/Makefile
  test/unit/lib/ruby/Makefile
  test/unit/fixtures/Makefile
  test/unit/fixtures/inverted-index/Makefile
  test/unit/fixtures/stress/Makefile
  test/unit/fixtures/performance/Makefile
  test/unit/fixtures/plugins/Makefile
  test/unit/fixtures/geo/Makefile
  test/unit/fixtures/story/Makefile
  test/unit/fixtures/story/taiyaki/Makefile
  test/unit/util/Makefile
  test/unit/core/Makefile
  test/unit/core/dat/Makefile
  test/unit/memcached/Makefile
  test/unit/http/Makefile
  test/unit/gqtp/Makefile
  test/unit/story/Makefile
  test/unit/groonga-benchmark/Makefile
  test/function/Makefile
  test/benchmark/Makefile
  test/benchmark/fixtures/Makefile
  test/benchmark/fixtures/geo-select/Makefile
  test/benchmark/lib/Makefile
  vendor/Makefile
])

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

# AIO
AC_ARG_ENABLE(aio,
  [AS_HELP_STRING([--enable-aio],
                  [use aio/dio based cache. [default=no]])],
  ,
  [enable_aio="no"])
if test "x$enable_aio" != "xno"; then
  AC_DEFINE(_XOPEN_SOURCE, [600], [Define to 600 for use AIO.])
  AC_SEARCH_LIBS(nanosleep, rt, [], [AC_MSG_ERROR("No librt found")])
  AC_DEFINE(USE_AIO, [1], [use AIO/DIO])
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
    [specify groonga default DB key(pat/dat/auto)])],
  GRN_DEFAULT_DB_KEY="$withval",
  GRN_DEFAULT_DB_KEY="auto")
AC_DEFINE_UNQUOTED(GRN_DEFAULT_DB_KEY, "$GRN_DEFAULT_DB_KEY",
                   "specified default DB key management algorithm")

if test "$os_win32" != "yes"; then
  AC_CHECK_HEADERS(pthread.h)
  AC_SEARCH_LIBS(pthread_create, pthread,
                 [],
                 [AC_MSG_ERROR("No libpthread found")])
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
  AC_DEFINE(WITH_NFKC, [1], [compile with nfkc.c])
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

## malloc
force_enable_dynamic_malloc_change="no"

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

# failmalloc
AC_MSG_CHECKING([whether enable fmalloc])
AC_ARG_ENABLE(fmalloc,
  [AS_HELP_STRING([--enable-fmalloc],
    [make memory allocation failed in specified condition for debug. [default=no]])],
  ,
  [enable_fmalloc="no"])
if test "x$enable_fmalloc" != "xno"; then
  force_enable_dynamic_malloc_change="yes"
  AC_DEFINE(USE_FAIL_MALLOC, [1], [use fmalloc])
fi
AC_MSG_RESULT($enable_fmalloc)

# abort
AC_MSG_CHECKING([whether enable abort])
AC_ARG_ENABLE(abort,
  [AS_HELP_STRING([--enable-abort],
    [enable query abortion. [default=no]])],
  ,
  [enable_abort="no"])
if test "x$enable_abort" != "xno"; then
  force_enable_dynamic_malloc_change="yes"
  AC_DEFINE(USE_QUERY_ABORT, [1], [use abort])
fi
AC_MSG_RESULT($enable_abort)

# dynamic malloc change
AC_MSG_CHECKING([whether allow dynamic memory allocation change])
AC_ARG_ENABLE(dynamic-malloc-change,
  [AS_HELP_STRING([--enable-dynamic-malloc-change],
    [allow dynamic memory allocation change for testing. [default=no]])],
  ,
  [enable_dynamic_malloc_change="no"])
if test "x$enable_dynamic_malloc_change" != "xyes" -a \
     "x$force_enable_dynamic_malloc_change" = "xyes"; then
  enable_dynamic_malloc_change="yes"
  AC_MSG_RESULT([$enable_dynamic_malloc_change (force)])
else
  AC_MSG_RESULT([$enable_dynamic_malloc_change])
fi

if test "x$enable_dynamic_malloc_change" = "xyes"; then
  AC_DEFINE(USE_DYNAMIC_MALLOC_CHANGE, [1],
            [Define to 1 if you enable dynamic malloc change])
fi

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
    AC_CHECK_HEADER(sys/poll.h, [
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
  WINDOWS_LIBS="-ladvapi32 -lws2_32"
else
  WINDOWS_LDFLAGS=
  WINDOWS_LIBS=
fi
AC_SUBST(WINDOWS_LDFLAGS)
AC_SUBST(WINDOWS_LIBS)

# groonga binary path
GROONGA="\$(abs_top_builddir)/src/groonga"
AC_SUBST(GROONGA)

# groonga-httpd binary path
GROONGA_HTTPD="\$(abs_top_builddir)/src/httpd/groonga-httpd"
AC_SUBST(GROONGA_HTTPD)

# groonga-benchmark binary path
GROONGA_BENCHMARK="\$(abs_top_builddir)/src/groonga-benchmark"
AC_SUBST(GROONGA_BENCHMARK)

# groonga-suggest-create-dataset binary path
GROONGA_SUGGEST_CREATE_DATASET="\$(abs_top_builddir)/src/suggest/groonga-suggest-create-dataset"
AC_SUBST(GROONGA_SUGGEST_CREATE_DATASET)

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
  AC_DEFINE(WITH_CUTTER, 1, [Define to 1 if you use Cutter])

  ac_cv_have_libmemcached=no
  AC_CHECK_HEADER(libmemcached/memcached.h,
                  [AC_SEARCH_LIBS(memcached_increment_with_initial, memcached,
                                  [ac_cv_have_libmemcached=yes])])
  AM_CONDITIONAL([WITH_LIBMEMCACHED],
                 [test "$ac_cv_have_libmemcached" = "yes"])
  if test "$ac_cv_have_libmemcached" = "yes"; then
    AC_DEFINE(WITH_LIBMEMCACHED, 1, [Define to 1 if you use libmemcached])
  fi
else
  AM_CONDITIONAL([WITH_LIBMEMCACHED], false)
fi

# check for benchmark
AC_ARG_ENABLE(benchmark,
  [AS_HELP_STRING([--disable-benchmark],
    [don't build benchmark programs.])],,
  [enable_benchmark="yes"])
if test "x$enable_benchmark" = "xno"; then
  benchmark_available="no"
else
  REQUIRED_GLIB_VERSION=2.8.0
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
  AC_DEFINE(WITH_BENCHMARK, 1, [Define to 1 if benchamrk is available])
fi
AM_CONDITIONAL([WITH_BENCHMARK], [test "$benchmark_available" = "yes"])

# check Ruby for HTTP test
ac_cv_ruby_available="no"
AC_ARG_WITH([ruby19],
            AS_HELP_STRING([--with-ruby19=PATH],
                           [Ruby 1.9 interpreter path (default: no)]),
            [RUBY="$withval"],
            [RUBY="no"])

if test "x$RUBY" = "xno"; then
  RUBY=
else
  if test "x$RUBY" = "xyes"; then
    AC_PATH_PROGS(RUBY, [ruby1.9 ruby19 ruby1.9.1 ruby], ruby19-not-found)
    if test "$RUBY" != "ruby19-not-found"; then
      ruby_version="`$RUBY --version`"
      if echo "$ruby_version" | grep -q -- 'ruby 1\.9'; then
        ac_cv_ruby_available="yes"
      else
        AC_MSG_WARN([$RUBY isn't Ruby 1.9 ($ruby_version)])
      fi
    fi
  else
    ruby_not_found_warning_message="$RUBY is not found. Disable HTTP test."
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

AM_CONDITIONAL([WITH_FUNCTION_TEST],
               [test "$ac_cv_ruby_available" = "yes"])

# check Inkscape for generating PNG images
inkscape_available="no"
AC_ARG_WITH([inkscape],
            AS_HELP_STRING([--with-inkscape=PATH],
                           [Inkscape path (default: auto)]),
            [INKSCAPE="$withval"],
            [INKSCAPE="yes"])

if test "x$INKSCAPE" = "xno"; then
  INKSCAPE=
else
  if test "x$INKSCAPE" = "xyes"; then
    AC_PATH_PROGS(INKSCAPE, [inkscape], none)
    if test "$INKSCAPE" != "none"; then
      inkscape_available="yes"
    fi
  else
    AC_CHECK_FILE([$INKSCAPE],
		  [inkscape_available="yes"],
		  [AC_MSG_WARN([$INKSCAPE is not found.
                                Disable PNG image generation.])])
  fi
fi
AC_SUBST(INKSCAPE)
AM_CONDITIONAL([WITH_INKSCAPE], [test "$inkscape_available" = "yes"])

# libedit
AC_ARG_ENABLE(libedit,
  [AS_HELP_STRING([--disable-libedit],
    [use libedit for console. [default=auto-detect]])],
  [enable_libedit="$enableval"],
  [enable_libedit="auto"])
if test "x$enable_libedit" != "xno"; then
  m4_ifdef([PKG_CHECK_MODULES], [
    PKG_CHECK_MODULES([LIBEDIT], [libedit >= 3.0],
      [LIBS_SAVE="$LIBS"
       LDFLAGS_SAVE="$LDFLAGS"
       LDFLAGS="$LIBEDIT_LIBS $LDFLAGS"
       AC_SEARCH_LIBS(el_wline, edit,
                      [libedit_available=yes],
                      [libedit_available=no])
       LIBS="$LIBS_SAVE"
       LDFLAGS="$LDFLAGS_SAVE"],
      [libedit_available=no])
    ],
    [libedit_available=no])
  if test "x$libedit_available" = "xyes"; then
    AC_DEFINE(WITH_LIBEDIT, [1], [Use libedit with multibyte support.])
  else
    if test "x$enable_editline" = "xyes"; then
      AC_MSG_ERROR("No libedit found")
    fi
  fi
fi

# zlib
AC_ARG_WITH(zlib,
  [AS_HELP_STRING([--with-zlib],
    [use zlib for data compression. [default=no]])],
  [with_zlib="$withval"],
  [with_zlib="no"])
if test "x$with_zlib" = "xyes"; then
  AC_DEFINE(WITH_ZLIB, [1], [with zlib])
  AC_SEARCH_LIBS(compress, z, [], [AC_MSG_ERROR("No libz found")])
fi

# LZO
AC_ARG_WITH(lzo,
  [AS_HELP_STRING([--with-lzo],
    [use LZO for data compression. [default=no]])],
  [with_lzo="$withval"],
  [with_lzo="no"])
if test "x$with_lzo" = "xyes"; then
  AC_DEFINE(WITH_LZO, [1], [with lzo])
  AC_SEARCH_LIBS(lzo1_compress, lzo2, [], [AC_MSG_ERROR("No liblzo2 found")])
fi

# MeCab
# NOTE: MUST be checked last
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
  AC_DEFINE(WITH_MECAB, [1], [use MeCab])
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
  AC_DEFINE(WITH_KYTEA, [1], [use KyTea])
fi
AM_CONDITIONAL(WITH_KYTEA, test "x$with_kytea" = "xyes")

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
    AC_DEFINE(WITH_ZEROMQ, [1], [Define to 1 if ZeroMQ is available.])
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
    AC_DEFINE(WITH_LIBEVENT, [1], [Define to 1 if libevent is available.])
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
AC_ARG_WITH(message-pack,
  [AS_HELP_STRING([--without-message-pack],
    [Disable MessagePack used for suggestion. [default=/usr]])],
  [with_message_pack="$withval"],
  [with_message_pack="/usr"])
if test "x$with_message_pack" != "xno"; then
  _SAVE_CFLAGS="$CFLAGS"
  _SAVE_LDFLAGS="$LDFLAGS"
  _SAVE_LIBS="$LIBS"
  CFLAGS="$CFLAGS -I$with_message_pack/include"
  LDFLAGS="$LDFLAGS -L$with_message_pack/lib"
  AC_SEARCH_LIBS(msgpack_version, msgpack,
                 [message_pack_available=yes],
                 [message_pack_available=no])
  CFLAGS="$_SAVE_CFLAGS"
  LDFLAGS="$_SAVE_LDFLAGS"
  LIBS="$_SAVE_LIBS"
  if test "x$message_pack_available" = "xyes"; then
    AC_DEFINE(WITH_MESSAGE_PACK, [1], [Define to 1 if MessagePack is available.])
    MESSAGE_PACK_CFLAGS="-I$with_message_pack/include"
    MESSAGE_PACK_LIBS="-L$with_message_pack/lib -lmsgpack"
  else
    if test "x$with_message_pack" = "xyes"; then
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
    AC_PATH_PROG(HG, hg, [])
    if test -n "$HG"; then
      document_available=yes
      document_buildable=yes
    else
      AC_MSG_ERROR("No hg found")
    fi
    AC_SUBST(HG)
  fi
fi

AM_CONDITIONAL([WITH_HG], [test -n "${HG}"])

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
     (supported package platforms: redhat, fedora)])],
  [package_platform="$withval"],
  [package_platform="no"])
AC_MSG_RESULT($package_platform)

AM_CONDITIONAL([REDHAT_PLATFORM],
               [test "${package_platform}" = "redhat"])
AM_CONDITIONAL([FEDORA_PLATFORM],
               [test "${package_platform}" = "fedora"])

# plugins check
relative_pluginsdir="\$(PACKAGE)/plugins"
AC_SUBST(relative_pluginsdir)

pluginsdir="\${libdir}/\$(relative_pluginsdir)"
AC_SUBST(pluginsdir)

tokenizer_pluginsdir="\${pluginsdir}/tokenizers"
AC_SUBST(tokenizer_pluginsdir)

suggest_pluginsdir="\${pluginsdir}/suggest"
AC_SUBST(suggest_pluginsdir)

table_pluginsdir="\${pluginsdir}/table"
AC_SUBST(table_pluginsdir)

AC_MSG_CHECKING(for the suffix of plugin shared libraries)
shrext_cmds=$(./libtool --config | grep '^shrext_cmds=')
eval $shrext_cmds
module=yes eval suffix="$shrext_cmds"
AC_MSG_RESULT($suffix)
if test -z "$suffix"; then
  AC_MSG_ERROR([can't detect plugin suffix])
fi
AC_DEFINE_UNQUOTED(GRN_PLUGIN_SUFFIX, ["$suffix"], "plugin suffix")

# for examples
examplesdir="\$(pkgdatadir)/examples"
AC_SUBST(examplesdir)

examples_dictionarydir="\$(examplesdir)/dictionary"
AC_SUBST(examples_dictionarydir)

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
GRN_DEFS="$GRN_DEFS -DGRN_RELATIVE_PLUGINS_DIR=\\\"\"lib/\$(relative_pluginsdir)\"\\\""
GRN_DEFS="$GRN_DEFS -DGRN_LOG_PATH=\\\"\"\$(grn_log_path)\"\\\""
GRN_DEFS="$GRN_DEFS -DGRN_DEFAULT_DOCUMENT_ROOT=\\\"\"\$(GRN_DEFAULT_DOCUMENT_ROOT)\"\\\""
GRN_DEFS="$GRN_DEFS -DGRN_DEFAULT_RELATIVE_DOCUMENT_ROOT=\\\"\"\$(GRN_DEFAULT_RELATIVE_DOCUMENT_ROOT)\"\\\""
AC_SUBST(GRN_DEFS)
CFLAGS="$CFLAGS $OPT_CFLAGS "
LIBS="$LIBS $WINDOWS_LIBS"
AC_DEFINE_UNQUOTED(CONFIGURE_OPTIONS, "$ac_configure_args", "specified configure options")

# For groonga.org
AC_ARG_WITH(groonga-github-com-path,
            [AS_HELP_STRING([--with-groonga-github-com-path=PATH],
            [specify groonga.github.com path to update groonga.org.])],
            [GROONGA_GITHUB_COM_PATH="$withval"],
            [GROONGA_GITHUB_COM_PATH=""])
AC_SUBST(GROONGA_GITHUB_COM_PATH)

# nginx
AC_ARG_ENABLE(nginx,
  [AS_HELP_STRING([--enable-nginx],
    [enable nginx used for groonga-httpd. [default=yes]])],
  [enable_nginx="$enableval"],
  [enable_nginx="auto"])
if test "x$enable_nginx" != "xno"; then
  AC_CONFIG_SUBDIRS([vendor/nginx])
fi

# For package
AC_ARG_WITH(rsync-path,
            [AS_HELP_STRING([--with-rsync-path=PATH],
            [specify rsync path to upload groonga packages.])],
            [RSYNC_PATH="$withval"],
            [RSYNC_PATH="packages@packages.groonga.org:public"])
AC_SUBST(RSYNC_PATH)

GPG_UID=m4_include(gpg_uid)
AC_SUBST(GPG_UID)

pkgsysconfdir="\${sysconfdir}/$PACKAGE_NAME"
AC_SUBST(pkgsysconfdir)

GRN_CONFIG_PATH="`
  test \"$prefix\" = NONE && prefix=
  eval echo ${sysconfdir}/groonga/groonga.conf
`"
AC_DEFINE_UNQUOTED(GRN_CONFIG_PATH, ["$GRN_CONFIG_PATH"],
  [Default command line option configuration file.])

GROONGA_HTTPD_DOCUMENT_ROOT="`
  test \"$prefix\" = NONE && prefix=/usr/local
  eval eval eval echo ${GRN_EXPANDED_DEFAULT_DOCUMENT_ROOT}
`"
AC_SUBST(GROONGA_HTTPD_DOCUMENT_ROOT)

AC_OUTPUT([
  packages/rpm/centos/groonga.spec
  packages/rpm/fedora/groonga.spec
  packages/apt/debian/groonga-keyring.postrm
  groonga.pc
  groonga-httpd-conf.sh
  data/groonga-httpd.conf
  ])

echo "$PACKAGE_NAME $PACKAGE_VERSION configuration:"
echo "-----------------------"
echo "  Compiler:              ${CC}"
echo "  CFLAGS:                ${CFLAGS}"
echo "  CXXFLAGS:              ${CXXFLAGS}"
echo "  Libraries:             ${LIBS}"
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
echo

echo "Tools:"
# echo "  Sphinx:                ${enable_sphinx}"
echo "  hg:                    ${HG}"
echo "  Ruby:                  ${RUBY}"
echo "  Cutter:                ${CUTTER}"
echo

echo "For packages:"
echo "  rsync path:            ${RSYNC_PATH}"
echo "  GPG UID:               ${GPG_UID}"
echo

echo "Now type 'make' to build $PACKAGE_NAME $PACKAGE_VERSION!"
