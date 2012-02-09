/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2012 Brazil

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

#include <stdlib.h>
#include <string.h>

#include <groonga/normalizer.h>
#include "nfkc.h"

static grn_obj *
utf8_nfkc_normalize(grn_ctx *ctx, int nargs, grn_obj **args,
                    grn_user_data *user_data)
{
  grn_obj *normalized_text = args[0];
  const char *orig;
  char *norm;
  short *checks = NULL, *ch;
  const unsigned char *s, *s_, *s__ = NULL, *p, *p2, *pe, *e;
  unsigned char *d, *d_, *de;
  unsigned char *ctypes = NULL, *cp;
  size_t length = 0, ls, lp;
  unsigned int size, ds;
  int flags;
  grn_bool remove_blank_p;

  grn_normalized_text_get_original_value(ctx, normalized_text, &orig, &size);
  ds = size * 3;
  flags = grn_normalized_text_get_flags(ctx, normalized_text);
  remove_blank_p = flags & GRN_NORMALIZE_REMOVE_BLANK;
  if (!(norm = GRN_PLUGIN_MALLOC(ctx, ds + 1))) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_NO_MEMORY_AVAILABLE,
                     "[normalizer][utf8][nfkc] "
                     "failed to allocate normalized text space");
    return NULL;
  }
  if (flags & GRN_NORMALIZE_WITH_CHECKS) {
    if (!(checks = GRN_PLUGIN_MALLOC(ctx, ds * sizeof(short) + 1))) {
      GRN_PLUGIN_FREE(ctx, norm);
      GRN_PLUGIN_ERROR(ctx,
                       GRN_NO_MEMORY_AVAILABLE,
                       "[normalizer][utf8][nfkc] "
                       "failed to allocate checks space");
      return NULL;
    }
  }
  ch = checks;
  if (flags & GRN_NORMALIZE_WITH_TYPES) {
    if (!(ctypes = GRN_PLUGIN_MALLOC(ctx, ds + 1))) {
      if (checks) { GRN_PLUGIN_FREE(ctx, checks); }
      GRN_PLUGIN_FREE(ctx, norm);
      GRN_PLUGIN_ERROR(ctx,
                       GRN_NO_MEMORY_AVAILABLE,
                       "[normalizer][utf8][nfkc] "
                       "failed to allocate character types space");
      return NULL;
    }
  }
  cp = ctypes;
  d = (unsigned char *)norm;
  de = d + ds;
  d_ = NULL;
  e = (unsigned char *)orig + size;
  for (s = s_ = (unsigned char *)orig; ; s += ls) {
    if (!(ls = grn_charlen_utf8(ctx, s, e))) {
      break;
    }
    if ((p = (unsigned char *)grn_nfkc_map1(s))) {
      pe = p + strlen((char *)p);
    } else {
      p = s;
      pe = p + ls;
    }
    if (d_ && (p2 = (unsigned char *)grn_nfkc_map2(d_, p))) {
      p = p2;
      pe = p + strlen((char *)p);
      if (cp) { cp--; }
      if (ch) {
        ch -= (d - d_);
        s_ = s__;
      }
      d = d_;
      length--;
    }
    for (; ; p += lp) {
      if (!(lp = grn_charlen_utf8(ctx, p, pe))) {
        break;
      }
      if ((*p == ' ' && remove_blank_p) || *p < 0x20  /* skip unprintable ascii */ ) {
        if (cp > ctypes) { *(cp - 1) |= GRN_CHAR_BLANK; }
      } else {
        if (de <= d + lp) {
          unsigned char *new_norm;
          ds += (ds >> 1) + lp;
          if (!(new_norm = GRN_PLUGIN_REALLOC(ctx, norm, ds + 1))) {
            if (ctypes) { GRN_PLUGIN_FREE(ctx, ctypes); }
            if (checks) { GRN_PLUGIN_FREE(ctx, checks); }
            GRN_PLUGIN_FREE(ctx, norm);
            GRN_PLUGIN_ERROR(ctx,
                             GRN_NO_MEMORY_AVAILABLE,
                             "[normalizer][utf8][nfkc] "
                             "failed to reallocate normalized text space");
            return NULL;
          }
          de = new_norm + ds;
          d = new_norm + (d - (unsigned char *)norm);
          norm = new_norm;
          if (ch) {
            short *new_checks;
            new_checks = GRN_PLUGIN_REALLOC(ctx, checks, ds * sizeof(short) + 1);
            if (!new_checks) {
              if (ctypes) { GRN_PLUGIN_FREE(ctx, ctypes); }
              GRN_PLUGIN_FREE(ctx, checks);
              GRN_PLUGIN_FREE(ctx, norm);
              GRN_PLUGIN_ERROR(ctx,
                               GRN_NO_MEMORY_AVAILABLE,
                               "[normalizer][utf8][nfkc] "
                               "failed to reallocate checks space");
              return NULL;
            }
            ch = new_checks + (ch - checks);
            checks = new_checks;
          }
          if (cp) {
            unsigned char *new_ctypes;
            if (!(new_ctypes = GRN_PLUGIN_REALLOC(ctx, ctypes, ds + 1))) {
              GRN_PLUGIN_FREE(ctx, ctypes);
              if (checks) { GRN_PLUGIN_FREE(ctx, checks); }
              GRN_PLUGIN_FREE(ctx, norm);
              GRN_PLUGIN_ERROR(ctx,
                               GRN_NO_MEMORY_AVAILABLE,
                               "[normalizer][utf8][nfkc] "
                               "failed to reallocate character types space");
              return NULL;
            }
            cp = new_ctypes + (cp - ctypes);
            ctypes = new_ctypes;
          }
        }
        memcpy(d, p, lp);
        d_ = d;
        d += lp;
        length++;
        if (cp) { *cp++ = grn_nfkc_ctype(p); }
        if (ch) {
          size_t i;
          if (s_ == s + ls) {
            *ch++ = -1;
          } else {
            *ch++ = (short)(s + ls - s_);
            s__ = s_;
            s_ = s + ls;
          }
          for (i = lp; i > 1; i--) { *ch++ = 0; }
        }
      }
    }
  }
  if (cp) { *cp = grn_char_null; }
  *d = '\0';
  grn_normalized_text_set_value(ctx, normalized_text,
                                norm, length,
                                (size_t)(d - (unsigned char *)norm));
  grn_normalized_text_set_checks(ctx, normalized_text, checks);
  grn_normalized_text_set_types(ctx, normalized_text, ctypes);
  return NULL;
}

grn_rc
GRN_PLUGIN_INIT(grn_ctx *ctx)
{
  return GRN_SUCCESS;
}

grn_rc
GRN_PLUGIN_REGISTER(grn_ctx *ctx)
{
  grn_obj *normalizer;

  normalizer = GRN_NORMALIZER_REGISTER(ctx, "NormalizerUTF8NFKC",
                                       NULL, utf8_nfkc_normalize, NULL);
  if (normalizer) {
    return GRN_SUCCESS;
  } else {
    return GRN_FILE_CORRUPT;
  }
}

grn_rc
GRN_PLUGIN_FIN(grn_ctx *ctx)
{
  return GRN_SUCCESS;
}
