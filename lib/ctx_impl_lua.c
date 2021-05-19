/*
  Copyright(C) 2019 Yasuhiro Horimoto <horimoto@clear-code.com>

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
#include "grn_ctx_impl_lua.h"

#ifdef GRN_WITH_LUAJIT
static void
grn_ctx_impl_lua_init_real(grn_ctx *ctx)
{
  lua_State *lua;

  lua = lua_open();
  luaL_openlibs(lua);
  ctx->impl->lua.state = lua;
}

static void
grn_ctx_impl_lua_fin_real(grn_ctx * ctx)
{
  if (ctx->impl->lua.state) {
    lua_close(ctx->impl->lua.state);
    ctx->impl->lua.state = NULL;
  }
}
#else /* GRN_WITH_LUAJIT */
static void
grn_ctx_impl_lua_init_real(grn_ctx *ctx)
{
}

static void
grn_ctx_impl_lua_fin_real(grn_ctx *ctx)
{
}
#endif /* GRN_WITH_LUAJIT */

void
grn_ctx_impl_lua_init(grn_ctx *ctx)
{
  ctx->impl->lua.initialized = GRN_TRUE;
  grn_ctx_impl_lua_init_real(ctx);
}

void
grn_ctx_impl_lua_fin(grn_ctx *ctx)
{
  if (!ctx->impl->lua.initialized) {
    return;
  }

  ctx->impl->lua.initialized = GRN_FALSE;
  grn_ctx_impl_lua_fin_real(ctx);
}
