/*
  Copyright (C) 2009-2016  Brazil
  Copyright (C) 2025  Sutou Kouhei <kou@clear-code.com>

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

#include <groonga.h>

#ifdef	__cplusplus
extern "C" {
#endif

GRN_API grn_char_type grn_nfkc_char_type(const unsigned char *utf8);
GRN_API grn_char_type grn_nfkc_latest_char_type(const unsigned char *utf8);

#ifdef __cplusplus
}
#endif
