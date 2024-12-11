/*
  Copyright(C) 2015-2018 Brazil

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

#pragma once

#include "grn_ctx.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const grn_log_level GRN_REPORT_INDEX_LOG_LEVEL;

void grn_report_index(grn_ctx *ctx,
                      const char *action,
                      const char *tag,
                      grn_obj *index);

void grn_report_index_not_used(grn_ctx *ctx,
                               const char *action,
                               const char *tag,
                               grn_obj *index,
                               const char *reason);

void grn_report_table(grn_ctx *ctx,
                      const char *action,
                      const char *tag,
                      grn_obj *table);

void grn_report_column(grn_ctx *ctx,
                       const char *action,
                       const char *tag,
                       grn_obj *column);

void grn_report_accessor(grn_ctx *ctx,
                         const char *action,
                         const char *tag,
                         grn_obj *accessor);

#ifdef __cplusplus
}
#endif
