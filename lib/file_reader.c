/*
  Copyright(C) 2015 Brazil
  Copyright(C) 2019 Sutou Kouhei <kou@clear-code.com>

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
#include "grn_str.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#ifdef WIN32
# include <share.h>
#else
# include <sys/select.h>
#endif /* WIN32 */

struct _grn_file_reader {
  FILE *file;
  grn_bool file_need_close;
};

grn_file_reader *
grn_file_reader_open(grn_ctx *ctx, const char *path)
{
  grn_file_reader *reader;
  FILE *file;
  grn_bool file_need_close;

  GRN_API_ENTER;

  if (!path) {
    ERR(GRN_INVALID_ARGUMENT, "[file-reader][open] path must not NULL");
    GRN_API_RETURN(NULL);
  }

  if (strcmp(path, "-") == 0) {
    file = stdin;
    file_need_close = GRN_FALSE;
  } else {
    file = grn_fopen(path, "r");
    if (!file) {
      SERR("[file-reader][open] failed to open path: <%s>", path);
      GRN_API_RETURN(NULL);
    }
    file_need_close = GRN_TRUE;
  }

  reader = GRN_CALLOC(sizeof(grn_file_reader));
  if (!reader) {
    if (file_need_close) {
      fclose(file);
    }
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[file-reader][open] failed to allocate reader: <%s>", path);
    GRN_API_RETURN(NULL);
  }
  reader->file = file;
  reader->file_need_close = file_need_close;

  GRN_API_RETURN(reader);
}

void
grn_file_reader_close(grn_ctx *ctx, grn_file_reader *reader)
{
  if (!reader) {
    return;
  }

  if (reader->file_need_close) {
    if (fclose(reader->file) != 0) {
      GRN_LOG(ctx, GRN_LOG_ERROR,
              "[file-reader][close] failed to close: <%s>",
              grn_strerror(errno));
    }
  }

  GRN_FREE(reader);
}

grn_rc
grn_file_reader_read_line(grn_ctx *ctx,
                          grn_file_reader *reader,
                          grn_obj *buffer)
{
  grn_rc rc = GRN_END_OF_DATA;

  for (;;) {
    size_t len;

#define BUFFER_SIZE 4096
    grn_rc reserve_rc = grn_bulk_reserve(ctx, buffer, BUFFER_SIZE);
    if (reserve_rc != GRN_SUCCESS) {
      rc = reserve_rc;
      break;
    }
    if (!fgets(GRN_BULK_CURR(buffer), BUFFER_SIZE, reader->file)) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        int fd = fileno(reader->file);
        int n_fds = 1;
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(fd, &read_fds);
        int n_ready_fds = select(n_fds, &read_fds, NULL, NULL, NULL);
        if (n_ready_fds != 1) {
          break;
        }
        continue;
      }
      break;
    }
#undef BUFFER_SIZE

    if (!(len = strlen(GRN_BULK_CURR(buffer)))) { break; }
    GRN_BULK_INCR_LEN(buffer, len);
    rc = GRN_SUCCESS;
    if (GRN_BULK_CURR(buffer)[-1] == '\n') { break; }
  }

  return rc;
}
