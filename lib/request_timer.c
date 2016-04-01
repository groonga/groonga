/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2016 Brazil

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

#include "grn_ctx.h"
#include "grn_request_timer.h"

static grn_ctx grn_request_timer_ctx;
static grn_mutex grn_request_timer_mutex;
static grn_request_timer grn_current_request_timer = { 0 };

grn_bool
grn_request_timer_init(void)
{
  grn_ctx *ctx = &grn_request_timer_ctx;

  grn_ctx_init(ctx, 0);

  MUTEX_INIT(grn_request_timer_mutex);

  return GRN_TRUE;
}

void *
grn_request_timer_register(grn_ctx *ctx,
                           const char *request_id,
                           unsigned int request_id_size,
                           double timeout)
{
  void *timer_id = NULL;

  MUTEX_LOCK(grn_request_timer_mutex);
  if (grn_current_request_timer.register_func) {
    void *user_data = grn_current_request_timer.user_data;
    timer_id = grn_current_request_timer.register_func(ctx,
                                                       request_id,
                                                       request_id_size,
                                                       timeout,
                                                       user_data);
  }
  MUTEX_UNLOCK(grn_request_timer_mutex);

  return timer_id;
}

void
grn_request_timer_unregister(grn_ctx *ctx, void *timer_id)
{
  MUTEX_LOCK(grn_request_timer_mutex);
  if (grn_current_request_timer.unregister_func) {
    void *user_data = grn_current_request_timer.user_data;
    grn_current_request_timer.unregister_func(ctx, timer_id, user_data);
  }
  MUTEX_UNLOCK(grn_request_timer_mutex);
}

void
grn_request_timer_set(grn_ctx *ctx, grn_request_timer *timer)
{
  MUTEX_LOCK(grn_request_timer_mutex);
  if (grn_current_request_timer.fin_func) {
    void *user_data = grn_current_request_timer.user_data;
    grn_current_request_timer.fin_func(ctx, user_data);
  }
  if (timer) {
    grn_current_request_timer = *timer;
  } else {
    memset(&grn_current_request_timer, 0, sizeof(grn_request_timer));
  }
  MUTEX_UNLOCK(grn_request_timer_mutex);
}

void
grn_request_timer_fin(void)
{
  grn_ctx *ctx = &grn_request_timer_ctx;

  grn_request_timer_set(ctx, NULL);
  MUTEX_FIN(grn_request_timer_mutex);
  grn_ctx_fin(ctx);
}
