/*
  Copyright(C) 2014-2018 Brazil
  Copyright(C) 2019 Kouhei Sutou <kou@clear-code.com>

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

#ifdef WIN32
# define GROONGA_MAIN
#endif /* WIN32 */

#include <stdio.h>

#include <grn_mrb.h>
#include <grn_ctx_impl.h>
#include <grn_ctx_impl_mrb.h>
#include <grn_encoding.h>

#include <mruby/variable.h>
#include <mruby/array.h>

static uint32_t
get_thread_limit(void *data)
{
  return 1;
}

static int
run_command(grn_ctx *ctx, int argc, char **argv)
{
  int exit_code = EXIT_SUCCESS;
  grn_mrb_data *data = &(ctx->impl->mrb);
  mrb_state *mrb = data->state;
  mrb_value mrb_command_line_module;
  mrb_value mrb_grndb_class;

  mrb_command_line_module = mrb_const_get(mrb,
                                          mrb_obj_value(data->module),
                                          mrb_intern_cstr(mrb, "CommandLine"));
  if (mrb->exc) {
    goto exit;
  }

  mrb_grndb_class = mrb_const_get(mrb,
                                  mrb_command_line_module,
                                  mrb_intern_cstr(mrb, "Grndb"));
  if (mrb->exc) {
    goto exit;
  }

  {
    int i;
    mrb_value mrb_argv;
    mrb_value mrb_grndb;
    mrb_value mrb_result;

    mrb_argv = mrb_ary_new_capa(mrb, argc);
    for (i = 0; i < argc; i++) {
      const char *utf8_arg;
      mrb_value mrb_utf8_arg;

      utf8_arg =
        grn_encoding_convert_to_utf8_from_locale(ctx, argv[i], -1, NULL);
      mrb_utf8_arg = mrb_str_new_cstr(mrb, utf8_arg);
      grn_encoding_converted_free(ctx, utf8_arg);
      mrb_ary_push(mrb, mrb_argv, mrb_utf8_arg);
    }
    mrb_grndb = mrb_funcall(mrb, mrb_grndb_class, "new", 1, mrb_argv);
    if (mrb->exc) {
      goto exit;
    }

    mrb_result = mrb_funcall(mrb, mrb_grndb, "run", 0);
    if (mrb->exc) {
      goto exit;
    }

    if (!mrb_bool(mrb_result)) {
      exit_code = EXIT_FAILURE;
    }
  }

exit :
  if (mrb->exc) {
    mrb_print_error(mrb);
    exit_code = EXIT_FAILURE;
  }

  return exit_code;
}

static int
run(grn_ctx *ctx, int argc, char **argv)
{
  int exit_code = EXIT_SUCCESS;
  const char *grndb_rb = "command_line/grndb.rb";
  grn_mrb_data *data = &(ctx->impl->mrb);
  mrb_state *mrb = data->state;

  mrb_gv_set(mrb, mrb_intern_lit(mrb, "$0"), mrb_str_new_cstr(mrb, argv[0]));

  grn_mrb_load(ctx, grndb_rb);
  if (ctx->rc != GRN_SUCCESS) {
    fprintf(stderr, "Failed to load Ruby script: <%s>: %s",
            grndb_rb, ctx->errbuf);
    goto exit;
  }

  {
    int arena_index;

    arena_index = mrb_gc_arena_save(mrb);
    exit_code = run_command(ctx, argc, argv);
    mrb_gc_arena_restore(mrb, arena_index);
  }

exit :
  if (ctx->rc != GRN_SUCCESS) {
    exit_code = EXIT_FAILURE;
  }
  return exit_code;
}

int
main(int argc, char **argv)
{
  int exit_code = EXIT_SUCCESS;
  const char *log_path = GRN_LOG_PATH;
  const char *log_level_name = NULL;
  const char *log_flags_name = NULL;

  grn_thread_set_get_limit_func(get_thread_limit, NULL);

  {
    int i;
    for (i = 1; i < argc; i++) {
      const char *arg = argv[i];

      if (arg[0] != '-') {
        continue;
      }

      if (arg[1] == '-' && arg[2] == '\0') {
        break;
      }

#define log_path_prefix "--log-path"
#define log_level_prefix "--log-level"
#define log_flags_prefix "--log-flags"
      if (strcmp(arg, log_path_prefix) == 0) {
        if (i + 1 < argc) {
          log_path = argv[i + 1];
          i++;
        }
      } else if (strncmp(arg,
                         log_path_prefix "=",
                         strlen(log_path_prefix "=")) == 0) {
        log_path = arg + strlen(log_path_prefix "=");
      } else if (strcmp(arg, log_level_prefix) == 0) {
        if (i + 1 < argc) {
          log_level_name = argv[i + 1];
          i++;
        }
      } else if (strncmp(arg,
                         log_level_prefix "=",
                         strlen(log_level_prefix "=")) == 0) {
        log_level_name = arg + strlen(log_level_prefix "=");
      } else if (strcmp(arg, log_flags_prefix) == 0) {
        if (i + 1 < argc) {
          log_flags_name = argv[i + 1];
          i++;
        }
      } else if (strncmp(arg,
                         log_flags_prefix "=",
                         strlen(log_flags_prefix "=")) == 0) {
        log_flags_name = arg + strlen(log_flags_prefix "=");
      }
#undef log_path_prefix
#undef log_level_prefix
#undef log_flags_prefix
    }
  }

  grn_default_logger_set_path(log_path);
  if (log_level_name) {
    grn_log_level log_level = GRN_LOG_DEFAULT_LEVEL;
    if (!grn_log_level_parse(log_level_name, &log_level)) {
      fprintf(stderr, "%s: failed to parse log level: <%s>\n",
              argv[0], log_level_name);
      return EXIT_FAILURE;
    }
    grn_default_logger_set_max_level(log_level);
  }

  if (log_flags_name) {
    int log_flags;
    if (!grn_log_flags_parse(log_flags_name, -1, &log_flags)) {
      fprintf(stderr, "%s: failed to parse log flags: <%s>\n",
              argv[0], log_flags_name);
      return EXIT_FAILURE;
    }
    grn_default_logger_set_flags(log_flags);
  }

  if (grn_init() != GRN_SUCCESS) {
    return EXIT_FAILURE;
  }

  {
    grn_ctx ctx;
    grn_ctx_init(&ctx, 0);
    grn_ctx_impl_mrb_ensure_init(&ctx);
    if (ctx.rc == GRN_SUCCESS) {
      exit_code = run(&ctx, argc, argv);
    } else {
      fprintf(stderr, "%d: %s\n", ctx.rc, ctx.errbuf);
      exit_code = EXIT_FAILURE;
    }
    grn_ctx_fin(&ctx);
  }

  grn_fin();

  return exit_code;
}
