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

#include "grn_logger.h"
#include "grn_ctx.h"

#include <string.h>

static char *windows_event_source_name = NULL;

#ifdef WIN32
typedef struct _grn_windows_event_logger_data {
  HANDLE event_log;
} grn_windows_event_logger_data;
#endif /* WIN32 */

const char *
grn_windows_event_logger_get_source_name(void)
{
  return windows_event_source_name;
}

void
grn_windows_event_logger_set_source_name(const char *name)
{
  if (windows_event_source_name) {
    free(windows_event_source_name);
  }

  if (name) {
    windows_event_source_name = grn_strdup_raw(name);
  } else {
    windows_event_source_name = NULL;
  }
}

#ifdef WIN32
static void
windows_event_logger_log(grn_ctx *ctx, grn_log_level level,
                         const char *timestamp, const char *title,
                         const char *message, const char *location,
                         void *user_data)
{
  grn_windows_event_logger_data *data = user_data;
  WORD type;
  WORD category = 0;
  DWORD event_id = 0;
  PSID user_sid = NULL;
  WORD n_strings = 1;
  DWORD data_size = 0;
  void *raw_data = NULL;

  switch (level) {
  case GRN_LOG_NONE :
    type = EVENTLOG_INFORMATION_TYPE;
    break;
  case GRN_LOG_EMERG :
  case GRN_LOG_ALERT :
  case GRN_LOG_CRIT :
  case GRN_LOG_ERROR :
    type = EVENTLOG_ERROR_TYPE;
    break;
  case GRN_LOG_WARNING :
    type = EVENTLOG_WARNING_TYPE;
    break;
  case GRN_LOG_NOTICE :
  case GRN_LOG_INFO :
  case GRN_LOG_DEBUG :
  case GRN_LOG_DUMP :
    type = EVENTLOG_INFORMATION_TYPE;
    break;
  default :
    type = EVENTLOG_INFORMATION_TYPE;
    break;
  }

  {
    const char level_marks[] = " EACewnid-";
    grn_obj formatted_buffer;
    UINT code_page;
    int n_converted_chars;
#define CONVERTED_BUFFER_SIZE 512

    GRN_TEXT_INIT(&formatted_buffer, 0);
    if (location && location[0]) {
      grn_text_printf(ctx, &formatted_buffer, "%s|%c|%s %s %s",
                      timestamp, level_marks[level], title, message, location);
    } else {
      grn_text_printf(ctx, &formatted_buffer, "%s|%c|%s %s",
                      timestamp, level_marks[level], title, message);
    }

    switch (ctx->encoding) {
    case GRN_ENC_EUC_JP :
      code_page = 20932;
      break;
    case GRN_ENC_UTF8 :
      code_page = CP_UTF8;
      break;
    case GRN_ENC_SJIS :
      code_page = 932;
      break;
    case GRN_ENC_LATIN1 :
      code_page = 1252;
      break;
    case GRN_ENC_KOI8R :
      code_page = 20866;
      break;
    default :
      code_page = CP_ACP;
      break;
    }

    n_converted_chars = MultiByteToWideChar(code_page,
                                            MB_PRECOMPOSED,
                                            GRN_TEXT_VALUE(&formatted_buffer),
                                            GRN_TEXT_LEN(&formatted_buffer),
                                            NULL,
                                            0);
    if (n_converted_chars < CONVERTED_BUFFER_SIZE) {
      WCHAR converted_buffer[CONVERTED_BUFFER_SIZE];
      const WCHAR *strings[1];
      n_converted_chars = MultiByteToWideChar(code_page,
                                              MB_PRECOMPOSED,
                                              GRN_TEXT_VALUE(&formatted_buffer),
                                              GRN_TEXT_LEN(&formatted_buffer),
                                              converted_buffer,
                                              CONVERTED_BUFFER_SIZE);
      converted_buffer[n_converted_chars] = L'\0';
      strings[0] = converted_buffer;
      ReportEventW(data->event_log,
                   type,
                   category,
                   event_id,
                   user_sid,
                   n_strings,
                   data_size,
                   strings,
                   raw_data);
    } else {
      WCHAR *converted;
      const WCHAR *strings[1];
      converted = GRN_MALLOCN(WCHAR, n_converted_chars);
      n_converted_chars = MultiByteToWideChar(code_page,
                                              MB_PRECOMPOSED,
                                              GRN_TEXT_VALUE(&formatted_buffer),
                                              GRN_TEXT_LEN(&formatted_buffer),
                                              converted,
                                              n_converted_chars);
      converted[n_converted_chars] = L'\0';
      strings[0] = converted;
      ReportEventW(data->event_log,
                   type,
                   category,
                   event_id,
                   user_sid,
                   n_strings,
                   data_size,
                   strings,
                   raw_data);
      GRN_FREE(converted);
    }
    GRN_OBJ_FIN(ctx, &formatted_buffer);
  }
}

static void
windows_event_logger_reopen(grn_ctx *ctx, void *user_data)
{
}

static void
windows_event_logger_fin(grn_ctx *ctx, void *user_data)
{
  grn_windows_event_logger_data *data = user_data;

  if (data->event_log) {
    DeregisterEventSource(data->event_log);
  }
  GRN_FREE(data);
}
#endif /* WIN32 */

grn_rc
grn_windows_event_logger_set(grn_ctx *ctx)
{
#ifdef WIN32
  grn_rc rc;
  grn_logger windows_event_logger;
  grn_windows_event_logger_data *data;
  const char *source_name;
  GRN_API_ENTER;

  data = GRN_MALLOC(sizeof(grn_windows_event_logger_data));
  if (!data) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "failed to allocate user data for Windows event logger");
    GRN_API_RETURN(ctx->rc);
  }

  if (windows_event_source_name) {
    source_name = windows_event_source_name;
  } else {
    source_name = "Groonga";
  }
  data->event_log = RegisterEventSourceA(NULL, source_name);
  if (!data->event_log) {
    GRN_FREE(data);
    SERR("RegisterEventSource");
    GRN_LOG(ctx, GRN_LOG_ERROR,
            "failed to register event source: <%s>",
            source_name);
    GRN_API_RETURN(ctx->rc);
  }

  windows_event_logger.max_level = GRN_LOG_DEFAULT_LEVEL;
  windows_event_logger.flags     = GRN_LOG_TIME | GRN_LOG_MESSAGE;
  windows_event_logger.user_data = data;
  windows_event_logger.log       = windows_event_logger_log;
  windows_event_logger.reopen    = windows_event_logger_reopen;
  windows_event_logger.fin       = windows_event_logger_fin;

  rc = grn_logger_set(ctx, &windows_event_logger);
  if (rc != GRN_SUCCESS) {
    windows_event_logger.fin(ctx, windows_event_logger.user_data);
  }

  GRN_API_RETURN(rc);
#else /* WIN32 */
  return GRN_FUNCTION_NOT_IMPLEMENTED;
#endif /* WIN32 */
}
