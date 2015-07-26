/* -*- c-basic-offset: 2 -*- */
/* Copyright(C) 2013 Brazil

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

#include "grn_error.h"
#include "grn_util.h"

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif /* HAVE_ERRNO_H */

#include <string.h>

#ifdef WIN32

grn_rc
grn_windows_error_code_to_rc(int error_code)
{
  grn_rc rc;

  switch (error_code) {
  case ERROR_FILE_NOT_FOUND :
  case ERROR_PATH_NOT_FOUND :
    rc = GRN_NO_SUCH_FILE_OR_DIRECTORY;
    break;
  case ERROR_TOO_MANY_OPEN_FILES :
    rc = GRN_TOO_MANY_OPEN_FILES;
    break;
  case ERROR_ACCESS_DENIED :
    rc = GRN_PERMISSION_DENIED;
    break;
  case ERROR_INVALID_HANDLE :
    rc = GRN_INVALID_ARGUMENT;
    break;
  case ERROR_ARENA_TRASHED :
    rc = GRN_ADDRESS_IS_NOT_AVAILABLE;
    break;
  case ERROR_NOT_ENOUGH_MEMORY :
    rc = GRN_NO_MEMORY_AVAILABLE;
    break;
  case ERROR_INVALID_BLOCK :
  case ERROR_BAD_ENVIRONMENT :
    rc = GRN_INVALID_ARGUMENT;
    break;
  case ERROR_BAD_FORMAT :
    rc = GRN_INVALID_FORMAT;
    break;
  case ERROR_INVALID_DATA :
    rc = GRN_INVALID_ARGUMENT;
    break;
  case ERROR_OUTOFMEMORY :
    rc = GRN_NO_MEMORY_AVAILABLE;
    break;
  case ERROR_INVALID_DRIVE :
    rc = GRN_INVALID_ARGUMENT;
    break;
  case ERROR_WRITE_PROTECT :
    rc = GRN_PERMISSION_DENIED;
    break;
  case ERROR_BAD_LENGTH :
    rc = GRN_INVALID_ARGUMENT;
    break;
  case ERROR_SEEK :
    rc = GRN_INVALID_SEEK;
    break;
  case ERROR_NOT_SUPPORTED :
    rc = GRN_OPERATION_NOT_SUPPORTED;
    break;
  case ERROR_NETWORK_ACCESS_DENIED :
    rc = GRN_OPERATION_NOT_PERMITTED;
    break;
  case ERROR_FILE_EXISTS :
    rc = GRN_FILE_EXISTS;
    break;
  case ERROR_INVALID_PARAMETER :
    rc = GRN_INVALID_ARGUMENT;
    break;
  case ERROR_BROKEN_PIPE :
    rc = GRN_BROKEN_PIPE;
    break;
  case ERROR_CALL_NOT_IMPLEMENTED :
    rc = GRN_FUNCTION_NOT_IMPLEMENTED;
    break;
  case ERROR_INVALID_NAME :
    rc = GRN_INVALID_ARGUMENT;
    break;
  case ERROR_BUSY_DRIVE :
  case ERROR_PATH_BUSY :
    rc = GRN_RESOURCE_BUSY;
    break;
  case ERROR_BAD_ARGUMENTS :
    rc = GRN_INVALID_ARGUMENT;
    break;
  case ERROR_BUSY :
    rc = GRN_RESOURCE_BUSY;
    break;
  case ERROR_ALREADY_EXISTS :
    rc = GRN_FILE_EXISTS;
    break;
  case ERROR_BAD_EXE_FORMAT :
    rc = GRN_EXEC_FORMAT_ERROR;
    break;
  default:
    rc = GRN_UNKNOWN_ERROR;
    break;
  }

  return rc;
}

# define LANG_ID_NEUTRAL()        MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL)
# define LANG_ID_USER_DEFAULT()   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)
# define LANG_ID_SYSTEM_DEFAULT() MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT)

const char *
grn_current_error_message(void)
{
# define ERROR_MESSAGE_BUFFER_SIZE 4096
  int error_code = GetLastError();
  static WCHAR utf16_message[ERROR_MESSAGE_BUFFER_SIZE];
  DWORD written_utf16_chars;
  static char message[ERROR_MESSAGE_BUFFER_SIZE];

  written_utf16_chars = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM |
                                       FORMAT_MESSAGE_IGNORE_INSERTS,
                                       NULL,
                                       error_code,
                                       LANG_ID_USER_DEFAULT(),
                                       utf16_message,
                                       ERROR_MESSAGE_BUFFER_SIZE,
                                       NULL);
  if (written_utf16_chars >= 2) {
    if (utf16_message[written_utf16_chars - 1] == L'\n') {
      utf16_message[written_utf16_chars - 1] = L'\0';
      written_utf16_chars--;
    }
    if (utf16_message[written_utf16_chars - 1] == L'\r') {
      utf16_message[written_utf16_chars - 1] = L'\0';
      written_utf16_chars--;
    }
  }

  {
    UINT code_page;
    DWORD convert_flags = 0;
    int written_bytes;

    code_page = grn_windows_encoding_to_code_page(grn_get_default_encoding());
    written_bytes = WideCharToMultiByte(code_page,
                                        convert_flags,
                                        utf16_message,
                                        written_utf16_chars,
                                        message,
                                        ERROR_MESSAGE_BUFFER_SIZE,
                                        NULL,
                                        NULL);
  }

  return message;

# undef ERROR_MESSAGE_BUFFER_SIZE
}
#else
const char *
grn_current_error_message(void)
{
  return strerror(errno);
}
#endif

const char *
grn_strerror(int error_code)
{
#ifdef WIN32
# define MESSAGE_BUFFER_SIZE 1024
  static char message[MESSAGE_BUFFER_SIZE];
  strerror_s(message, MESSAGE_BUFFER_SIZE, error_code);
  return message;
# undef MESSAGE_BUFFER_SIZE
#else /* WIN32 */
  return strerror(error_code);
#endif /* WIN32 */
}
