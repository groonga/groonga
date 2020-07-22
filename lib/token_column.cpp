/* -*- c-basic-offset: 2 -*- */
/*
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

#include <cstring>

#include "grn.h"
#include "grn_str.h"
#include "grn_token_column.h"
#include "grn_token_cursor.h"

#ifdef GRN_WITH_APACHE_ARROW
# include "grn_arrow.hpp"
# include <arrow/util/thread_pool.h>
# include <mutex>
#endif

namespace grn {
  namespace token_column {
    static uint32_t parallel_chunk_size = 1024;
    static uint32_t parallel_table_size_threshold = parallel_chunk_size * 10;

    class Builder
    {
    public:
      Builder(grn_ctx *ctx,
              grn_obj *column)
        : ctx_(ctx),
          column_(column),
          table_(grn_ctx_at(ctx, column->header.domain)),
          lexicon_(grn_ctx_at(ctx, DB_OBJ(column)->range)),
          source_(NULL) {
        grn_id *source_ids = static_cast<grn_id *>(DB_OBJ(column)->source);
        source_ = grn_ctx_at(ctx, source_ids[0]);
      }

      ~Builder() {
        grn_obj_unref(ctx_, source_);
        grn_obj_unref(ctx_, lexicon_);
        grn_obj_unref(ctx_, table_);
      }

      void
      build() {
        grn_obj_set_visibility(ctx_, column_, false);
        if (use_parallel()) {
          build_parallel();
        } else {
          build_sequential();
        }
        grn_obj_set_visibility(ctx_, column_, true);
      }

    private:
      bool
      use_parallel() {
#ifdef GRN_WITH_APACHE_ARROW
        return grn_table_size(ctx_, table_) >= parallel_table_size_threshold;
#else
        return false;
#endif
      }

      void
      build_parallel() {
#ifdef GRN_WITH_APACHE_ARROW
        std::mutex mutex;
        auto pool = arrow::internal::GetCpuThreadPool();
        uint32_t token_cursor_flags = GRN_TOKEN_CURSOR_PARALLEL;
        grn_obj *db = grn_ctx_db(ctx_);
        uint32_t chunk_size = parallel_chunk_size;
        auto build_chunk = [&](std::vector<grn_id> ids) {
          grn_ctx ctx;
          grn_ctx_init(&ctx, 0);
          grn_ctx_use(&ctx, db);
          grn_obj tokens;
          grn_obj value;
          GRN_RECORD_INIT(&tokens, GRN_OBJ_VECTOR, DB_OBJ(lexicon_)->id);
          GRN_VOID_INIT(&value);
          for (auto id : ids) {
            GRN_BULK_REWIND(&tokens);
            GRN_BULK_REWIND(&value);
            grn_obj_get_value(&ctx, source_, id, &value);
            if (GRN_TEXT_LEN(&value) > 0) {
              auto token_cursor = grn_token_cursor_open(&ctx,
                                                        lexicon_,
                                                        GRN_BULK_HEAD(&value),
                                                        GRN_BULK_VSIZE(&value),
                                                        GRN_TOKEN_ADD,
                                                        token_cursor_flags);
              if (token_cursor) {
                while (token_cursor->status == GRN_TOKEN_CURSOR_DOING) {
                  grn_id token_id = grn_token_cursor_next(&ctx, token_cursor);
                  if (token_id == GRN_ID_NIL) {
                    break;
                  }
                  GRN_RECORD_PUT(&ctx, &tokens, token_id);
                }
                grn_token_cursor_close(&ctx, token_cursor);
              }
            }
            grn_obj_set_value(&ctx, column_, id, &tokens, GRN_OBJ_SET);
            if (ctx.rc != GRN_SUCCESS) {
              break;
            }
          }
          GRN_OBJ_FIN(&ctx, &value);
          GRN_OBJ_FIN(&ctx, &tokens);
          if (ctx.rc != GRN_SUCCESS) {
            std::lock_guard<std::mutex> lock(mutex);
            if (ctx_->rc == GRN_SUCCESS) {
              ctx_->rc = ctx.rc;
              ctx_->errlvl = ctx.errlvl;
              ctx_->errfile = ctx.errfile;
              ctx_->errline = ctx.errline;
              ctx_->errfunc = ctx.errfunc;
              grn_strcpy(ctx_->errbuf, GRN_CTX_MSGSIZE, ctx.errbuf);
            }
          }
          grn_ctx_fin(&ctx);
          return arrow::Status::OK();
        };

        std::vector<grn_id> ids;
        std::vector<arrow::Future<arrow::Status>> futures;
        GRN_TABLE_EACH_BEGIN_FLAGS(ctx_, table_, cursor, id, GRN_CURSOR_BY_ID) {
          ids.push_back(id);
          if (ids.size() == chunk_size) {
            auto future = pool->Submit(build_chunk, ids);
            if (!grnarrow::check(ctx_,
                                 future,
                                 "[token-column][build][parallel] "
                                 "failed to submit a job")) {
              break;
            }
            futures.push_back(*future);
            ids.clear();
          }
        } GRN_TABLE_EACH_END(ctx_, cursor);
        if (ctx_->rc == GRN_SUCCESS && !ids.empty()) {
          auto future = pool->Submit(build_chunk, ids);
          if (grnarrow::check(ctx_,
                              future,
                              "[token-column][build][parallel] "
                              "failed to submit a job")) {
            futures.push_back(*future);
          }
        }
        auto status = arrow::Status::OK();
        for (auto& future : futures) {
          status &= future.status();
        }
        grnarrow::check(ctx_,
                        status,
                        "[token-column][build][parallel] "
                        "failed to complete a job");
#endif
      }

      void
      build_sequential() {
        grn_obj tokens;
        grn_obj value;
        GRN_RECORD_INIT(&tokens, GRN_OBJ_VECTOR, DB_OBJ(lexicon_)->id);
        GRN_VOID_INIT(&value);
        unsigned int token_flags = 0;
        GRN_TABLE_EACH_BEGIN_FLAGS(ctx_, table_, cursor, id, GRN_CURSOR_BY_ID) {
          GRN_BULK_REWIND(&tokens);
          GRN_BULK_REWIND(&value);
          grn_obj_get_value(ctx_, source_, id, &value);
          if (GRN_TEXT_LEN(&value) > 0) {
            grn_token_cursor *token_cursor =
              grn_token_cursor_open(ctx_,
                                    lexicon_,
                                    GRN_BULK_HEAD(&value),
                                    GRN_BULK_VSIZE(&value),
                                    GRN_TOKEN_ADD,
                                    token_flags);
            if (token_cursor) {
              while (token_cursor->status == GRN_TOKEN_CURSOR_DOING) {
                grn_id token_id = grn_token_cursor_next(ctx_, token_cursor);
                if (token_id == GRN_ID_NIL) {
                  break;
                }
                GRN_RECORD_PUT(ctx_, &tokens, token_id);
              }
              grn_token_cursor_close(ctx_, token_cursor);
            }
          }
          grn_obj_set_value(ctx_, column_, id, &tokens, GRN_OBJ_SET);
          if (ctx_->rc != GRN_SUCCESS) {
            break;
          }
        } GRN_TABLE_EACH_END(ctx_, cursor);
        GRN_OBJ_FIN(ctx_, &value);
        GRN_OBJ_FIN(ctx_, &tokens);
      }

      grn_ctx *ctx_;
      grn_obj *column_;
      grn_obj *table_;
      grn_obj *lexicon_;
      grn_obj *source_;
    };
  }
}

extern "C" {
  void
  grn_token_column_init_from_env(void)
  {
    {
      char grn_token_column_parallel_chunk_size_env[GRN_ENV_BUFFER_SIZE];
      grn_getenv("GRN_TOKEN_COLUMN_PARALLEL_CHUNK_SIZE",
                 grn_token_column_parallel_chunk_size_env,
                 GRN_ENV_BUFFER_SIZE);
      if (grn_token_column_parallel_chunk_size_env[0]) {
        size_t env_len = strlen(grn_token_column_parallel_chunk_size_env);
        uint32_t chunk_size =
          grn_atoui(grn_token_column_parallel_chunk_size_env,
                    grn_token_column_parallel_chunk_size_env + env_len,
                    NULL);
        if (chunk_size > 0) {
          grn::token_column::parallel_chunk_size = chunk_size;
        }
      }
    }

    {
      char grn_token_column_parallel_table_size_threshold_env[GRN_ENV_BUFFER_SIZE];
      grn_getenv("GRN_TOKEN_COLUMN_PARALLEL_TABLE_SIZE_THRESHOLD",
                 grn_token_column_parallel_table_size_threshold_env,
                 GRN_ENV_BUFFER_SIZE);
      if (grn_token_column_parallel_table_size_threshold_env[0]) {
        size_t env_len =
          strlen(grn_token_column_parallel_table_size_threshold_env);
        uint32_t threshold =
          grn_atoui(grn_token_column_parallel_table_size_threshold_env,
                    grn_token_column_parallel_table_size_threshold_env + env_len,
                    NULL);
        if (threshold > 0) {
          grn::token_column::parallel_table_size_threshold = threshold;
        }
      }
    }
  }

  void
  grn_token_column_update(grn_ctx *ctx,
                          grn_obj *column,
                          grn_id id,
                          int section,
                          grn_obj *old_value,
                          grn_obj *new_value)
  {
    grn_obj *lexicon = grn_ctx_at(ctx, DB_OBJ(column)->range);
    grn_obj tokens;
    GRN_RECORD_INIT(&tokens, GRN_OBJ_VECTOR, DB_OBJ(lexicon)->id);
    unsigned int token_flags = 0;
    grn_token_cursor *token_cursor =
      grn_token_cursor_open(ctx,
                            lexicon,
                            GRN_TEXT_VALUE(new_value),
                            GRN_TEXT_LEN(new_value),
                            GRN_TOKEN_ADD,
                            token_flags);
    if (token_cursor) {
      while (token_cursor->status == GRN_TOKEN_CURSOR_DOING) {
        grn_id token_id = grn_token_cursor_next(ctx, token_cursor);
        if (token_id == GRN_ID_NIL) {
          break;
        }
        GRN_RECORD_PUT(ctx, &tokens, token_id);
      }
      grn_token_cursor_close(ctx, token_cursor);
    }
    grn_obj_set_value(ctx, column, id, &tokens, GRN_OBJ_SET);
    GRN_OBJ_FIN(ctx, &tokens);
    grn_obj_unref(ctx, lexicon);
  }

  void
  grn_token_column_build(grn_ctx *ctx, grn_obj *column)
  {
    grn::token_column::Builder builder(ctx, column);
    builder.build();
  }
}
