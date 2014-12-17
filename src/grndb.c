/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2014 Brazil

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

#ifdef WIN32
# define GROONGA_MAIN
#endif /* WIN32 */

#include <grn_mrb.h>
#include <grn_ctx_impl.h>

#include <mruby/variable.h>
#include <mruby/array.h>

static int
run(grn_ctx *ctx, int argc, char **argv)
{
  const char *grndb_rb = "command/grndb.rb";

  grn_mrb_load(ctx, grndb_rb);
  if (ctx->rc != GRN_SUCCESS) {
      fprintf(stderr, "Failed to load Ruby script: <%s>: %s",
              grndb_rb, ctx->errbuf);
      goto exit;
  }

  /* TODO: Handle error. */
  {
    grn_mrb_data *data = &(ctx->impl->mrb);
    mrb_state *mrb = data->state;
    mrb_value mrb_command_module;
    mrb_value mrb_grndb_class;
    int arena_index;

    arena_index = mrb_gc_arena_save(mrb);
    mrb_command_module = mrb_const_get(mrb,
                                       mrb_obj_value(data->module),
                                       mrb_intern_cstr(mrb, "Command"));
    mrb_grndb_class = mrb_const_get(mrb,
                                    mrb_command_module,
                                    mrb_intern_cstr(mrb, "Grndb"));
    {
      int i;
      mrb_value mrb_argv;
      mrb_value mrb_grndb;
      mrb_value mrb_result;

      mrb_argv = mrb_ary_new_capa(mrb, argc);
      for (i = 0; i < argc; i++) {
        mrb_ary_push(mrb, mrb_argv, mrb_str_new_cstr(mrb, argv[i]));
      }
      mrb_grndb = mrb_funcall(mrb, mrb_grndb_class, "new", 1, mrb_argv);
      mrb_result = mrb_funcall(mrb, mrb_grndb, "run", 0);
    }
    mrb_gc_arena_restore(mrb, arena_index);
  }

exit :
  if (ctx->rc == GRN_SUCCESS) {
    return EXIT_SUCCESS;
  } else {
    return EXIT_FAILURE;
  }
}

int
main(int argc, char **argv)
{
  int exit_code = EXIT_SUCCESS;

  if (grn_init() != GRN_SUCCESS) {
    return EXIT_FAILURE;
  }

  {
    grn_ctx ctx;
    grn_ctx_init(&ctx, 0);
    exit_code = run(&ctx, argc, argv);
    grn_ctx_fin(&ctx);
  }

  grn_fin();

  return exit_code;
}
