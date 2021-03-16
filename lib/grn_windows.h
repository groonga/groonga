/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2010-2017  Brazil
  Copyright(C) 2021  Sutou Kouhei <kou@clear-code.com>

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
#include "wchar.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
void grn_windows_init(void);
void grn_windows_fin(void);
GRN_API UINT grn_windows_encoding_to_code_page(grn_encoding encoding);
bool grn_windows_symbol_initialize(grn_ctx *ctx, HANDLE process);
bool grn_windows_symbol_cleanup(grn_ctx *ctx, HANDLE process);
void grn_windows_log_trace(grn_ctx *ctx,
                           grn_log_level level,
                           HANDLE process,
                           DWORD64 address);
#endif /* WIN32 */

#ifdef __cplusplus
}
#endif
