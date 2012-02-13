/* -*- c-basic-offset: 2 -*- */
/* Copyright(C) 2009-2012 Brazil

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
#ifndef GRN_NFKC_H
#define GRN_NFKC_H

#include <groonga.h>

#ifdef	__cplusplus
extern "C" {
#endif

unsigned char grn_nfkc_unicode_51_ctype(const unsigned char *str);
const char *grn_nfkc_unicode_51_map1(const unsigned char *str);
const char *grn_nfkc_unicode_51_map2(const unsigned char *prefix,
                                     const unsigned char *suffix);

#ifdef __cplusplus
}
#endif

#endif /* GRN_NFKC_H */
