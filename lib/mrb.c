/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2013 Brazil

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

#include "mrb.h"
#include "ctx_impl.h"

#ifdef GRN_WITH_MRUBY
# include <mruby/proc.h>
# include <mruby/compile.h>
# include <mruby/string.h>
#endif

#ifdef GRN_WITH_MRUBY
#ifdef WIN32
static char *win32_ruby_scripts_dir = NULL;
static char win32_ruby_scripts_dir_buffer[PATH_MAX];
static const char *
grn_mrb_get_system_ruby_scripts_dir(void)
{
  if (!win32_ruby_scripts_dir) {
    const char *base_dir;
    const char *relative_path = GRN_RELATIVE_RUBY_SCRIPTS_DIR;
    char *path;
    size_t base_dir_length;

    base_dir = grn_win32_base_dir();
    base_dir_length = strlen(base_dir);
    strcpy(win32_ruby_scripts_dir_buffer, base_dir);
    strcat(win32_ruby_scripts_dir_buffer, "/");
    strcat(win32_ruby_scripts_dir_buffer, relative_path);
    win32_ruby_scripts_dir = win32_ruby_scripts_dir_buffer;
  }
  return win32_ruby_scripts_dir;
}

#else /* WIN32 */
static const char *
grn_mrb_get_system_ruby_scripts_dir(void)
{
  return GRN_RUBY_SCRIPTS_DIR;
}
#endif /* WIN32 */

static FILE *
grn_mrb_open_script(grn_ctx *ctx, const char *name)
{
  const char *ruby_scripts_dir;
  char dir_last_char;
  char path[PATH_MAX];
  int name_length, max_name_length;
  FILE *script_file;

  GRN_API_ENTER;
  if (name[0] == '/') {
    path[0] = '\0';
  } else {
    ruby_scripts_dir = getenv("GRN_RUBY_SCRIPTS_DIR");
    if (!ruby_scripts_dir) {
      ruby_scripts_dir = grn_mrb_get_system_ruby_scripts_dir();
    }
    strcpy(path, ruby_scripts_dir);

    dir_last_char = ruby_scripts_dir[strlen(path) - 1];
    if (dir_last_char != '/') {
      strcat(path, "/");
    }
  }

  name_length = strlen(name);
  max_name_length = PATH_MAX - strlen(path) - 1;
  if (name_length > max_name_length) {
    ERR(GRN_INVALID_ARGUMENT,
        "script name is too long: %d (max: %d) <%s%s>",
        name_length, max_name_length,
        path, name);
  } else {
    strcat(path, name);
    script_file = fopen(path, "r");
  }

  GRN_API_RETURN(script_file);
}

mrb_value
grn_mrb_load(grn_ctx *ctx, const char *name)
{
  mrb_state *mrb = ctx->impl->mrb.state;
  int n;
  FILE *fp;
  mrb_value result;
  struct mrb_parser_state *parser;

  if (!mrb) {
    return mrb_nil_value();
  }
  if (!(fp = grn_mrb_open_script(ctx, name))) {
    return mrb_nil_value();
  }

  parser = mrb_parse_file(mrb, fp, NULL);
  n = mrb_generate_code(mrb, parser);
  result = mrb_run(mrb,
                   mrb_proc_new(mrb, mrb->irep[n]),
                   mrb_top_self(mrb));
  mrb_parser_free(parser);
  fclose(fp);

  return result;
}

mrb_value
grn_mrb_eval(grn_ctx *ctx, const char *script, int script_length)
{
  mrb_state *mrb = ctx->impl->mrb.state;
  int n;
  mrb_value result;
  struct mrb_parser_state *parser;

  if (!mrb) {
    return mrb_nil_value();
  }

  if (script_length < 0) {
    script_length = strlen(script);
  }
  parser = mrb_parse_nstring(mrb, script, script_length, NULL);
  n = mrb_generate_code(mrb, parser);
  result = mrb_run(mrb,
                   mrb_proc_new(mrb, mrb->irep[n]),
                   mrb_top_self(mrb));
  mrb_parser_free(parser);

  return result;
}

grn_rc
grn_mrb_to_grn(grn_ctx *ctx, mrb_value mrb_object, grn_obj *grn_object)
{
  grn_rc rc = GRN_SUCCESS;

  switch (mrb_type(mrb_object)) {
  case MRB_TT_FIXNUM :
    grn_obj_reinit(ctx, grn_object, GRN_DB_INT32, 0);
    GRN_INT32_SET(ctx, grn_object, mrb_fixnum(mrb_object));
    break;
  case MRB_TT_STRING :
    grn_obj_reinit(ctx, grn_object, GRN_DB_TEXT, 0);
    GRN_TEXT_SET(ctx, grn_object,
                 RSTRING_PTR(mrb_object),
                 RSTRING_LEN(mrb_object));
    break;
  default :
    rc = GRN_INVALID_ARGUMENT;
    break;
  }

  return rc;
}
#endif
