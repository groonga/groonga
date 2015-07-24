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

#include "grn_ctx.h"

static grn_thread_get_count_func get_count_func = NULL;
static void *get_count_func_data = NULL;
static grn_thread_set_count_func set_count_func = NULL;
static void *set_count_func_data = NULL;

uint32_t
grn_thread_get_count(void)
{
  if (get_count_func) {
    return get_count_func(get_count_func_data);
  } else {
    return 0;
  }
}

void
grn_thread_set_count(uint32_t new_count)
{
  if (!set_count_func) {
    return;
  }

  set_count_func(new_count, set_count_func_data);
}

void
grn_thread_set_get_count_func(grn_thread_get_count_func func,
                              void *data)
{
  get_count_func = func;
  get_count_func_data = data;
}

void
grn_thread_set_set_count_func(grn_thread_set_count_func func, void *data)
{
  set_count_func = func;
  set_count_func_data = data;
}
