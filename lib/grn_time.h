/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2009-2016 Brazil

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License version 2.1 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#pragma once

#include "grn.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  int64_t tv_sec;
  int32_t tv_nsec;
} grn_timeval;

#ifndef GRN_TIMEVAL_STR_SIZE
#define GRN_TIMEVAL_STR_SIZE 0x100
#endif /* GRN_TIMEVAL_STR_SIZE */
#ifndef GRN_TIMEVAL_STR_FORMAT
#define GRN_TIMEVAL_STR_FORMAT "%04d-%02d-%02d %02d:%02d:%02d.%06d"
#endif /* GRN_TIMEVAL_STR_FORMAT */
#define GRN_TIMEVAL_TO_MSEC(timeval)                    \
  (((timeval)->tv_sec * GRN_TIME_MSEC_PER_SEC) +        \
   ((timeval)->tv_nsec / GRN_TIME_NSEC_PER_MSEC))
#define GRN_TIME_NSEC_PER_SEC 1000000000
#define GRN_TIME_NSEC_PER_SEC_F 1000000000.0
#define GRN_TIME_NSEC_PER_MSEC 1000000
#define GRN_TIME_NSEC_PER_USEC (GRN_TIME_NSEC_PER_SEC / GRN_TIME_USEC_PER_SEC)
#define GRN_TIME_MSEC_PER_SEC 1000
#define GRN_TIME_NSEC_TO_USEC(nsec) ((nsec) / GRN_TIME_NSEC_PER_USEC)
#define GRN_TIME_USEC_TO_NSEC(usec) ((usec) * GRN_TIME_NSEC_PER_USEC)

GRN_API grn_rc grn_timeval_now(grn_ctx *ctx, grn_timeval *tv);
GRN_API grn_rc grn_timeval2str(grn_ctx *ctx, grn_timeval *tv, char *buf, size_t buf_size);
struct tm *grn_timeval2tm(grn_ctx *ctx, grn_timeval *tv, struct tm *tm_buffer);
grn_rc grn_str2timeval(const char *str, uint32_t str_len, grn_timeval *tv);

#ifdef __cplusplus
}
#endif
