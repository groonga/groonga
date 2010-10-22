/* -*- c-basic-offset: 2 -*- */
/* Copyright(C) 2010 Brazil

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License version 2.1 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifndef GRN_MODULE_IMPL_H
#define GRN_MODULE_IMPL_H

#include "groonga.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GRN_MODULE_INIT grn_module_impl_init
#define GRN_MODULE_REGISTER grn_module_impl_register
#define GRN_MODULE_FIN grn_module_impl_fin

#if defined(_WIN32) || defined(_WIN64)
#  define GRN_MODULE_EXPORT __declspec(dllexpoert)
#else /* defined(_WIN32) || defined(_WIN64) */
#  define GRN_MODULE_EXPORT
#endif /* defined(_WIN32) || defined(_WIN64) */

GRN_MODULE_EXPORT grn_rc GRN_MODULE_INIT(grn_ctx *ctx);
GRN_MODULE_EXPORT grn_rc GRN_MODULE_REGISTER(grn_ctx *ctx);
GRN_MODULE_EXPORT grn_rc GRN_MODULE_FIN(grn_ctx *ctx);

#ifdef __cplusplus
}
#endif

#endif /* GRN_MODULE_IMPL_H */
