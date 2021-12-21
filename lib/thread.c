/*
  Copyright(C) 2015  Brazil
  Copyright(C) 2018-2021  Sutou Kouhei <kou@clear-code.com>

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
#include "grn_windows.h"

#ifdef _WIN32
# include <stdio.h>
# include <tlhelp32.h>
#endif

static grn_thread_get_limit_func get_limit_func = NULL;
static void *get_limit_func_data = NULL;
static grn_thread_set_limit_func set_limit_func = NULL;
static void *set_limit_func_data = NULL;

static grn_thread_get_limit_with_ctx_func get_limit_with_ctx_func = NULL;
static void *get_limit_with_ctx_func_data = NULL;
static grn_thread_set_limit_with_ctx_func set_limit_with_ctx_func = NULL;
static void *set_limit_with_ctx_func_data = NULL;

uint32_t
grn_thread_get_limit(void)
{
  if (get_limit_func) {
    return get_limit_func(get_limit_func_data);
  } else {
    return 0;
  }
}

void
grn_thread_set_limit(uint32_t new_limit)
{
  if (!set_limit_func) {
    return;
  }

  set_limit_func(new_limit, set_limit_func_data);
}

uint32_t
grn_thread_get_limit_with_ctx(grn_ctx *ctx)
{
  if (get_limit_with_ctx_func) {
    return get_limit_with_ctx_func(ctx, get_limit_with_ctx_func_data);
  } else {
    return grn_thread_get_limit();
  }
}

void
grn_thread_set_limit_with_ctx(grn_ctx *ctx, uint32_t new_limit)
{
  if (set_limit_with_ctx_func) {
    set_limit_with_ctx_func(ctx, new_limit, set_limit_func_data);
  } else {
    grn_thread_set_limit(new_limit);
  }
}

void
grn_thread_set_get_limit_func(grn_thread_get_limit_func func,
                              void *data)
{
  get_limit_func = func;
  get_limit_func_data = data;
}

void
grn_thread_set_set_limit_func(grn_thread_set_limit_func func, void *data)
{
  set_limit_func = func;
  set_limit_func_data = data;
}

void
grn_thread_set_get_limit_with_ctx_func(grn_thread_get_limit_with_ctx_func func,
                                       void *data)
{
  get_limit_with_ctx_func = func;
  get_limit_with_ctx_func_data = data;
}

void
grn_thread_set_set_limit_with_ctx_func(grn_thread_set_limit_with_ctx_func func,
                                       void *data)
{
  set_limit_with_ctx_func = func;
  set_limit_with_ctx_func_data = data;
}

grn_rc
grn_thread_dump(grn_ctx *ctx)
{
  GRN_API_ENTER;
#ifdef _WIN32
  grn_log_level level = GRN_LOG_NOTICE;
  HANDLE thread_snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
  if (thread_snapshot == INVALID_HANDLE_VALUE) {
    SERR("[thread][dump] failed to create thread snapshot");
    GRN_API_RETURN(ctx->rc);
  }
  HANDLE process = GetCurrentProcess();
  DWORD process_id = GetCurrentProcessId();
  THREADENTRY32 entry;
  entry.dwSize = sizeof(entry);
  if (!Thread32First(thread_snapshot, &entry)) {
    SERR("[thread][dump] failed to retrieve the first thread");
  }
  if (ctx->rc == GRN_SUCCESS) {
    do {
      if (entry.th32OwnerProcessID != process_id) {
        continue;
      }

      HANDLE thread = OpenThread(THREAD_GET_CONTEXT,
                                 FALSE,
                                 entry.th32ThreadID);
      if (thread == INVALID_HANDLE_VALUE) {
        continue;
      }
      CONTEXT context;
      context.ContextFlags = CONTEXT_FULL;
      if (GetThreadContext(thread, &context)) {
        GRN_LOG(ctx, level, "-- Thread %08" GRN_FMT_DWORD " --",
                entry.th32ThreadID);
        grn_windows_log_back_trace(ctx,
                                   level,
                                   process,
                                   thread,
                                   &context);
        GRN_LOG(ctx, level, "---------------------");
      }
      CloseHandle(thread);
    } while (Thread32Next(thread_snapshot, &entry));
  }
  CloseHandle(thread_snapshot);
#else
#endif
  GRN_API_RETURN(ctx->rc);
}
