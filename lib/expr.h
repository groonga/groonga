/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2013 Brazil

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

#ifndef GRN_EXPR_H
#define GRN_EXPR_H

#ifdef __cplusplus
extern "C" {
#endif

#define SCAN_ACCESSOR                  (0x01)
#define SCAN_PUSH                      (0x02)
#define SCAN_POP                       (0x04)
#define SCAN_PRE_CONST                 (0x08)

typedef enum {
  SCAN_START = 0,
  SCAN_VAR,
  SCAN_COL1,
  SCAN_COL2,
  SCAN_CONST
} scan_stat;

typedef struct _grn_scan_info scan_info;

#ifdef __cplusplus
}
#endif

#endif /* GRN_EXPR_H */
