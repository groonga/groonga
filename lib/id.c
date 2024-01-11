/*
  Copyright(C) 2016  Brazil
  Copyright(C) 2020  Sutou Kouhei <kou@clear-code.com>

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

#include "grn.h"
#include "grn_db.h"

bool
grn_id_is_builtin(grn_ctx *ctx, grn_id id)
{
  if (id == GRN_ID_NIL) {
    return false;
  }
  return id < GRN_N_RESERVED_TYPES;
}

grn_bool
grn_id_is_builtin_type(grn_ctx *ctx, grn_id id)
{
  return grn_type_id_is_builtin(ctx, id);
}

bool
grn_id_maybe_table(grn_ctx *ctx, grn_id id)
{
  return id != GRN_ID_NIL && !grn_id_is_builtin(ctx, id);
}
