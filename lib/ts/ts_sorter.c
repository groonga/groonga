/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2015 Brazil

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

#include "ts_sorter.h"

#include <string.h>

#include "ts_log.h"
#include "ts_util.h"

/*-------------------------------------------------------------
 * grn_ts_sorter_node.
 */

/* grn_ts_sorter_node_init() initializes a sorter node. */
static void
grn_ts_sorter_node_init(grn_ctx *ctx, grn_ts_sorter_node *node)
{
  memset(node, 0, sizeof(*node));
  node->expr = NULL;
  node->next = NULL;
}

/* grn_ts_sorter_node_fin() finalizes a sorter node. */
static void
grn_ts_sorter_node_fin(grn_ctx *ctx, grn_ts_sorter_node *node)
{
  if (node->expr) {
    grn_ts_expr_close(ctx, node->expr);
  }
}

/* grn_ts_sorter_node_open() creates a sorter nodes. */
static grn_rc
grn_ts_sorter_node_open(grn_ctx *ctx, grn_ts_expr *expr, grn_ts_bool reverse,
                        grn_ts_sorter_node **node)
{
  grn_ts_sorter_node *new_node;
  new_node = GRN_MALLOCN(grn_ts_sorter_node, 1);
  if (!new_node) {
    GRN_TS_ERR_RETURN(GRN_NO_MEMORY_AVAILABLE,
                      "GRN_MALLOCN failed: %" GRN_FMT_SIZE " x 1",
                      sizeof(grn_ts_sorter_node));
  }
  grn_ts_sorter_node_init(ctx, new_node);
  new_node->expr = expr;
  new_node->reverse = reverse;
  *node = new_node;
  return GRN_SUCCESS;
}

/* grn_ts_sorter_node_close() destroys a sorter node. */
static void
grn_ts_sorter_node_close(grn_ctx *ctx, grn_ts_sorter_node *node)
{
  grn_ts_sorter_node_fin(ctx, node);
  GRN_FREE(node);
}

/* grn_ts_sorter_node_list_close() destroys a linked list of sorter nodes. */
static void
grn_ts_sorter_node_list_close(grn_ctx *ctx, grn_ts_sorter_node *head)
{
  grn_ts_sorter_node *node = head;
  while (node) {
    grn_ts_sorter_node *next = node->next;
    grn_ts_sorter_node_close(ctx, node);
    node = next;
  }
}

/*-------------------------------------------------------------
 * grn_ts_sorter.
 */

static void
grn_ts_sorter_init(grn_ctx *ctx, grn_ts_sorter *sorter)
{
  memset(sorter, 0, sizeof(*sorter));
  sorter->table = NULL;
  sorter->head = NULL;
}

static void
grn_ts_sorter_fin(grn_ctx *ctx, grn_ts_sorter *sorter)
{
  if (sorter->head) {
    grn_ts_sorter_node_list_close(ctx, sorter->head);
  }
  if (sorter->table) {
    grn_obj_unlink(ctx, sorter->table);
  }
}

grn_rc
grn_ts_sorter_open(grn_ctx *ctx, grn_obj *table, grn_ts_sorter_node *head,
                   grn_ts_int offset, grn_ts_int limit, grn_ts_sorter **sorter)
{
  grn_rc rc;
  grn_ts_sorter *new_sorter;
  if (!ctx) {
    return GRN_INVALID_ARGUMENT;
  }
  if (!table || !grn_ts_obj_is_table(ctx, table) || !head || !sorter) {
    GRN_TS_ERR_RETURN(GRN_INVALID_ARGUMENT, "invalid argument");
  }
  new_sorter = GRN_MALLOCN(grn_ts_sorter, 1);
  if (!new_sorter) {
    GRN_TS_ERR_RETURN(GRN_INVALID_ARGUMENT,
                      "GRN_MALLOCN failed: %" GRN_FMT_SIZE " x 1",
                      sizeof(grn_ts_sorter));
  }
  rc = grn_ts_obj_increment_ref_count(ctx, table);
  if (rc != GRN_SUCCESS) {
    GRN_FREE(new_sorter);
    return rc;
  }
  grn_ts_sorter_init(ctx, new_sorter);
  new_sorter->table = table;
  new_sorter->head = head;
  new_sorter->offset = offset;
  new_sorter->limit = limit;
  *sorter = new_sorter;
  return GRN_SUCCESS;
}

grn_rc
grn_ts_sorter_parse(grn_ctx *ctx, grn_obj *table,
                    grn_ts_str str, grn_ts_int offset,
                    grn_ts_int limit, grn_ts_sorter **sorter)
{
  // TODO
  return GRN_FUNCTION_NOT_IMPLEMENTED;
}

grn_rc
grn_ts_sorter_close(grn_ctx *ctx, grn_ts_sorter *sorter)
{
  if (!ctx) {
    return GRN_INVALID_ARGUMENT;
  }
  if (!sorter) {
    GRN_TS_ERR_RETURN(GRN_INVALID_ARGUMENT, "invalid argument");
  }
  grn_ts_sorter_fin(ctx, sorter);
  GRN_FREE(sorter);
  return GRN_SUCCESS;
}

grn_rc
grn_ts_sorter_progress(grn_ctx *ctx, grn_ts_sorter *sorter,
                       grn_ts_record *recs, size_t n_recs)
{
  // TODO
  return GRN_FUNCTION_NOT_IMPLEMENTED;
}

grn_rc
grn_ts_sorter_complete(grn_ctx *ctx, grn_ts_sorter *sorter,
                       grn_ts_record *recs, size_t n_recs)
{
  // TODO
  return GRN_FUNCTION_NOT_IMPLEMENTED;
}

/*-------------------------------------------------------------
 * grn_ts_sorter_builder.
 */

/* grn_ts_sorter_builder_init() initializes a sorter builder. */
static void
grn_ts_sorter_builder_init(grn_ctx *ctx, grn_ts_sorter_builder *builder)
{
  memset(builder, 0, sizeof(*builder));
  builder->table = NULL;
  builder->head = NULL;
  builder->tail = NULL;
}

/* grn_ts_sorter_builder_fin() finalizes a sorter builder. */
static void
grn_ts_sorter_builder_fin(grn_ctx *ctx, grn_ts_sorter_builder *builder)
{
  if (builder->head) {
    grn_ts_sorter_node_list_close(ctx, builder->head);
  }
  if (builder->table) {
    grn_obj_unlink(ctx, builder->table);
  }
}

grn_rc
grn_ts_sorter_builder_open(grn_ctx *ctx, grn_obj *table,
                           grn_ts_sorter_builder **builder)
{
  grn_rc rc;
  grn_ts_sorter_builder *new_builder;
  if (!ctx) {
    return GRN_INVALID_ARGUMENT;
  }
  if (!table || !grn_ts_obj_is_table(ctx, table) || !builder) {
    GRN_TS_ERR_RETURN(GRN_INVALID_ARGUMENT, "invalid argument");
  }
  new_builder = GRN_MALLOCN(grn_ts_sorter_builder, 1);
  if (!new_builder) {
    GRN_TS_ERR_RETURN(GRN_NO_MEMORY_AVAILABLE,
                      "GRN_MALLOCN failed: %" GRN_FMT_SIZE " x 1",
                      sizeof(grn_ts_sorter_builder));
  }
  grn_ts_sorter_builder_init(ctx, new_builder);
  rc = grn_ts_obj_increment_ref_count(ctx, table);
  if (rc != GRN_SUCCESS) {
    grn_ts_sorter_builder_fin(ctx, new_builder);
    GRN_FREE(new_builder);
    return rc;
  }
  new_builder->table = table;
  *builder = new_builder;
  return GRN_SUCCESS;
}

grn_rc
grn_ts_sorter_builder_close(grn_ctx *ctx, grn_ts_sorter_builder *builder)
{
  if (!ctx) {
    return GRN_INVALID_ARGUMENT;
  }
  if (!builder) {
    GRN_TS_ERR_RETURN(GRN_INVALID_ARGUMENT, "invalid argument");
  }
  grn_ts_sorter_builder_fin(ctx, builder);
  GRN_FREE(builder);
  return GRN_SUCCESS;
}

grn_rc
grn_ts_sorter_builder_complete(grn_ctx *ctx, grn_ts_sorter_builder *builder,
                               grn_ts_int offset, grn_ts_int limit,
                               grn_ts_sorter **sorter)
{
  grn_rc rc;
  grn_ts_sorter *new_sorter;
  if (!ctx) {
    return GRN_INVALID_ARGUMENT;
  }
  if (!builder || !builder->head || !sorter) {
    GRN_TS_ERR_RETURN(GRN_INVALID_ARGUMENT, "invalid argument");
  }
  rc = grn_ts_sorter_open(ctx, builder->table, builder->head,
                          offset, limit, &new_sorter);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  builder->head = NULL;
  builder->tail = NULL;
  *sorter = new_sorter;
  return GRN_SUCCESS;
}

grn_rc
grn_ts_sorter_builder_push(grn_ctx *ctx, grn_ts_sorter_builder *builder,
                           grn_ts_expr *expr, grn_ts_bool reverse)
{
  grn_rc rc;
  grn_ts_sorter_node *new_node;
  if (!ctx) {
    return GRN_INVALID_ARGUMENT;
  }
  if (!builder || !expr) {
    GRN_TS_ERR_RETURN(GRN_INVALID_ARGUMENT, "invalid argument");
  }
  rc = grn_ts_sorter_node_open(ctx, expr, reverse, &new_node);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  if (builder->tail) {
    builder->tail->next = new_node;
  } else {
    builder->head = new_node;
  }
  builder->tail = new_node;
  return GRN_SUCCESS;
}
