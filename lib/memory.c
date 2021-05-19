/*
  Copyright(C) 2019 Kouhei Sutou <kou@clear-code.com>

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

#include <stdio.h>

#ifdef WIN32
/* For Windows Server 2018 or earlier. */
# define PSAPI_VERSION 1
# include <psapi.h>
#endif

uint64_t
grn_memory_get_usage(grn_ctx *ctx)
{
#ifdef WIN32
  PROCESS_MEMORY_COUNTERS_EX counters;
  if (!GetProcessMemoryInfo(GetCurrentProcess(),
                            (PPROCESS_MEMORY_COUNTERS)&counters,
                            sizeof(counters))) {
    SERR("GetProcessMemoryInfo");
    return 0;
  }
  return counters.PrivateUsage;
#elif defined(HAVE_GETRUSAGE)
  struct rusage usage;
  if (getrusage(RUSAGE_SELF, &usage) != 0) {
    SERR("getrusage");
    return 0;
  }
  return usage.ru_maxrss * 1024;
#else
  return 0;
#endif
}
