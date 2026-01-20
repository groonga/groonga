/*
  Copyright(C) 2022 Yasuhiro Horimoto <horimoto@clear-code.com>

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

#include "grn_ctx_impl.h"
#include "grn_ctx_impl_duckdb.h"

#ifdef GRN_WITH_DUCKDB
static bool
_grn_ctx_impl_duckdb_init(grn_ctx *ctx) {
  duckdb_state db_open_success;
  db_open_success = duckdb_open(":memory:", ctx->impl->duckdb.db);
  if (db_open_success == DuckDBError) {
    GRN_LOG(ctx, GRN_LOG_WARNING, "DuckDB open failed");
    return false;
  }
  return true;
}

static void
_grn_ctx_impl_duckdb_fin(grn_ctx *ctx) {
  duckdb_disconnection(ctx->impl->duckdb.connection);
  duckdb_close(ctx->impl->duckdb.db);
}
#endif

void
grn_ctx_impl_duckdb_init(grn_ctx *ctx) {
  bool db_init_success = _grn_ctx_impl_duckdb_init(ctx);
  if (db_init_success) {
    ctx->impl->duckdb.initialized = true;
  } else {
    ctx->impl->duckdb.initialized = false;
  }
}

void
grn_ctx_impl_duckdb_fin(grn_ctx *ctx) {
  if(!ctx->impl->duckdb.initialized) {
    return;
  }
  _grn_ctx_impl_duckdb_fin(ctx);
  ctx->impl->duckdb.initialized = false;
}
