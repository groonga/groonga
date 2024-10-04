/*
  Copyright (C) 2024  Sutou Kouhei <kou@clear-code.com>

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

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Language model.
 *
 *        You need to use \ref grn_language_model_loader_load to load
 *        \ref grn_language_model.
 */
typedef struct grn_language_model_ grn_language_model;
/**
 * \brief Language model inferencer.
 *
 *        You need to use \ref grn_language_model_open_inferencer to
 *        open \ref grn_language_model_inferencer.
 */
typedef struct grn_language_model_inferencer_ grn_language_model_inferencer;
/**
 * \brief Language model loader.
 */
typedef struct grn_language_model_loader_ grn_language_model_loader;

/**
 * \brief Open a new language model loader.
 *
 * \param ctx The context object.
 *
 * \return A newly created language model loader on success, `NULL` on
 *         error.
 */
GRN_API grn_language_model_loader *
grn_language_model_loader_open(grn_ctx *ctx);
/**
 * \brief Close a language model loader.
 *
 * \param ctx The context object.
 * \param loader The loader to close.
 *
 * \return \ref GRN_SUCCESS on success, the appropriate \ref grn_rc on
 *         error.
 */
GRN_API grn_rc
grn_language_model_loader_close(grn_ctx *ctx,
                                grn_language_model_loader *loader);
/**
 * \brief Set language model name to load.
 *
 * \param ctx The context object.
 * \param loader The loader.
 * \param model The model name to load.
 * \param model_length The byte size of `model`. You can use `-1` if
 *                     `model` is a `\0`-terminated string.
 *
 * \return \ref GRN_SUCCESS on success, the appropriate \ref grn_rc on
 *         error.
 */
GRN_API grn_rc
grn_language_model_loader_set_model(grn_ctx *ctx,
                                    grn_language_model_loader *loader,
                                    const char *model,
                                    int64_t model_length);
/**
 * \brief Load a language model.
 *
 *        If the target model is already loaded, this reuses the model
 *        instead of loading a new model.
 *
 * \param ctx The context object.
 * \param loader The loader.
 *
 * \return A loaded \ref grn_language_model on success, `NULL` on error.
 *
 *         See `ctx->rc` for error details.
 */
GRN_API grn_language_model *
grn_language_model_loader_load(grn_ctx *ctx, grn_language_model_loader *loader);

/**
 * \brief Close a language model.
 *
 * \param ctx The context object.
 * \param model The model to close.
 *
 * \return \ref GRN_SUCCESS on success, the appropriate \ref grn_rc on
 *         error.
 */
GRN_API grn_rc
grn_language_model_close(grn_ctx *ctx, grn_language_model *model);
/**
 * \brief Open a new language model inferencer.
 *
 * \param ctx The context object.
 * \param model The model to inference.
 *
 * \return A newly created \ref grn_language_model_inferencer on
 *         success, `NULL` on error.
 *
 *         See `ctx->rc` for error details.
 */
GRN_API grn_language_model_inferencer *
grn_language_model_open_inferencer(grn_ctx *ctx, grn_language_model *model);

/**
 * \brief Close a language model inferencer.
 *
 * \param ctx The context object.
 * \param inferencer The inferencer to close.
 *
 * \return \ref GRN_SUCCESS on success, the appropriate \ref grn_rc on
 *         error.
 */
GRN_API grn_rc
grn_language_model_inferencer_close(grn_ctx *ctx,
                                    grn_language_model_inferencer *inferencer);
/**
 * \brief Vectorize a text.
 *
 *        In other words, compute embeddings of a text.
 *
 * \param ctx The context object.
 * \param inferencer The inferencer.
 * \param text The text to vectorize.
 * \param text_length The byte size of `text`. You can use `-1` if
 *                    `text` is a `\0`-terminated string.
 * \param output_vector \ref GRN_DB_FLOAT32 vector as an output. A
 *                      caller must initialize this by \ref
 *                      GRN_FLOAT32_INIT and \ref GRN_OBJ_VECTOR.
 *
 * \return \ref GRN_SUCCESS on success, the appropriate \ref grn_rc on
 *         error.
 */
GRN_API grn_rc
grn_language_model_inferencer_vectorize(
  grn_ctx *ctx,
  grn_language_model_inferencer *inferencer,
  const char *text,
  int64_t text_length,
  grn_obj *output_vector);

#ifdef __cplusplus
}
#endif
