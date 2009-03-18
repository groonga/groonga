AC_INIT([groonga],0.0.2,[senna@razil.jp])
AM_CONFIG_HEADER(config.h)

AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)

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
  CHECK_CFLAG([-Winline])
  CHECK_CFLAG([--param max-inline-insns-single=12000])
  CHECK_CFLAG([--param large-function-growth=28000])
  CHECK_CFLAG([--param inline-unit-growth=400])
fi
AM_PROG_LIBTOOL
AC_CONFIG_FILES([Makefile
                 src/Makefile
                 lib/Makefile
                 examples/Makefile
                 test/Makefile
		 test/unit/Makefile
		 test/unit/lib/Makefile
		 test/unit/fixtures/Makefile
		 test/unit/fixtures/inverted-index/Makefile
		 test/unit/fixtures/stress/Makefile
		 test/unit/fixtures/performance/Makefile
		 test/unit/util/Makefile
		 test/unit/core/Makefile
		 test/ql/Makefile
		 test/benchmark/Makefile
		 test/benchmark/lib/Makefile
                ])

AC_CHECK_HEADERS(sys/mman.h sys/time.h sys/param.h sys/types.h pthread.h sys/resource.h)
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

# will deprecated
AC_ARG_WITH(groongahome,
  [AC_HELP_STRING([--with-groongahome=DIR],
    [specify Groonga home directory])],
  GROONGA_HOME="$withval",
  GROONGA_HOME="${prefix}/lib/$PACKAGE_NAME/$PACKAGE_VERSION")
AC_DEFINE_UNQUOTED(GROONGA_HOME, "$GROONGA_HOME", "specified home directory")

# log directory
AC_ARG_WITH(log,
  [AC_HELP_STRING([--with-log=PATH],
    [specify Groonga log path.])],
  GROONGA_LOG_PATH="$withval",
  GROONGA_LOG_PATH="/var/log/$PACKAGE_NAME/$PACKAGE_NAME.log")
AC_DEFINE_UNQUOTED(GROONGA_LOG_PATH, "$GROONGA_LOG_PATH", "specified log directory")

# default encoding
AC_ARG_WITH(encoding,
  [AC_HELP_STRING([--with-encoding=ENCODING],
    [specify Groonga default encoding(euc/sjis/utf8/latin1/koi8r/none)])],
  GROONGA_DEFAULT_ENCODING="$withval",
  GROONGA_DEFAULT_ENCODING="euc_jp")
AC_DEFINE_UNQUOTED(GROONGA_DEFAULT_ENCODING, "$GROONGA_DEFAULT_ENCODING", "specified default encoding")

# default query escalation threshold
AC_ARG_WITH(query_escalation_threshold,
  [AC_HELP_STRING([--with-query-escalation-threshold=NUMBER],
    [specify Groonga default query escalation threshold])],
  GROONGA_DEFAULT_QUERY_ESCALATION_THRESHOLD="$withval",
  GROONGA_DEFAULT_QUERY_ESCALATION_THRESHOLD="0")
AC_DEFINE_UNQUOTED(GROONGA_DEFAULT_QUERY_ESCALATION_THRESHOLD, $GROONGA_DEFAULT_QUERY_ESCALATION_THRESHOLD, "specified query escalation threshold")

AC_CHECK_LIB(m, log, [M_LIBS="-lm"], [AC_MSG_ERROR("No libm found")])
AC_CHECK_LIB(pthread, pthread_mutex_init, [PTHREAD_LIBS="-lpthread"], [AC_MSG_ERROR("No libpthread found")])
AC_CHECK_LIB(nsl, gethostbyname, [NSL_LIBS="-lnsl"])
AC_CHECK_LIB(socket, socket, [SOCKET_LIBS="-lsocket"])
AC_CHECK_LIB(dl, dlopen, [NSL_LIBS="-ldl"])

# aio
AC_ARG_ENABLE(aio,
  [AC_HELP_STRING([--enable-aio],
    [use aio/dio based cache. [default=no]])],,
  [enable_aio="no"])
if test "x$enable_aio" != "xno"; then
 	AC_CHECK_LIB(rt, nanosleep, [], [AC_MSG_ERROR("No librt found")])
  AC_DEFINE(USE_AIO, [1], [use AIO/DIO])
fi

# nfkc
AC_ARG_ENABLE(nfkc,
  [AC_HELP_STRING([--enable-nfkc],
    [use nfkc based utf8 normalization. [default=yes]])],,
  [enable_nfkc="yes"])
if test "x$enable_nfkc" != "xyes"; then
  AC_DEFINE(NO_NFKC, [1], [compile without nfkc.c])
fi

# coverage
m4_ifdef([AC_CHECK_COVERAGE], [AC_CHECK_COVERAGE])
GENHTML_OPTIONS="--title 'Groonga Code Coverage'"

## malloc
force_enable_dynamic_malloc_change="no"

# failmalloc
AC_MSG_CHECKING([whether enable fmalloc])
AC_ARG_ENABLE(fmalloc,
  [AC_HELP_STRING([--enable-fmalloc],
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
  [AC_HELP_STRING([--enable-abort],
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
  [AC_HELP_STRING([--enable-dynamic-malloc-change],
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

# libmyigroonga
AC_ARG_WITH(mysqlsrc,
  [AC_HELP_STRING([--with-mysqlsrc=DIR],
    [specify MySQL source dictionary.])],
  MYSQL_SRCDIR="$withval")
AC_SUBST(MYSQL_SRCDIR)

# epoll/poll/select check
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
  AC_CHECK_HEADER(sys/poll.h, [
    AC_CHECK_FUNC(poll, [
      have_poll="yes"
      AC_DEFINE(USE_POLL, [1], [use poll])
    ])
  ])
  if test "x$have_poll" != "xyes"; then
    AC_CHECK_FUNC(select, [
      have_select="yes"
      AC_CHECK_HEADERS(sys/select.h)
      AC_DEFINE(USE_SELECT, [1], [use select])
    ],[
      AC_MSG_ERROR([epoll/poll/select is missing.])
    ])
  fi
fi

# check MSG_NOSIGNAL defined
AC_MSG_CHECKING([whether MSG_NOSIGNAL defined])
AC_COMPILE_IFELSE([[
#include <sys/types.h>
#include <sys/socket.h>

#if __FreeBSD__ >= 2 && __FreeBSD_version >= 600020
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0x20000
#endif
#endif

int main(int argc, char **argv)
{
  return MSG_NOSIGNAL;
}
    ]],
    [
      AC_MSG_RESULT(yes)
      AC_DEFINE(USE_MSG_NOSIGNAL, [1], [use MSG_SIGNAL])
    ],
    [
      AC_MSG_RESULT(no)
    ])

# check Cutter with GLib support if available
m4_ifdef([AC_CHECK_GCUTTER], [
AC_CHECK_GCUTTER
],
[ac_cv_use_cutter="no"])

AM_CONDITIONAL([WITH_CUTTER], [test "$ac_cv_use_cutter" != "no"])
if test "$ac_cv_use_cutter" != "no"; then
  AC_DEFINE(WITH_CUTTER, 1, [Define to 1 if you use Cutter])
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

# zlib
AC_ARG_WITH(zlib,
  [AC_HELP_STRING([--with-zlib],
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
  [AC_HELP_STRING([--with-lzo],
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
  [AC_HELP_STRING([--with-mecab],
    [use mecab for morphological analysis. [default=yes]])],
  [with_mecab="$withval"],
  [with_mecab="yes"])
if test "x$with_mecab" = "xno"; then
  AC_DEFINE(NO_MECAB, [1], [without MeCab])
else
  # mecab-config
  AC_ARG_WITH(mecab-config,
    [AC_HELP_STRING([--with-mecab-config=PATH],
      [set mecab-config location. [search path]])],
    [AC_CHECK_FILE("$withval", MECAB_CONFIG="$withval", MECAB_CONFIG=no)],
    [AC_PATH_PROG(MECAB_CONFIG, mecab-config, no)])
  if test "x$MECAB_CONFIG" = "xno"; then
    AC_MSG_ERROR([mecab-config is missing.])
  fi

  MECAB_CFLAGS="-I`$MECAB_CONFIG --inc-dir`"
  LDFLAGS="$LDFLAGS -L`$MECAB_CONFIG --libs-only-L`"
 	AC_CHECK_LIB(mecab, mecab_new, [], [AC_MSG_ERROR("No libmecab found")])
  CPPFLAGS="$CPPFLAGS $MECAB_CFLAGS"
  AC_CHECK_HEADER(mecab.h, , [AC_MSG_ERROR("No mecab.h found")])
fi

# flags for compile groonga
CFLAGS="$CFLAGS $MECAB_CFLAGS $OPT_CFLAGS"
LIBS="$LIBS $ZLIB_LIBS $LZO_LIBS $PTHREAD_LIBS $M_LIBS $NSL_LIBS $SOCKET_LIBS"
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

AC_OUTPUT([
  groonga.spec
  groonga.pc
  vcc/config.h
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
