/*
  Copyright (C) 2026  Sutou Kouhei <kou@clear-code.com>

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

/*
  WebAssembly.Table accepts only a function that is exported by a
  WebAssembly module. WebAssembly.Function that creates one from a
  JavaScript function isn't available by default. So groonga.mjs in
  wasm/ uses libgroonga-callback.wasm that is built from this file to
  register a JavaScript function as a callback of the Groonga C API
  such as grn_ctx_recv_handler_set().

  The signature is the same as \ref grn_recv_handler_func in
  include/groonga/groonga.h. All parameters are int because a pointer
  and an int are i32 on wasm32.

  We don't put this into libgroonga.wasm because libgroonga.wasm should
  not require a JavaScript specific import.
*/

__attribute__((import_module("env"),
               import_name("grn_recv_handler_func"))) extern void
grn_recv_handler_func_body(int ctx, int flags, int user_data);

__attribute__((export_name("grn_recv_handler_func"))) void
grn_recv_handler_func(int ctx, int flags, int user_data)
{
  grn_recv_handler_func_body(ctx, flags, user_data);
}
