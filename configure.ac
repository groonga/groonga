AC_PREREQ(2.59)
AC_INIT([groonga],[0.7.0],[groonga@razil.jp])
AM_CONFIG_HEADER(config.h)

AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)

AC_CANONICAL_HOST
AC_DEFINE_UNQUOTED(HOST_CPU, ["$host_cpu"], [host CPU])
AC_DEFINE_UNQUOTED(HOST_OS, ["$host_os"], [host OS])

AC_MSG_CHECKING([for native Win32])
case "$host" in
  *-*-mingw*)
    os_win32=yes
    ;;
  *)
    os_win32=no
    ;;
esac
AC_MSG_RESULT([$os_win32])

AC_MSG_CHECKING([for some Win32 platform])
case "$host" in
  *-*-mingw*|*-*-cygwin*)
    platform_win32=yes
    ;;
  *)
    platform_win32=no
    ;;
esac
AC_MSG_RESULT([$platform_win32])

AM_CONDITIONAL(OS_WIN32, test "$os_win32" = "yes")
AM_CONDITIONAL(PLATFORM_WIN32, test "$platform_win32" = "yes")

AC_C_BIGENDIAN
AC_PROG_CC
AM_PROG_CC_C_O
AC_DEFUN([CHECK_CFLAG], [
  old_CFLAGS=$CFLAGS
  CFLAGS="$CFLAGS $1 -Werror"
  AC_COMPILE_IFELSE([[
#include <stdio.h>
int main(int argc, char **argv)
{
  printf("test cflags, argc:%d argv[0]:%s", argc, argv[0]);
  return 0;
}
    ]],
    [check_cflag=yes],
    [check_cflag=no])
  CFLAGS="$old_CFLAGS"
  if test "x$check_cflag" = "xyes"; then
    CFLAGS="$CFLAGS $1"
  fi
])
if test "$GCC" = "yes"; then
  CHECK_CFLAG([-Wall])
  CHECK_CFLAG([-Wextra])
  if test "x$check_cflag" = "xno"; then
    CHECK_CFLAG([-W])
  fi
  CHECK_CFLAG([-Wno-unused-parameter])
  CHECK_CFLAG([-Wno-sign-compare])
  CHECK_CFLAG([-Wno-pointer-sign])
  CHECK_CFLAG([-Wno-missing-field-initializers])

  CHECK_CFLAG([-Wformat=2])
  CHECK_CFLAG([-Wstrict-aliasing=2])
  CHECK_CFLAG([-Wdisabled-optimization])
  CHECK_CFLAG([-Wfloat-equal])
  CHECK_CFLAG([-Wpointer-arith])
  CHECK_CFLAG([-Wdeclaration-after-statement])
  CHECK_CFLAG([-Wbad-function-cast])
  CHECK_CFLAG([-Wcast-align])
  CHECK_CFLAG([-Wredundant-decls])
#  CHECK_CFLAG([-Wunsafe-loop-optimizations])
#  CHECK_CFLAG([-Wunreachable-code])
#  CHECK_CFLAG([-Wswitch-enum])
#  CHECK_CFLAG([-Wshadow])
#  CHECK_CFLAG([-Wconversion])
#  CHECK_CFLAG([-Wwrite-strings])
#  CHECK_CFLAG([-Winline])
fi
AC_LIBTOOL_WIN32_DLL
AM_PROG_LIBTOOL
AC_CONFIG_FILES([Makefile
                 src/Makefile
                 lib/Makefile
                 modules/Makefile
                 modules/tokenizers/Makefile
                 modules/functions/Makefile
                 examples/Makefile
                 apt/Makefile
                 test/Makefile
                 test/unit/Makefile
                 test/unit/lib/Makefile
                 test/unit/lib/ruby/Makefile
                 test/unit/fixtures/Makefile
                 test/unit/fixtures/inverted-index/Makefile
                 test/unit/fixtures/stress/Makefile
                 test/unit/fixtures/performance/Makefile
                 test/unit/util/Makefile
                 test/unit/core/Makefile
                 test/unit/memcached/Makefile
                 test/unit/http/Makefile
                 test/benchmark/Makefile
                 test/benchmark/lib/Makefile
                ])

AC_CHECK_HEADERS(sys/mman.h sys/time.h sys/timeb.h sys/param.h sys/types.h pthread.h sys/resource.h)
AC_CHECK_HEADERS(netdb.h sys/wait.h sys/socket.h netinet/in.h netinet/tcp.h)
AC_CHECK_HEADERS(ucontext.h signal.h errno.h execinfo.h)
AC_CHECK_FUNCS(localtime_r)
AC_SYS_LARGEFILE
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_CHECK_SIZEOF(off_t)

AC_MSG_CHECKING([for fpclassify])
AC_COMPILE_IFELSE([[
#define _ISOC99_SOURCE
#include <math.h>

int main(int argc, char **argv)
{
  return fpclassify (0.0);
}
    ]],
    [
      AC_MSG_RESULT(yes)
      AC_DEFINE(HAVE_FPCLASSIFY, [1], [use fpclassify])
    ],
    [
      AC_MSG_RESULT(no)
    ])

# log path
AC_ARG_WITH(log_path,
  [AS_HELP_STRING([--with-log-path=PATH],
    [specify groonga log path.])],
  GROONGA_LOG_PATH="$withval",
  GROONGA_LOG_PATH="/var/log/$PACKAGE_NAME/$PACKAGE_NAME.log")
AC_DEFINE_UNQUOTED(GROONGA_LOG_PATH, "$GROONGA_LOG_PATH", "specified log directory")

# default encoding
AC_ARG_WITH(default_encoding,
  [AS_HELP_STRING([--with-default-encoding=ENCODING],
    [specify groonga default encoding(euc/sjis/utf8/latin1/koi8r/none)])],
  GROONGA_DEFAULT_ENCODING="$withval",
  GROONGA_DEFAULT_ENCODING="utf8")
AC_DEFINE_UNQUOTED(GROONGA_DEFAULT_ENCODING, "$GROONGA_DEFAULT_ENCODING", "specified default encoding")

# default query escalation threshold
AC_ARG_WITH(query_escalation_threshold,
  [AS_HELP_STRING([--with-query-escalation-threshold=NUMBER],
    [specify groonga default query escalation threshold])],
  GROONGA_DEFAULT_QUERY_ESCALATION_THRESHOLD="$withval",
  GROONGA_DEFAULT_QUERY_ESCALATION_THRESHOLD="0")
AC_DEFINE_UNQUOTED(GROONGA_DEFAULT_QUERY_ESCALATION_THRESHOLD, $GROONGA_DEFAULT_QUERY_ESCALATION_THRESHOLD, "specified query escalation threshold")

AC_CHECK_LIB(m, log, [M_LIBS="-lm"], [AC_MSG_ERROR("No libm found")])
AC_CHECK_LIB(pthread, pthread_mutex_init,
             [PTHREAD_LIBS="-lpthread"],
             [test "$os_win32" != "yes" && AC_MSG_ERROR("No libpthread found")])
AC_CHECK_LIB(nsl, gethostbyname, [NSL_LIBS="-lnsl"])
AC_CHECK_LIB(socket, socket, [SOCKET_LIBS="-lsocket"])
AC_CHECK_LIB(dl, dlopen, [NSL_LIBS="-ldl"])

# aio
AC_ARG_ENABLE(aio,
  [AS_HELP_STRING([--enable-aio],
    [use aio/dio based cache. [default=no]])],,
  [enable_aio="no"])
if test "x$enable_aio" != "xno"; then
 	AC_CHECK_LIB(rt, nanosleep, [], [AC_MSG_ERROR("No librt found")])
  AC_DEFINE(USE_AIO, [1], [use AIO/DIO])
fi

# nfkc
AC_ARG_ENABLE(nfkc,
  [AS_HELP_STRING([--enable-nfkc],
    [use nfkc based utf8 normalization. [default=yes]])],,
  [enable_nfkc="yes"])
if test "x$enable_nfkc" != "xyes"; then
  AC_DEFINE(NO_NFKC, [1], [compile without nfkc.c])
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
if test "x$enable_uyield" != "xno"; then
  AC_DEFINE(USE_UYIELD, [1], [use uyield])
fi
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
AC_COMPILE_IFELSE([[
#include <sys/types.h>
#include <sys/socket.h>

int main(int argc, char **argv)
{
  return MSG_MORE;
}
    ]],
    [
      AC_MSG_RESULT(yes)
      AC_DEFINE(USE_MSG_MORE, [1], [use MSG_MORE])
    ],
    [
      AC_MSG_RESULT(no)
    ])

# check MSG_NOSIGNAL defined
AC_MSG_CHECKING([whether MSG_NOSIGNAL defined])
AC_COMPILE_IFELSE([[
#include <sys/types.h>
#include <sys/socket.h>

int main(int argc, char **argv)
{
  return MSG_NOSIGNAL;
}
    ]],
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
GROONGA="\$(abs_top_srcdir)/src/groonga"
AC_SUBST(GROONGA)

# check Cutter with GLib support if available
REQUIRED_MINIMUM_CUTTER_VERSION=1.1.3
m4_ifdef([AC_CHECK_GCUTTER], [
AC_CHECK_GCUTTER(>= $REQUIRED_MINIMUM_CUTTER_VERSION)
],
[ac_cv_use_cutter="no"])

AM_CONDITIONAL([WITH_CUTTER], [test "$ac_cv_use_cutter" != "no"])
if test "$ac_cv_use_cutter" != "no"; then
  AC_DEFINE(WITH_CUTTER, 1, [Define to 1 if you use Cutter])

  ac_cv_have_libmemcached=no
  AC_CHECK_HEADER(libmemcached/memcached.h,
                  [AC_CHECK_LIB(memcached, memcached_increment_with_initial,
                                [ac_cv_have_libmemcached=yes])])
  AM_CONDITIONAL([WITH_LIBMEMCACHED],
                 [test "$ac_cv_have_libmemcached" != "no"])
  if test "$ac_cv_have_libmemcached" = "yes"; then
    AC_DEFINE(WITH_LIBMEMCACHED, 1, [Define to 1 if you use libmemcached])
  fi

  m4_ifdef([AC_CHECK_SOUPCUTTER], [
  AC_CHECK_SOUPCUTTER(>= $REQUIRED_MINIMUM_CUTTER_VERSION)
  ],
  [ac_cv_use_soupcutter="no"])
  AM_CONDITIONAL([WITH_SOUPCUTTER],
                 [test "$ac_cv_use_soupcutter" = "yes"])
  if test "$ac_cv_use_soupcutter" = "yes"; then
    AC_DEFINE(WITH_SOUPCUTTER, 1, [Define to 1 if you use soupcutter])
  fi
else
  AM_CONDITIONAL([WITH_LIBMEMCACHED], false)
  AM_CONDITIONAL([WITH_SOUPCUTTER], false)
fi

# check GLib for benchmark
AC_MSG_NOTICE(chcking if benchmark is available)
m4_ifdef([AM_PATH_GLIB_2_0], [
GLIB_REQUIRED=2.8.0
AC_SUBST(GLIB_REQUIRED)
AM_PATH_GLIB_2_0($GLIB_REQUIRED,
                 [ac_cv_benchmark_available="yes"],
                 [ac_cv_benchmark_available="no"],
                 [gobject gthread])
],
[ac_cv_benchmark_available="no"])
if test "$ac_cv_benchmark_available" = "yes"; then
  AC_MSG_NOTICE(benchmark is available)
else
  AC_MSG_NOTICE(benchmark isn't available)
fi

AM_CONDITIONAL([WITH_BENCHMARK], [test "$ac_cv_benchmark_available" = "yes"])
if test "$ac_cv_benchmark_available" = "yes"; then
  AC_DEFINE(WITH_BENCHMARK, 1, [Define to 1 if benchamrk is available])
fi

# check Ruby for HTTP test
ac_cv_ruby_available="no"
AC_ARG_WITH([ruby],
            AS_HELP_STRING([--with-ruby=PATH],
                           [Ruby interpreter path (default: auto-detect)]),
            [RUBY="$withval"])

if test "$RUBY" = "no"; then
  : # ignore
elif test "$RUBY" = ""; then
  AC_PATH_PROG(RUBY, ruby, none)
  if test "$RUBY" != "none"; then
    ac_cv_ruby_available="yes"
  fi
else
  AC_CHECK_FILE([$RUBY],
                [ac_cv_ruby_available="yes"],
                [AC_MSG_WARN([$RUBY is not found. Disable HTTP test.])])
fi
AC_SUBST(RUBY)
AM_CONDITIONAL([WITH_RUBY], [test "$ac_cv_ruby_available" = "yes"])

# zlib
AC_ARG_WITH(zlib,
  [AS_HELP_STRING([--with-zlib],
    [use zlib for data compression. [default=no]])],
  [with_zlib="$withval"],
  [with_zlib="no"])
if test "x$with_zlib" = "xno"; then
  AC_DEFINE(NO_ZLIB, [1], [without zlib])
else
  AC_CHECK_LIB(z, compress, [ZLIB_LIBS="-lz"], [AC_MSG_ERROR("No libz found")])
fi

# lzo
AC_ARG_WITH(lzo,
  [AS_HELP_STRING([--with-lzo],
    [use lzo for data compression. [default=no]])],
  [with_lzo="$withval"],
  [with_lzo="no"])
if test "x$with_lzo" = "xno"; then
  AC_DEFINE(NO_LZO, [1], [without lzo])
else
  AC_CHECK_LIB(lzo2, lzo1_compress, [LZO_LIBS="-llzo2"], [AC_MSG_ERROR("No liblzo2 found")])
fi

# mecab
# NOTE: MUST be checked last
AC_ARG_WITH(mecab,
  [AS_HELP_STRING([--with-mecab],
    [use mecab for morphological analysis. [default=yes]])],
  [with_mecab="$withval"],
  [with_mecab="yes"])
AC_MSG_CHECKING([whether enable MeCab])
AC_MSG_RESULT($with_mecab)
if test "x$with_mecab" = "xno"; then
  AC_DEFINE(NO_MECAB, [1], [without MeCab])
else
  # mecab-config
  AC_ARG_WITH(mecab-config,
    [AS_HELP_STRING([--with-mecab-config=PATH],
      [set mecab-config location. [default=auto-detect]])],
    [AC_CHECK_FILE("$withval", MECAB_CONFIG="$withval", MECAB_CONFIG=no)],
    [AC_PATH_PROG(MECAB_CONFIG, mecab-config, no)])
  if test "x$MECAB_CONFIG" = "xno"; then
    AC_DEFINE(NO_MECAB, [1], [without MeCab])
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
    CPPFLAGS="$_SAVE_CPPFLAGS"
    AC_SUBST(MECAB_CPPFLAGS)
    AC_SUBST(MECAB_LDFLAGS)
    AC_SUBST(MECAB_LIBS)
  fi
fi

# futex check
AC_ARG_ENABLE(futex,
  [AS_HELP_STRING([--enable-futex],
    [use futex. [default=no]])],
  ,
  [enable_futex="no"])
if test "x$enable_futex" != "xno"; then
  AC_CHECK_HEADERS(linux/futex.h sys/syscall.h, [
    AC_DEFINE(USE_UYIELD, [1], [use uyield])
  ], [
    AC_MSG_ERROR("linux/futex.h or sys/syscall.h not found")
  ])
fi
AC_MSG_CHECKING([whether enable futex])
AC_MSG_RESULT($enable_futex)

# modules check
modulesdir="\${libdir}/\$(PACKAGE)/modules"
AC_SUBST(modulesdir)

tokenizer_modulesdir="\${modulesdir}/tokenizers"
AC_SUBST(tokenizer_modulesdir)

function_modulesdir="\${modulesdir}/functions"
AC_SUBST(function_modulesdir)

AC_MSG_CHECKING(for the suffix of module shared libraries)
suffix=$(./libtool --config | grep '^shrext_cmds=' | sed -e 's/shrext_cmds=//')
suffix=$(echo "$suffix" | sed -e "s/'/\"/g")
AC_MSG_RESULT($suffix)
AC_DEFINE_UNQUOTED(GRN_MODULE_SUFFIX, [$suffix], "module suffix")


# flags for compile groonga
CFLAGS="$CFLAGS $OPT_CFLAGS "
CFLAGS="$CFLAGS -DTOKENIZER_MODULES_DIR=\\\"\"\$(tokenizer_modulesdir)\"\\\""
CFLAGS="$CFLAGS -DFUNCTION_MODULES_DIR=\\\"\"\$(function_modulesdir)\"\\\""
LIBS="$LIBS $ZLIB_LIBS $LZO_LIBS $PTHREAD_LIBS $M_LIBS $NSL_LIBS $SOCKET_LIBS $WINDOWS_LIBS"
AC_DEFINE_UNQUOTED(CONFIGURE_OPTIONS, "$ac_configure_args", "specified configure options")

# flags for groonga-cfg
#GROONGA_LIBDIR='-L${libdir}'
#GROONGA_INCLUDEDIR="-I${includedir}/groonga"
#GROONGA_CFLAGS=""
#GROONGA_LIBS="-lgroonga $PTHREAD_LIBS"

#AC_SUBST(GROONGA_LIBDIR)
#AC_SUBST(GROONGA_INCLUDEDIR)
#AC_SUBST(GROONGA_CFLAGS)
#AC_SUBST(GROONGA_LIBS)

# For Debian package release
AC_ARG_WITH(rsync-path,
            [AS_HELP_STRING([--with-rsync-path=PATH],
            [specify rsync path to upload groonga Debian packages.])],
            [RSYNC_PATH="$withval"],
            [RSYNC_PATH=""])
AC_SUBST(RSYNC_PATH)

AC_OUTPUT([
  groonga.spec
  groonga.pc
  ])

echo "
$PACKAGE_NAME $PACKAGE_VERSION configuration:
-----------------------
  Compiler:              ${CC}
  CFLAGS:                ${CFLAGS}
  Libraries:             ${LIBS}

  Install path prefix:   ${prefix}

Now type 'make' to build $PACKAGE_NAME $PACKAGE_VERSION!
"
