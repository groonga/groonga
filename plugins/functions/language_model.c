// Copyright (C) 2024  Sutou Kouhei <kou@clear-code.com>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA

#ifdef GRN_EMBEDDED
#  define GRN_PLUGIN_FUNCTION_TAG functions_language_model
#endif

#include <groonga/plugin.h>

static grn_obj *
func_language_model_vectorize(grn_ctx *ctx,
                              int n_args,
                              grn_obj **args,
                              grn_user_data *user_data)
{
  const char *tag = "language_model_vectorize():";

  if (!(n_args == 2 || n_args == 3)) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "%s wrong number of arguments (%d for 2..3)",
                     tag,
                     n_args);
    return NULL;
  }

  grn_obj *vector = NULL;
  grn_language_model_loader *loader = grn_language_model_loader_open(ctx);
  grn_language_model *model = NULL;
  grn_language_model_inferencer *inferencer = NULL;

  grn_obj *model_name = args[0];
  if (!grn_obj_is_text_family_bulk(ctx, model_name)) {
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, model_name);
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "%s the 1st argument must be model name: %.*s",
                     tag,
                     (int)(GRN_TEXT_LEN(&inspected)),
                     GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    goto exit;
  }
  grn_language_model_loader_set_model(ctx,
                                      loader,
                                      GRN_TEXT_VALUE(model_name),
                                      GRN_TEXT_LEN(model_name));

  grn_obj *text = args[1];
  if (!grn_obj_is_text_family_bulk(ctx, text)) {
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, text);
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "%s the 2nd argument must be text to vectorize: %.*s",
                     tag,
                     (int)(GRN_TEXT_LEN(&inspected)),
                     GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    goto exit;
  }

  grn_obj *options = NULL;
  if (n_args == 3) {
    options = args[2];
  }

  grn_obj *prefix = NULL;
  if (options) {
    grn_rc rc = grn_proc_options_parse(ctx,
                                       options,
                                       tag,
                                       "prefix",
                                       GRN_PROC_OPTION_VALUE_RAW,
                                       &prefix,
                                       NULL);
    if (rc != GRN_SUCCESS) {
      goto exit;
    }

    if (prefix && !grn_obj_is_text_family_bulk(ctx, prefix)) {
      grn_obj inspected;
      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect(ctx, &inspected, prefix);
      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
                       "%s prefix must be a string: %.*s",
                       tag,
                       (int)(GRN_TEXT_LEN(&inspected)),
                       GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
      goto exit;
    }
  }

  model = grn_language_model_loader_load(ctx, loader);
  if (!model) {
    GRN_PLUGIN_ERROR(ctx,
                     ctx->rc,
                     "%s failed to load model: %s",
                     tag,
                     ctx->errbuf);
    goto exit;
  }

  inferencer = grn_language_model_open_inferencer(ctx, model);
  if (!inferencer) {
    GRN_PLUGIN_ERROR(ctx,
                     ctx->rc,
                     "%s failed to open model inferencer: %s",
                     tag,
                     ctx->errbuf);
    goto exit;
  }

  vector =
    grn_plugin_proc_alloc(ctx, user_data, GRN_DB_FLOAT32, GRN_OBJ_VECTOR);
  if (!vector) {
    return NULL;
  }

  grn_obj prefixed_text;
  GRN_TEXT_INIT(&prefixed_text, 0);
  if (prefix) {
    GRN_TEXT_PUT(ctx,
                 &prefixed_text,
                 GRN_TEXT_VALUE(prefix),
                 GRN_TEXT_LEN(prefix));
  }
  GRN_TEXT_PUT(ctx, &prefixed_text, GRN_TEXT_VALUE(text), GRN_TEXT_LEN(text));
  grn_rc rc =
    grn_language_model_inferencer_vectorize(ctx,
                                            inferencer,
                                            GRN_TEXT_VALUE(&prefixed_text),
                                            GRN_TEXT_LEN(&prefixed_text),
                                            vector);
  GRN_OBJ_FIN(ctx, &prefixed_text);

  if (rc != GRN_SUCCESS) {
    GRN_PLUGIN_ERROR(ctx,
                     ctx->rc,
                     "%s failed to vectorize: %s",
                     tag,
                     ctx->errbuf);
    grn_obj_close(ctx, vector);
    vector = NULL;
    goto exit;
  }

exit:
  grn_language_model_inferencer_close(ctx, inferencer);
  grn_language_model_close(ctx, model);
  grn_language_model_loader_close(ctx, loader);

  return vector;
}

static grn_rc
applier_language_model_vectorize(grn_ctx *ctx, grn_applier_data *data)
{
  const char *tag = "language_model_vectorize():";

  size_t n_args;
  grn_obj **args = grn_applier_data_get_args(ctx, data, &n_args);

  if (!(n_args == 2 || n_args == 3)) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "%s wrong number of arguments (%d for 2..3)",
                     tag,
                     (uint32_t)n_args);
    return ctx->rc;
  }

  grn_language_model_loader *loader = grn_language_model_loader_open(ctx);
  grn_language_model *model = NULL;
  grn_language_model_inferencer *inferencer = NULL;

  grn_obj *model_name = args[0];
  if (!grn_obj_is_text_family_bulk(ctx, model_name)) {
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, model_name);
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "%s the 1st argument must be model name: %.*s",
                     tag,
                     (int)(GRN_TEXT_LEN(&inspected)),
                     GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    goto exit;
  }
  grn_language_model_loader_set_model(ctx,
                                      loader,
                                      GRN_TEXT_VALUE(model_name),
                                      GRN_TEXT_LEN(model_name));

  grn_obj *input_column = args[1];
  if (!(grn_obj_is_text_family_scalar_column(ctx, input_column) ||
        grn_obj_is_text_family_scalar_accessor(ctx, input_column))) {
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, input_column);
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "%s the 2nd argument must be a text family column "
                     "or accessor to vectorize: %.*s",
                     tag,
                     (int)(GRN_TEXT_LEN(&inspected)),
                     GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    goto exit;
  }

  grn_obj *options = NULL;
  if (n_args == 3) {
    options = args[2];
  }

  model = grn_language_model_loader_load(ctx, loader);
  if (!model) {
    GRN_PLUGIN_ERROR(ctx,
                     ctx->rc,
                     "%s failed to load model: %s",
                     tag,
                     ctx->errbuf);
    goto exit;
  }

  inferencer = grn_language_model_open_inferencer(ctx, model);
  if (!inferencer) {
    GRN_PLUGIN_ERROR(ctx,
                     ctx->rc,
                     "%s failed to open model inferencer: %s",
                     tag,
                     ctx->errbuf);
    goto exit;
  }

  if (options) {
    grn_obj *prefix = NULL;
    grn_rc rc = grn_proc_options_parse(ctx,
                                       options,
                                       tag,
                                       "prefix",
                                       GRN_PROC_OPTION_VALUE_RAW,
                                       &prefix,
                                       NULL);
    if (rc != GRN_SUCCESS) {
      goto exit;
    }

    if (prefix && !grn_obj_is_text_family_bulk(ctx, prefix)) {
      grn_obj inspected;
      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect(ctx, &inspected, prefix);
      GRN_PLUGIN_ERROR(ctx,
                       GRN_INVALID_ARGUMENT,
                       "%s prefix must be a string: %.*s",
                       tag,
                       (int)(GRN_TEXT_LEN(&inspected)),
                       GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
      goto exit;
    }
    if (prefix && GRN_TEXT_LEN(prefix) > 0) {
      grn_language_model_inferencer_set_input_column_value_prefix(
        ctx,
        inferencer,
        GRN_TEXT_VALUE(prefix),
        (int64_t)GRN_TEXT_LEN(prefix));
      if (ctx->rc != GRN_SUCCESS) {
        goto exit;
      }
    }
  }

  grn_rc rc = grn_language_model_inferencer_vectorize_applier(ctx,
                                                              inferencer,
                                                              input_column,
                                                              data);
  if (rc != GRN_SUCCESS) {
    GRN_PLUGIN_ERROR(ctx,
                     ctx->rc,
                     "%s failed to vectorize: %s",
                     tag,
                     ctx->errbuf);
    goto exit;
  }

exit:
  grn_language_model_inferencer_close(ctx, inferencer);
  grn_language_model_close(ctx, model);
  grn_language_model_loader_close(ctx, loader);

  return ctx->rc;
}

grn_rc
GRN_PLUGIN_INIT(grn_ctx *ctx)
{
  return ctx->rc;
}

grn_rc
GRN_PLUGIN_REGISTER(grn_ctx *ctx)
{
  grn_rc rc = GRN_SUCCESS;

  {
    grn_obj *proc = grn_proc_create(ctx,
                                    "language_model_vectorize",
                                    -1,
                                    GRN_PROC_FUNCTION,
                                    func_language_model_vectorize,
                                    NULL,
                                    NULL,
                                    0,
                                    NULL);
    grn_proc_set_applier(ctx, proc, applier_language_model_vectorize);
  }

  return rc;
}

grn_rc
GRN_PLUGIN_FIN(grn_ctx *ctx)
{
  return GRN_SUCCESS;
}
