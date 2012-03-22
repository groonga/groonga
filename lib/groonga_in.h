/* -*- c-basic-offset: 2 -*- */
/* Copyright(C) 2009-2012 Brazil

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef GROONGA_IN_H
#define GROONGA_IN_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef __cplusplus
#define __STDC_LIMIT_MACROS
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif /* HAVE_STDINT_H */

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif /* HAVE_SYS_PARAM_H */

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif /* HAVE_SYS_MMAN_H */

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif /* HAVE_SYS_TIME_H */
#ifdef HAVE_SYS_TIMEB_H
#include <sys/timeb.h>
#endif /* HAVE_SYS_TIMEB_H */

#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif /* HAVE_SYS_RESOURCE_H */

#ifdef WIN32
#  define GRN_API __declspec(dllexport)
#ifdef GROONGA_MAIN
#  define GRN_VAR __declspec(dllimport)
#else
#  define GRN_VAR __declspec(dllexport) extern
#endif /* GROONGA_MAIN */
#else
#  define GRN_API
#  define GRN_VAR extern
#endif

#ifdef HAVE_OPEN
# define GRN_OPEN(pathname, ...) open(pathname, __VA_ARGS__)
#else
# define GRN_OPEN(pathname, ...) _open(pathname, __VA_ARGS__)
#endif /* HAVE_OPEN */

#ifdef HAVE_CLOSE
# define GRN_CLOSE(fd) close(fd)
#else
# define GRN_CLOSE(fd) _close(fd)
#endif /* HAVE_CLOSE */

#ifdef HAVE_READ
# define GRN_READ(fd, buf, count) read(fd, buf, count)
#else
# define GRN_READ(fd, buf, count) _read(fd, buf, count)
#endif /* HAVE_READ */

#ifdef HAVE_WRITE
# define GRN_WRITE(fd, buf, count) write(fd, buf, count)
#else
# define GRN_WRITE(fd, buf, count) _write(fd, buf, count)
#endif /* HAVE_WRITE */

#ifdef WIN32

#if defined(__GNUC__) && !defined(WINVER)
#  include <w32api.h>
#  define WINVER WindowsXP
#endif /* defined(__GNUC__) && !defined(WINVER) */

#include <basetsd.h>
#include <process.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stddef.h>
#include <windef.h>
#include <float.h>
#include <time.h>
#include <sys/types.h>

#ifndef __GNUC__
#define PATH_MAX (MAX_PATH - 1)
#define inline _inline
#endif

#define snprintf _snprintf
#if _MSC_VER < 1500
  #define vsnprintf _vsnprintf
#endif /* _MSC_VER < 1500 */
#define unlink _unlink
#define lseek _lseek
#define getpid _getpid
#if !defined(__GNUC__) && _MSC_VER < 1400
# define fstat _fstat
#endif /* !defined(__GNUC__) && _MSC_VER < 1400 */
#define usleep(x) Sleep((x) / 1000)
#define sleep(x) Sleep((x) * 1000)
#if !defined(strcasecmp)
#  define strcasecmp stricmp
#endif /* !defined(strcasecmp) */

#ifdef __GNUC__
#include <stdint.h>
#else
#define uint8_t UINT8
#define int8_t INT8
#define int_least8_t INT8
#define uint_least8_t UINT8
#define int16_t INT16
#define uint16_t UINT16
#define int32_t INT32
#define uint32_t UINT32
#define int64_t INT64
#define uint64_t UINT64
#define ssize_t SSIZE_T
#define pid_t int
#endif

#undef MSG_WAITALL
#define MSG_WAITALL 0 /* before Vista, not supported... */
#define SHUT_RDWR SD_BOTH

#ifdef HAVE_FPCLASSIFY
# define CASE_FP_NAN case FP_NAN:
# define CASE_FP_INFINITE case FP_INFINITE:
#else
# define HAVE_FPCLASSIFY 1
# define fpclassify _fpclass
# define CASE_FP_NAN case _FPCLASS_SNAN: case _FPCLASS_QNAN:
# define CASE_FP_INFINITE case _FPCLASS_NINF: case _FPCLASS_PINF:
#endif /* HAVE_FPCLASSIFY */

typedef SOCKET grn_sock;
#define grn_sock_close closesocket

#define CALLBACK __stdcall

#ifndef __GNUC__
#include <intrin.h>
#include <sys/timeb.h>
#include <errno.h>
#endif
#else /* WIN32 */

#define GROONGA_API

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

# ifndef PATH_MAX
#  if defined(MAXPATHLEN)
#   define PATH_MAX MAXPATHLEN
#  else /* MAXPATHLEN */
#   define PATH_MAX 1024
#  endif /* MAXPATHLEN */
# endif /* PATH_MAX */
# ifndef INT_LEAST8_MAX
typedef char int_least8_t;
# endif /* INT_LEAST8_MAX */
# ifndef UINT_LEAST8_MAX
typedef unsigned char uint_least8_t;
# endif /* UINT_LEAST8_MAX */
typedef int grn_sock;
# define grn_sock_close close
# define CALLBACK

# ifdef HAVE_FPCLASSIFY
#  define CASE_FP_NAN case FP_NAN:
#  define CASE_FP_INFINITE case FP_INFINITE:
# else
#  if (defined(__sun) && defined(__SVR4)) /* SUN */
#   define HAVE_FPCLASSIFY 1
#   include <ieeefp.h>
#   define fpclassify fpclass
#   define CASE_FP_NAN case FP_SNAN: case FP_QNAN:
#   define CASE_FP_INFINITE case FP_NINF: case FP_PINF:
#  endif /* SUN */
# endif /* HAVE_FPCLASSIFY */

#endif /* WIN32 */

#ifndef INT8_MAX
#define INT8_MAX (127)
#endif /* INT8_MAX */

#ifndef INT8_MIN
#define INT8_MIN (-128)
#endif /* INT8_MIN */

#ifndef INT16_MAX
#define INT16_MAX (32767)
#endif /* INT16_MAX */

#ifndef INT16_MIN
#define INT16_MIN (-32768)
#endif /* INT16_MIN */

#ifndef INT32_MAX
#define INT32_MAX (2147483647)
#endif /* INT32_MAX */

#ifndef INT32_MIN
#define INT32_MIN (-2147483648)
#endif /* INT32_MIN */

#ifndef UINT32_MAX
#define UINT32_MAX (4294967295)
#endif /* UINT32_MAX */

#ifndef INT64_MAX
#define INT64_MAX (9223372036854775807)
#endif /* INT64_MAX */

#ifndef INT64_MIN
#define INT64_MIN (-9223372036854775808)
#endif /* INT64_MIN */

#ifdef HAVE_PTHREAD_H
#include <pthread.h>
typedef pthread_t grn_thread;
#define THREAD_CREATE(thread,func,arg) (pthread_create(&(thread), NULL, (func), (arg)))
#define THREAD_JOIN(thread) (pthread_join(thread, NULL))
typedef pthread_mutex_t grn_mutex;
#define MUTEX_INIT(m) pthread_mutex_init(&m, NULL)
#define MUTEX_LOCK(m) pthread_mutex_lock(&m)
#define MUTEX_UNLOCK(m) pthread_mutex_unlock(&m)
#define MUTEX_FIN(m)
typedef pthread_mutex_t grn_critical_section;
#define CRITICAL_SECTION_INIT(cs) pthread_mutex_init(&(cs), NULL)
#define CRITICAL_SECTION_ENTER(cs) pthread_mutex_lock(&(cs))
#define CRITICAL_SECTION_LEAVE(cs) pthread_mutex_unlock(&(cs))
#define CRITICAL_SECTION_FIN(cs)

typedef pthread_cond_t grn_cond;
#define COND_INIT(c) pthread_cond_init(&c, NULL)
#define COND_SIGNAL(c) pthread_cond_signal(&c)
#define COND_WAIT(c,m) pthread_cond_wait(&c, &m)

typedef pthread_key_t grn_thread_key;
#define THREAD_KEY_CREATE pthread_key_create
#define THREAD_KEY_DELETE pthread_key_delete
#define THREAD_SETSPECIFIC pthread_setspecific
#define THREAD_GETSPECIFIC pthread_getspecific

#if USE_UYIELD
  extern int grn_uyield_count;
  #define GRN_TEST_YIELD() \
    do {\
      if (((++grn_uyield_count) & (0x20 - 1)) == 0) {\
        sched_yield();\
        if(grn_uyield_count > 0x1000) {\
          grn_uyield_count = (uint32_t)time(NULL) % 0x1000;\
        }\
      }\
    } while (0)
  #undef assert
  #define assert(assert_expr) \
    do {\
      if (!(assert_expr)){\
        fprintf(stderr, "assertion failed: %s\n", #assert_expr);\
        abort();\
      }\
      GRN_TEST_YIELD();\
    } while (0)
  #define if(if_cond) \
    if ((((++grn_uyield_count) & (0x100 - 1)) != 0 || (sched_yield() * 0) == 0) && (if_cond))
  #define while(while_cond) \
    while ((((++grn_uyield_count) & (0x100 - 1)) != 0 || (sched_yield() * 0) == 0) && (while_cond))

  #if !defined(_POSIX_PRIORITY_SCHEDULING)
  #define sched_yield() usleep(1000 * 20)
  #endif
#else /* USE_UYIELD */
  #define GRN_TEST_YIELD() \
    do { \
    } while (0)
#endif /* USE_UYIELD */

#else /* HAVE_PTHREAD_H */

/* todo */
typedef int grn_thread_key;
#define THREAD_KEY_CREATE(key,destr)
#define THREAD_KEY_DELETE(key)
#define THREAD_SETSPECIFIC(key)
#define THREAD_GETSPECIFIC(key,value)

#ifdef WIN32
typedef uintptr_t grn_thread;
#define THREAD_CREATE(thread,func,arg) (((thread)=_beginthreadex(NULL, 0, (func), (arg), 0, NULL)) == NULL)
#define THREAD_JOIN(thread) (WaitForSingleObject((thread), INFINITE) == WAIT_FAILED)
typedef HANDLE grn_mutex;
#define MUTEX_INIT(m) ((m) = CreateMutex(0, FALSE, NULL))
#define MUTEX_LOCK(m) WaitForSingleObject((m), INFINITE)
#define MUTEX_UNLOCK(m) ReleaseMutex(m)
#define MUTEX_FIN(m) CloseHandle(m)
typedef CRITICAL_SECTION grn_critical_section;
#define CRITICAL_SECTION_INIT(cs) InitializeCriticalSection(&(cs))
#define CRITICAL_SECTION_ENTER(cs) EnterCriticalSection(&(cs))
#define CRITICAL_SECTION_LEAVE(cs) LeaveCriticalSection(&(cs))
#define CRITICAL_SECTION_FIN(cs) DeleteCriticalSection(&(cs))

typedef struct
{
  int waiters_count_;
  HANDLE waiters_count_lock_;
  HANDLE sema_;
  HANDLE waiters_done_;
} grn_cond;

#define COND_INIT(c) { \
  (c).waiters_count_ = 0; \
  (c).sema_ = CreateSemaphore(NULL, 0, 0x7fffffff, NULL); \
  MUTEX_INIT((c).waiters_count_lock_); \
}

#define COND_SIGNAL(c) { \
  MUTEX_LOCK((c).waiters_count_lock_); \
  { \
    int have_waiters = (c).waiters_count_ > 0; \
    MUTEX_UNLOCK((c).waiters_count_lock_); \
    if (have_waiters) { \
      ReleaseSemaphore((c).sema_, 1, 0); \
    } \
  } \
}
#define COND_WAIT(c,m) { \
  MUTEX_LOCK((c).waiters_count_lock_); \
  (c).waiters_count_++; \
  MUTEX_UNLOCK((c).waiters_count_lock_); \
  SignalObjectAndWait((m), (c).sema_, INFINITE, FALSE); \
  MUTEX_LOCK((c).waiters_count_lock_); \
  (c).waiters_count_--; \
  MUTEX_UNLOCK((c).waiters_count_lock_); \
  WaitForSingleObject((m), INFINITE); \
}

#else /* WIN32 */
/* todo */
typedef int grn_cond;
#define COND_INIT(c) ((c) = 0)
#define COND_SIGNAL(c)
#define COND_WAIT(c,m) do { MUTEX_UNLOCK(m); usleep(1000); MUTEX_LOCK(m); } while (0)
/* todo : must be enhanced! */

#endif /* WIN32 */

#define GRN_TEST_YIELD() \
  do { \
  } while (0)

#endif /* HAVE_PTHREAD_H */

/* format string for printf */
#ifdef WIN32
#define GRN_FMT_LLD "I64d"
#define GRN_FMT_LLU "I64u"
#else /* WIN32 */
#define GRN_FMT_LLD "lld"
#define GRN_FMT_LLU "llu"
#endif /* WIN32 */

#ifdef __GNUC__
# if (defined(__i386__) || defined(__x86_64__)) /* ATOMIC ADD */
#  define GRN_ATOMIC_ADD_EX(p,i,r) \
  __asm__ __volatile__ ("lock; xaddl %0,%1" : "=r"(r), "=m"(*p) : "0"(i), "m" (*p))
#  define GRN_BIT_SCAN_REV(v,r) \
  __asm__ __volatile__ ("\tmovl %1,%%eax\n\tbsr %%eax,%0\n" : "=r"(r) : "r"(v) : "%eax")
#  define GRN_BIT_SCAN_REV0(v,r) \
  __asm__ __volatile__ ("\tmovl %1,%%eax\n\tbsr %%eax,%0\n\tcmovz %%eax,%0\n" : "=r"(r) : "r"(v) : "%eax")
# elif (defined(__PPC__) || defined(__ppc__)) /* ATOMIC ADD */
#  define GRN_ATOMIC_ADD_EX(p,i,r) \
  __asm__ __volatile__ ("\n1:\n\tlwarx %0, 0, %1\n\tadd %0, %0, %2\n\tstwcx. %0, 0, %1\n\tbne- 1b\n\tsub %0, %0, %2" : "=&r" (r) : "r" (p), "r" (i) : "cc", "memory");
/* todo */
#  define GRN_BIT_SCAN_REV(v,r)   for (r = 31; r && !((1 << r) & v); r--)
#  define GRN_BIT_SCAN_REV0 GRN_BIT_SCAN_REV
# elif (defined(__sun) && defined(__SVR4)) /* ATOMIC ADD */
#  include <atomic.h>
#  define GRN_ATOMIC_ADD_EX(p,i,r) \
  r = atomic_add_32_nv(p, i) - i
/* todo */
#  define GRN_BIT_SCAN_REV(v,r)   for (r = 31; r && !((1 << r) & v); r--)
#  define GRN_BIT_SCAN_REV0 GRN_BIT_SCAN_REV
# else /* ATOMIC ADD */
/* todo */
#  define GRN_BIT_SCAN_REV(v,r)   for (r = 31; r && !((1 << r) & v); r--)
#  define GRN_BIT_SCAN_REV0 GRN_BIT_SCAN_REV
# endif /* ATOMIC ADD */

# ifdef __i386__ /* ATOMIC 64BIT SET */
#  define GRN_SET_64BIT(p,v) \
  __asm__ __volatile__ ("\txchgl %%esi, %%ebx\n1:\n\tmovl (%0), %%eax\n\tmovl 4(%0), %%edx\n\tlock; cmpxchg8b (%0)\n\tjnz 1b\n\txchgl %%ebx, %%esi" : : "D"(p), "S"(*(((uint32_t *)&(v))+0)), "c"(*(((uint32_t *)&(v))+1)) : "ax", "dx", "memory");
# elif defined(__x86_64__) /* ATOMIC 64BIT SET */
#  define GRN_SET_64BIT(p,v) \
  *(p) = (v);
# elif (defined(__sun) && defined(__SVR4)) /* ATOMIC 64BIT SET */
/* todo */
#  define GRN_SET_64BIT(p,v) \
  (void)atomic_swap_64(p, v)
# endif /* ATOMIC 64BIT SET */

# ifdef HAVE_MKOSTEMP
#  define GRN_MKOSTEMP(template,flags,mode) mkostemp(template,flags)
# else /* HAVE_MKOSTEMP */
#  define GRN_MKOSTEMP(template,flags,mode) \
  (mktemp(template), GRN_OPEN((template),flags,mode))
# endif /* HAVE_MKOSTEMP */

#elif (defined(WIN32) || defined (_WIN64)) /* __GNUC__ */

# define GRN_ATOMIC_ADD_EX(p,i,r) \
  (r) = (uint32_t)InterlockedExchangeAdd((int32_t *)(p), (int32_t)(i));
# if defined(_WIN64) /* ATOMIC 64BIT SET */
#  define GRN_SET_64BIT(p,v) \
  *(p) = (v);
# else /* ATOMIC 64BIT SET */
#  define GRN_SET_64BIT(p,v) \
{\
  uint32_t v1, v2; \
  uint64_t *p2= (p); \
  v1 = *(((uint32_t *)&(v))+0);\
  v2 = *(((uint32_t *)&(v))+1);\
  __asm  _set_loop: \
  __asm  mov esi, p2 \
  __asm  mov ebx, v1 \
  __asm  mov ecx, v2 \
  __asm  mov eax, dword ptr [esi] \
  __asm  mov edx, dword ptr [esi + 4] \
  __asm  lock cmpxchg8b qword ptr [esi] \
  __asm  jnz  _set_loop \
}\
/* TODO: use _InterlockedCompareExchange64 or inline asm */
# endif /* ATOMIC 64BIT SET */

/* todo */
# define GRN_BIT_SCAN_REV(v,r)   for (r = 31; r && !((1 << r) & v); r--)
# define GRN_BIT_SCAN_REV0 GRN_BIT_SCAN_REV

# define GRN_MKOSTEMP(template,flags,mode) \
  (mktemp(template), GRN_OPEN((template),((flags)|O_BINARY),mode))

#else /* __GNUC__ */

# if (defined(__sun) && defined(__SVR4)) /* ATOMIC ADD */
#  define __FUNCTION__ ""
#  include <atomic.h>
#  define GRN_ATOMIC_ADD_EX(p,i,r) \
  r = atomic_add_32_nv(p, i) - i
/* todo */
#  define GRN_BIT_SCAN_REV(v,r)   for (r = 31; r && !((1 << r) & v); r--)
#  define GRN_BIT_SCAN_REV0 GRN_BIT_SCAN_REV
/* todo */
#  define GRN_SET_64BIT(p,v) \
  (void)atomic_swap_64(p, v)
# endif /* ATOMIC ADD */
/* todo */
# define GRN_BIT_SCAN_REV(v,r)   for (r = 31; r && !((1 << r) & v); r--)
# define GRN_BIT_SCAN_REV0 GRN_BIT_SCAN_REV

# define GRN_MKOSTEMP(template,flags,mode) \
  (mktemp(template), GRN_OPEN((template),flags,mode))

#endif /* __GNUC__ */

typedef uint8_t byte;

#define GRN_ID_WIDTH 30

#ifdef __GNUC__
inline static int
grn_str_greater(const uint8_t *ap, uint32_t as, const uint8_t *bp, uint32_t bs)
{
  for (;; ap++, bp++, as--, bs--) {
    if (!as) { return 0; }
    if (!bs) { return 1; }
    if (*ap < *bp) { return 0; }
    if (*ap > *bp) { return 1; }
  }
}
#else /* __GNUC__ */
# define grn_str_greater(ap,as,bp,bs)\
  (((as) > (bs)) ? (memcmp((ap), (bp), (bs)) >= 0) : (memcmp((ap), (bp), (as)) > 0))
#endif /* __GNUC__ */

#ifdef WORDS_BIGENDIAN
#define grn_hton(buf,key,size) \
{\
  uint32_t size_ = (uint32_t)size;\
  uint8_t *buf_ = (uint8_t *)buf;\
  uint8_t *key_ = (uint8_t *)key;\
  while (size_--) { *buf_++ = *key_++; }\
}
#define grn_ntohi(buf,key,size) \
{\
  uint32_t size_ = (uint32_t)size;\
  uint8_t *buf_ = (uint8_t *)buf;\
  uint8_t *key_ = (uint8_t *)key;\
  if (size_) { *buf_++ = 0x80 ^ *key_++; size_--; }\
  while (size_) { *buf_++ = *key_++; size_--; }\
}
#else /* WORDS_BIGENDIAN */
#define grn_hton(buf,key,size) \
{\
  uint32_t size_ = (uint32_t)size;\
  uint8_t *buf_ = (uint8_t *)buf;\
  uint8_t *key_ = (uint8_t *)key + size;\
  while (size_--) { *buf_++ = *(--key_); }\
}
#define grn_ntohi(buf,key,size) \
{\
  uint32_t size_ = (uint32_t)size;\
  uint8_t *buf_ = (uint8_t *)buf;\
  uint8_t *key_ = (uint8_t *)key + size;\
  while (size_ > 1) { *buf_++ = *(--key_); size_--; }\
  if (size_) { *buf_ = 0x80 ^ *(--key_); } \
}
#endif /* WORDS_BIGENDIAN */
#define grn_ntoh grn_hton

#define grn_gton(keybuf,key,size)\
{\
  uint8_t *keybuf_ = keybuf;\
  const void *key_ = key;\
  int la = ((grn_geo_point *)key_)->latitude;\
  int lo = ((grn_geo_point *)key_)->longitude;\
  uint8_t *p = keybuf_;\
  int i = 32;\
  while (i) {\
    i -= 4;\
    *p++ = ((((la >> i) & 8) << 4) + (((lo >> i) & 8) << 3) +\
            (((la >> i) & 4) << 3) + (((lo >> i) & 4) << 2) +\
            (((la >> i) & 2) << 2) + (((lo >> i) & 2) << 1) +\
            (((la >> i) & 1) << 1) + (((lo >> i) & 1) << 0));\
  }\
}

#define grn_ntog(keybuf,key,size)\
{\
  uint8_t *keybuf_ = keybuf;\
  uint8_t *key_ = key;\
  uint32_t size_ = size;\
  int la = 0, lo = 0;\
  uint8_t v, *p = key_;\
  int i = 32;\
  while (size_--) {\
    i -= 4;\
    v = *p++;\
    la += (((v & 128) >> 4) + ((v &  32) >> 3) +\
           ((v &   8) >> 2) + ((v &   2) >> 1)) << i;\
    lo += (((v &  64) >> 3) + ((v &  16) >> 2) +\
           ((v &   4) >> 1) + ((v &   1) >> 0)) << i;\
  }\
  ((grn_geo_point *)keybuf_)->latitude = la;\
  ((grn_geo_point *)keybuf_)->longitude = lo;\
}

#ifdef USE_FUTEX
#include <linux/futex.h>
#include <sys/syscall.h>

#define GRN_FUTEX_WAIT(p) do {\
  int err;\
  struct timespec timeout = {1, 0};\
  while (1) {\
    if (!(err = syscall(SYS_futex, p, FUTEX_WAIT, *p, &timeout))) {\
      break;\
    }\
    if (err == ETIMEDOUT) {\
      GRN_LOG(ctx, GRN_LOG_CRIT, "timeout in GRN_FUTEX_WAIT(%p)", p);\
      break;\
    } else if (err != EWOULDBLOCK) {\
      GRN_LOG(ctx, GRN_LOG_CRIT, "error %d in GRN_FUTEX_WAIT(%p)", err);\
      break;\
    }\
  }\
} while(0)

#define GRN_FUTEX_WAKE(p) \
  syscall(SYS_futex, p, FUTEX_WAKE, 1)
#else /* USE_FUTEX */
#define GRN_FUTEX_WAIT(p) usleep(1000)
#define GRN_FUTEX_WAKE(p)
#endif /* USE_FUTEX */

#ifndef HOST_NAME_MAX
#ifdef _POSIX_HOST_NAME_MAX
#define HOST_NAME_MAX _POSIX_HOST_NAME_MAX
#else /* POSIX_HOST_NAME_MAX */
#define HOST_NAME_MAX 128
#endif /* POSIX_HOST_NAME_MAX */
#endif /* HOST_NAME_MAX */

#ifndef GROONGA_H
#include "groonga.h"
#endif /* GROONGA_H */

#endif /* GROONGA_IN_H */
