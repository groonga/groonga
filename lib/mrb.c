/*
  Copyright(C) 2013-2018 Brazil

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

#include "grn_mrb.h"
#include "grn_ctx_impl.h"
#include "grn_encoding.h"
#include "grn_util.h"

#include <string.h>

#ifdef GRN_WITH_MRUBY
# include <mruby/compile.h>
# include <mruby/opcode.h>
# include <mruby/proc.h>
# include <mruby/string.h>
#endif

#include <ctype.h>

#ifdef WIN32
# include <share.h>
#endif /* WIN32 */

#define BUFFER_SIZE 2048
#define E_LOAD_ERROR (mrb_class_get(mrb, "LoadError"))

static char grn_mrb_ruby_scripts_dir[GRN_ENV_BUFFER_SIZE];
static grn_bool grn_mrb_order_by_estimated_size_enable = GRN_FALSE;

void
grn_mrb_init_from_env(void)
{
  grn_getenv("GRN_RUBY_SCRIPTS_DIR",
             grn_mrb_ruby_scripts_dir,
             GRN_ENV_BUFFER_SIZE);
  {
    char grn_order_by_estimated_size_enable_env[GRN_ENV_BUFFER_SIZE];
    grn_getenv("GRN_ORDER_BY_ESTIMATED_SIZE_ENABLE",
               grn_order_by_estimated_size_enable_env,
               GRN_ENV_BUFFER_SIZE);
    if (strcmp(grn_order_by_estimated_size_enable_env, "yes") == 0) {
      grn_mrb_order_by_estimated_size_enable = GRN_TRUE;
    } else {
      grn_mrb_order_by_estimated_size_enable = GRN_FALSE;
    }
  }
}

grn_bool
grn_mrb_is_order_by_estimated_size_enabled(void)
{
  return grn_mrb_order_by_estimated_size_enable;
}

#ifdef GRN_WITH_MRUBY
# ifdef WIN32
static char *windows_ruby_scripts_dir = NULL;
static char windows_ruby_scripts_dir_buffer[PATH_MAX];
static const char *
grn_mrb_get_default_system_ruby_scripts_dir(grn_ctx *ctx)
{
  if (!windows_ruby_scripts_dir) {
    const char *base_dir;
    const char *relative_path = GRN_RELATIVE_RUBY_SCRIPTS_DIR;

    base_dir = grn_windows_base_dir();
    grn_strcpy(windows_ruby_scripts_dir_buffer, PATH_MAX, base_dir);
    grn_strcat(windows_ruby_scripts_dir_buffer, PATH_MAX, "/");
    grn_strcat(windows_ruby_scripts_dir_buffer, PATH_MAX, relative_path);
    windows_ruby_scripts_dir = windows_ruby_scripts_dir_buffer;
  }
  return windows_ruby_scripts_dir;
}

# else /* WIN32 */
static const char *
grn_mrb_get_default_system_ruby_scripts_dir(grn_ctx *ctx)
{
  return GRN_RUBY_SCRIPTS_DIR;
}
# endif /* WIN32 */

const char *
grn_mrb_get_system_ruby_scripts_dir(grn_ctx *ctx)
{
  if (grn_mrb_ruby_scripts_dir[0]) {
    return grn_mrb_ruby_scripts_dir;
  } else {
    return grn_mrb_get_default_system_ruby_scripts_dir(ctx);
  }
}

static grn_bool
grn_mrb_is_absolute_path(const char *path)
{
  if (path[0] == '/') {
    return GRN_TRUE;
  }

  if (isalpha((unsigned char)path[0]) && path[1] == ':' && path[2] == '/') {
    return GRN_TRUE;
  }

  return GRN_FALSE;
}

static grn_bool
grn_mrb_expand_script_path(grn_ctx *ctx, const char *path,
                           char *expanded_path, size_t expanded_path_size)
{
  const char *ruby_scripts_dir;
  char dir_last_char;
  int path_length;
  int max_path_length;

  if (grn_mrb_is_absolute_path(path)) {
    expanded_path[0] = '\0';
  } else if (path[0] == '.' && path[1] == '/') {
    grn_strcpy(expanded_path, expanded_path_size, ctx->impl->mrb.base_directory);
    grn_strcat(expanded_path, expanded_path_size, "/");
  } else {
    ruby_scripts_dir = grn_mrb_get_system_ruby_scripts_dir(ctx);
    grn_strcpy(expanded_path, expanded_path_size, ruby_scripts_dir);

    dir_last_char = ruby_scripts_dir[strlen(expanded_path) - 1];
    if (dir_last_char != '/') {
      grn_strcat(expanded_path, expanded_path_size, "/");
    }
  }

  path_length = strlen(path);
  max_path_length = PATH_MAX - strlen(expanded_path) - 1;
  if (path_length > max_path_length) {
    const char *grn_encoding_path;
    grn_encoding_path =
      grn_encoding_convert_from_locale(ctx,
                                       path,
                                       path_length,
                                       NULL);
    ERR(GRN_INVALID_ARGUMENT,
        "script path is too long: %d (max: %d) <%s%s>",
        path_length, max_path_length,
        expanded_path, grn_encoding_path);
    grn_encoding_converted_free(ctx, grn_encoding_path);
    return GRN_FALSE;
  }

  grn_strcat(expanded_path, expanded_path_size, path);

  return GRN_TRUE;
}

mrb_value
grn_mrb_load(grn_ctx *ctx, const char *path)
{
  grn_mrb_data *data = &(ctx->impl->mrb);
  mrb_state *mrb = data->state;
  char expanded_path[PATH_MAX];
  FILE *file;
  mrb_value result;
  struct mrb_parser_state *parser;

  if (!mrb) {
    return mrb_nil_value();
  }

  if (!grn_mrb_expand_script_path(ctx, path, expanded_path, PATH_MAX)) {
    return mrb_nil_value();
  }

  file = grn_fopen(expanded_path, "r");
  if (!file) {
    const char *grn_encoding_expanded_path;
    mrb_value exception;

    grn_encoding_expanded_path =
      grn_encoding_convert_from_locale(ctx, expanded_path, -1, NULL);
    SERR("fopen: failed to open mruby script file: <%s>",
         grn_encoding_expanded_path);
    grn_encoding_converted_free(ctx, grn_encoding_expanded_path);
    exception = mrb_exc_new(mrb, E_LOAD_ERROR,
                            ctx->errbuf, strlen(ctx->errbuf));
    mrb->exc = mrb_obj_ptr(exception);
    return mrb_nil_value();
  }

  {
    char current_base_directory[PATH_MAX];
    char *last_directory;

    grn_strcpy(current_base_directory, PATH_MAX, data->base_directory);
    grn_strcpy(data->base_directory, PATH_MAX, expanded_path);
    last_directory = strrchr(data->base_directory, '/');
    if (last_directory) {
      last_directory[0] = '\0';
    }

    parser = mrb_parser_new(mrb);
    {
      const char *utf8_expanded_path;
      utf8_expanded_path =
        grn_encoding_convert_to_utf8_from_locale(ctx,
                                                 expanded_path,
                                                 -1,
                                                 NULL);
      mrb_parser_set_filename(parser, utf8_expanded_path);
      grn_encoding_converted_free(ctx, utf8_expanded_path);
    }
    parser->s = parser->send = NULL;
    parser->f = file;
    mrb_parser_parse(parser, NULL);
    fclose(file);

    {
      struct RProc *proc;
      int arena_index;
      proc = mrb_generate_code(mrb, parser);
      MRB_PROC_SET_TARGET_CLASS(proc, mrb->object_class);
      arena_index = mrb_gc_arena_save(mrb);
      result = mrb_yield_with_class(mrb,
                                    mrb_obj_value(proc),
                                    0,
                                    NULL,
                                    mrb_top_self(mrb),
                                    mrb->object_class);
      mrb_gc_arena_restore(mrb, arena_index);
    }
    mrb_parser_free(parser);

    grn_strcpy(data->base_directory, PATH_MAX, current_base_directory);
  }

  return result;
}
#endif
