/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2012 Brazil

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
#ifndef GRN_NORMALIZER_H
#define GRN_NORMALIZER_H

#include <groonga/plugin.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GRN_NORMALIZER_REGISTER(ctx, name, init_func, normalize_func, fin_func)\
  (grn_proc_create((ctx), (name), strlen((name)),\
                   GRN_PROC_NORMALIZER,\
                   (init_func), (normalize_func), (fin_func),\
                   0, NULL))

#ifdef __cplusplus
}
#endif

#endif /* GRN_NORMALIZER_H */
