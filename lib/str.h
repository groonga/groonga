/* -*- c-basic-offset: 2 -*- */
/* Copyright(C) 2009 Brazil

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License version 2.1 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifndef GRN_STR_H
#define GRN_STR_H

#ifndef GROONGA_H
#include "groonga_in.h"
#endif /* GROONGA_H */

#ifndef GRN_NFKC_H
#include "nfkc.h"
#endif /* GRN_NFKC_H */

#ifdef	__cplusplus
extern "C" {
#endif

typedef enum {
  getopt_op_none = 0,
  getopt_op_on,
  getopt_op_off,
  getopt_op_update
} grn_str_getopt_op;

typedef struct {
  const char opt; /* ends opt == 0 && longopt == NULL */
  const char *longopt;
  char **arg; /* if NULL, no arg are required */
  int flag;
  grn_str_getopt_op op;
} grn_str_getopt_opt;

size_t grn_str_len(grn_ctx *ctx, const char *str, grn_encoding encoding, const char **last);

#define GRN_STR_BLANK 0x80
#define GRN_STR_ISBLANK(c) (c & 0x80)
#define GRN_STR_CTYPE(c) (c & 0x7f)

int grn_isspace(const char *s, grn_encoding encoding);
int grn_atoi(const char *nptr, const char *end, const char **rest);
unsigned int grn_atoui(const char *nptr, const char *end, const char **rest);
unsigned int grn_htoui(const char *nptr, const char *end, const char **rest);
int64_t grn_atoll(const char *nptr, const char *end, const char **rest);
grn_rc grn_itoa(int i, char *p, char *end, char **rest);
grn_rc grn_lltoa(int64_t i, char *p, char *end, char **rest);
const char *grn_enctostr(grn_encoding enc);
grn_encoding grn_strtoenc(const char *str);

void grn_itoh(unsigned int i, char *p, unsigned int len);
int grn_str_tok(char *str, size_t str_len, char delim, char **tokbuf, int buf_size, char **rest);
int grn_str_getopt(int argc, char * const argv[], const grn_str_getopt_opt *opts, int *flags);

extern int grn_str_margin_size;

char *grn_itob(grn_id id, char *p);
grn_id grn_btoi(char *b);

grn_rc grn_substring(grn_ctx *ctx, char **str, char **str_end, int start, int end, grn_encoding encoding);

void grn_logger_fin(void);

/* bulk */

#define GRN_BULK_VALUE(bulk) \
 (((bulk)->header.type == GRN_BULK) ? GRN_BULK_HEAD(bulk) : NULL)

#define GRN_BULK_LEN(bulk) \
 (((bulk)->header.type == GRN_BULK) ? GRN_BULK_VSIZE(bulk) : 0)

#define GRN_BULK_SET(ctx,bulk,str,len) {\
  if ((bulk)->header.type == GRN_VOID) {\
    GRN_OBJ_INIT((bulk), GRN_BULK, 0);\
  }\
  if ((bulk)->header.type == GRN_BULK) {\
    if ((bulk)->header.flags & GRN_OBJ_DO_SHALLOW_COPY) {\
      (bulk)->u.b.head = (char *)(str);\
      (bulk)->u.b.curr = (char *)(str) + len;\
    } else {\
      grn_bulk_write((ctx), (bulk), (const char *)(str), (unsigned int)(len));\
    }\
  } else {\
    (ctx)->rc = GRN_INVALID_ARGUMENT;\
  }\
}

#ifdef __cplusplus
}
#endif

#endif /* GRN_STR_H */
