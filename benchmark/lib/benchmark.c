/*
  Copyright (C) 2008-2013  Kouhei Sutou <kou@clear-code.com>

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

#include "benchmark.h"

void
bench_init(gint *argc, gchar ***argv)
{
#if !GLIB_CHECK_VERSION(2, 36, 0)
  if (!g_thread_supported())
    g_thread_init(NULL);

  g_type_init();
#endif
}

void
bench_quit(void)
{
}
