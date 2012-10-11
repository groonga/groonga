/* -*- c-basic-offset: 2 -*- */
/* Copyright(C) 2012 Brazil

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

#include <groonga/plugin.h>

/* groonga's internal headers: They should be removed. */
/* for grn_text_fgets() */
#include <str.h>
/* for grn_win32_base_dir() */
#include <util.h>

#include <stdio.h>
#include <string.h>

#define MAX_SYNONYM_BYTES 4096

static grn_hash *synonyms = NULL;

#ifdef WIN32
static char *win32_synonyms_file = NULL;
const char *
get_system_synonyms_file(void)
{
  if (!win32_synonyms_file) {
    const char *base_dir;
    const char *relative_path = GRN_RELATIVE_QUERY_EXPANDER_TSV_SYNONYMS_FILE;
    char *synonyms_file;
    char *path;
    size_t base_dir_length;

    base_dir = grn_win32_base_dir();
    base_dir_length = strlen(base_dir);
    synonyms_file =
      malloc(base_dir_length + strlen("/") + strlen(relative_path) + 1);
    strcpy(synonyms_file, base_dir);
    strcat(synonyms_file, "/");
    strcat(synonyms_file, relative_path);
    win32_synonyms_file = synonyms_file;
  }
  return win32_synonyms_file;
}

#else /* WIN32 */
const char *
get_system_synonyms_file(void)
{
  return GRN_QUERY_EXPANDER_TSV_SYNONYMS_FILE;
}
#endif /* WIN32 */

static inline grn_bool
is_comment_mark(char character)
{
  return character == '#';
}

static void
parse_synonyms_file_line(grn_ctx *ctx, const char *line, int line_length,
                         grn_obj *key, grn_obj *value)
{
  size_t i = 0;

  if (is_comment_mark(line[i])) {
    return;
  }

  while (i < line_length) {
    char character = line[i];
    i++;
    if (character == '\t') {
      break;
    }
    GRN_TEXT_PUTC(ctx, key, character);
  }

  if (i == line_length) {
    return;
  }

  GRN_TEXT_PUTS(ctx, value, "((");
  while (i < line_length) {
    char character = line[i];
    i++;
    if (character == '\t') {
      GRN_TEXT_PUTS(ctx, value, ") OR (");
    } else {
      GRN_TEXT_PUTC(ctx, value, character);
    }
  }
  GRN_TEXT_PUTS(ctx, value, "))");

  {
    grn_id id;
    void *value_location = NULL;

    id = grn_hash_add(ctx, synonyms, GRN_TEXT_VALUE(key), GRN_TEXT_LEN(key),
                      &value_location, NULL);
    if (id == GRN_ID_NIL) {
      GRN_PLUGIN_LOG(ctx, GRN_LOG_WARNING,
                     "[plugin][query-expander][tsv] "
                     "failed to register key: <%.*s>",
                     (int)GRN_TEXT_LEN(key), GRN_TEXT_VALUE(key));
      return;
    }

    grn_bulk_truncate(ctx, value, MAX_SYNONYM_BYTES - 1);
    GRN_TEXT_PUTC(ctx, value, '\0');
    memcpy(value_location, GRN_TEXT_VALUE(value), MAX_SYNONYM_BYTES);
  }
}

static void
load_synonyms(grn_ctx *ctx)
{
  const char *path;
  FILE *file;
  grn_obj line, key, value;

  path = getenv("GRN_QUERY_EXPANDER_TSV_SYNONYMS_FILE");
  if (!path) {
    path = get_system_synonyms_file();
  }
  file = fopen(path, "r");
  if (!file) {
    GRN_LOG(ctx, GRN_LOG_WARNING,
            "[plugin][query-expander][tsv] "
            "synonyms file doesn't exist: <%s>",
            path);
    return;
  }

  GRN_TEXT_INIT(&line, 0);
  GRN_TEXT_INIT(&key, 0);
  GRN_TEXT_INIT(&value, 0);
  grn_bulk_reserve(ctx, &value, MAX_SYNONYM_BYTES);
  while (grn_text_fgets(ctx, &line, file) == GRN_SUCCESS) {
    GRN_BULK_REWIND(&key);
    GRN_BULK_REWIND(&value);
    parse_synonyms_file_line(ctx,
                             GRN_TEXT_VALUE(&line), GRN_TEXT_LEN(&line),
                             &key, &value);
    GRN_BULK_REWIND(&line);
  }
  GRN_OBJ_FIN(ctx, &line);
  GRN_OBJ_FIN(ctx, &key);
  GRN_OBJ_FIN(ctx, &value);

  fclose(file);
}

static grn_obj *
func_query_expander_tsv(grn_ctx *ctx, int nargs, grn_obj **args,
                        grn_user_data *user_data)
{
  grn_rc rc = GRN_END_OF_DATA;
  grn_id id;
  grn_obj *term, *expanded_term;
  void *value;
  grn_obj *rc_object;

  term = args[0];
  expanded_term = args[1];
  id = grn_hash_get(ctx, synonyms,
                    GRN_TEXT_VALUE(term), GRN_TEXT_LEN(term),
                    &value);
  if (id != GRN_ID_NIL) {
    const char *query = value;
    GRN_TEXT_PUTS(ctx, expanded_term, query);
    rc = GRN_SUCCESS;
  }

  rc_object = grn_plugin_proc_alloc(ctx, user_data, GRN_DB_INT32, 0);
  if (rc_object) {
    GRN_INT32_SET(ctx, rc_object, rc);
  }

  return rc_object;
}

grn_rc
GRN_PLUGIN_INIT(grn_ctx *ctx)
{
  if (!synonyms) {
    synonyms = grn_hash_create(ctx, NULL,
                               GRN_TABLE_MAX_KEY_SIZE,
                               MAX_SYNONYM_BYTES,
                               GRN_OBJ_TABLE_HASH_KEY | GRN_OBJ_KEY_VAR_SIZE);
    if (!synonyms) {
      return ctx->rc;
    }
    load_synonyms(ctx);
  }
  return ctx->rc;
}

grn_rc
GRN_PLUGIN_REGISTER(grn_ctx *ctx)
{
  grn_proc_create(ctx, "QueryExpanderTSV", strlen("QueryExpanderTSV"),
                  GRN_PROC_FUNCTION,
                  func_query_expander_tsv, NULL, NULL,
                  0, NULL);
  return GRN_SUCCESS;
}

grn_rc
GRN_PLUGIN_FIN(grn_ctx *ctx)
{
  if (synonyms) {
    grn_hash_close(ctx, synonyms);
    synonyms = NULL;
  }
  return GRN_SUCCESS;
}
