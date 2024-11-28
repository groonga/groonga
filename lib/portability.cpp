/*
  Copyright (C) 2024  Abe Tomoaki <abe@clear-code.com>

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

#include <algorithm>
#include "grn.h"

extern "C" {

/* todo: Implement the general grn_qsort_r() that takes `base` as `void *`. */
void
grn_qsort_r_grn_id(grn_id *base,
                   size_t nmemb,
                   int (*compar)(const grn_id, const grn_id, void *),
                   void *arg)
{
  if (compar) {
    std::sort(base,
              base + nmemb,
              [compar, arg](const grn_id id1, const grn_id id2) {
                return compar(id1, id2, arg) == -1;
              });
  } else {
    std::sort(base, base + nmemb);
  }
}
}
