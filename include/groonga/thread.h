/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2015 Brazil

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

#ifndef GROONGA_THREAD_H
#define GROONGA_THREAD_H

#ifdef  __cplusplus
extern "C" {
#endif

GRN_API uint32_t grn_thread_get_count(void);
GRN_API void grn_thread_set_count(uint32_t new_count);


typedef uint32_t (*grn_thread_get_count_func)(void *data);
GRN_API void grn_thread_set_get_count_func(grn_thread_get_count_func func,
                                           void *data);
typedef void (*grn_thread_set_count_func)(uint32_t new_count, void *data);
GRN_API void grn_thread_set_set_count_func(grn_thread_set_count_func func,
                                           void *data);

#ifdef __cplusplus
}
#endif

#endif /* GROONGA_THREAD_H */
