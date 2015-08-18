/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2015 Brazil

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

#ifndef GRN_REPORT_H
#define GRN_REPORT_H

#include "grn_ctx.h"

#ifdef __cplusplus
extern "C" {
#endif

const grn_log_level GRN_REPORT_INDEX_LOG_LEVEL;

void grn_report_index(grn_ctx *ctx,
                      const char *action,
                      const char *tag,
                      grn_obj *index);

#ifdef __cplusplus
}
#endif

#endif /* GRN_REPORT_H */
