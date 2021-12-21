/*
  Copyright(C) 2010-2017  Brazil
  Copyright(C) 2020-2021  Sutou Kouhei <kou@clear-code.com>

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

#include "grn.h"
#include "grn_encoding.h"
#include "grn_windows.h"

#ifdef WIN32
# include <dbghelp.h>

static grn_critical_section grn_windows_symbol_critical_section;

static DWORD grn_windows_trace_logging = TLS_OUT_OF_INDEXES;
/* Address is important. Value is meaningless. */
static bool grn_windows_trace_logging_true;
static bool grn_windows_trace_logging_false;

void
grn_windows_init(void)
{
  CRITICAL_SECTION_INIT(grn_windows_symbol_critical_section);

  grn_windows_trace_logging = TlsAlloc();
}

void
grn_windows_fin(void)
{
  CRITICAL_SECTION_FIN(grn_windows_symbol_critical_section);

  if (grn_windows_trace_logging != TLS_OUT_OF_INDEXES) {
    TlsFree(grn_windows_trace_logging);
    grn_windows_trace_logging = TLS_OUT_OF_INDEXES;
  }
}

static bool
grn_windows_trace_logging_get(void)
{
  if (grn_windows_trace_logging == TLS_OUT_OF_INDEXES) {
    return false;
  }

  return TlsGetValue(grn_windows_trace_logging) ==
    &grn_windows_trace_logging_true;
}

static void
grn_windows_trace_logging_set(bool logging)
{
  if (grn_windows_trace_logging == TLS_OUT_OF_INDEXES) {
    return;
  }

  if (logging) {
    TlsSetValue(grn_windows_trace_logging,
                &grn_windows_trace_logging_true);
  } else {
    TlsSetValue(grn_windows_trace_logging,
                &grn_windows_trace_logging_false);
  }
}

static char *windows_base_dir = NULL;
const char *
grn_windows_base_dir(void)
{
  if (!windows_base_dir) {
    HMODULE dll;
    const wchar_t *dll_filename = GRN_DLL_FILENAME;
    wchar_t absolute_dll_filename[MAX_PATH];
    DWORD absolute_dll_filename_size;
    dll = GetModuleHandleW(dll_filename);
    absolute_dll_filename_size = GetModuleFileNameW(dll,
                                                    absolute_dll_filename,
                                                    MAX_PATH);
    if (absolute_dll_filename_size == 0) {
      windows_base_dir = grn_strdup_raw(".");
    } else {
      DWORD ansi_dll_filename_size;

      {
        DWORD i;

        absolute_dll_filename[absolute_dll_filename_size] = L'\0';

        if (wcsncmp(absolute_dll_filename, L"\\\\?\\", 4) == 0) {
          wmemmove_s(absolute_dll_filename,
                     MAX_PATH,
                     &absolute_dll_filename[4],
                     absolute_dll_filename_size - 4);
          absolute_dll_filename[absolute_dll_filename_size - 4] = L'\0';
          absolute_dll_filename_size -= 4;
        }

        for (i = 0; i < absolute_dll_filename_size; i++) {
          if (absolute_dll_filename[i] == L'\\') {
            absolute_dll_filename[i] = L'/';
          }
        }

        /* Remove basename (DLL filename). */
        for (i = absolute_dll_filename_size - 1; i > 0; i--) {
          if (absolute_dll_filename[i] == L'/') {
            absolute_dll_filename[i] = L'\0';
            absolute_dll_filename_size = i;
            break;
          }
        }

        if ((absolute_dll_filename_size >= 4) &&
            ((_wcsicmp(absolute_dll_filename + absolute_dll_filename_size - 4,
                       L"/bin") == 0) ||
             (_wcsicmp(absolute_dll_filename + absolute_dll_filename_size - 4,
                       L"/lib") == 0))) {
          absolute_dll_filename_size -= 4;
          absolute_dll_filename[absolute_dll_filename_size] = L'\0';
        }
      }

      ansi_dll_filename_size =
        WideCharToMultiByte(CP_ACP, 0,
                            absolute_dll_filename, absolute_dll_filename_size,
                            NULL, 0, NULL, NULL);
      if (ansi_dll_filename_size == 0) {
        windows_base_dir = grn_strdup_raw(".");
      } else {
        windows_base_dir = malloc(ansi_dll_filename_size + 1);
        WideCharToMultiByte(CP_ACP, 0,
                            absolute_dll_filename, absolute_dll_filename_size,
                            windows_base_dir, ansi_dll_filename_size,
                            NULL, NULL);
        windows_base_dir[ansi_dll_filename_size] = '\0';
      }
    }
  }
  return windows_base_dir;
}

UINT
grn_windows_encoding_to_code_page(grn_encoding encoding)
{
  UINT code_page;

  switch (encoding) {
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

  return code_page;
}

static void
grn_windows_append_path(grn_ctx *ctx,
                        grn_obj *buffer,
                        const char *path)
{
  for (; *path; path++) {
    if (*path == '/') {
      GRN_TEXT_PUTC(ctx, buffer, '\\');
    } else {
      GRN_TEXT_PUTC(ctx, buffer, *path);
    }
  }
}

static void
grn_windows_symbol_add_search_path(grn_ctx *ctx,
                                   grn_obj *search_path,
                                   const char *relative_path)
{
  if (GRN_TEXT_LEN(search_path) > 0) {
    GRN_TEXT_PUTC(ctx, search_path, ';');
  }

  grn_windows_append_path(ctx, search_path, grn_windows_base_dir());
  grn_windows_append_path(ctx, search_path, "/");
  grn_windows_append_path(ctx, search_path, relative_path);
}

static void
grn_windows_symbol_add_search_paths(grn_ctx *ctx,
                                    grn_obj *search_path,
                                    const char *relative_path)
{
  grn_obj base_dir;
  GRN_TEXT_INIT(&base_dir, 0);
  grn_windows_append_path(ctx, &base_dir, grn_windows_base_dir());
  grn_windows_append_path(ctx, &base_dir, "/");
  grn_windows_append_path(ctx, &base_dir, relative_path);
  grn_windows_append_path(ctx, &base_dir, "/*");
  GRN_TEXT_PUTC(ctx, &base_dir, '\0');

  WIN32_FIND_DATA data;
  HANDLE finder = FindFirstFile(GRN_TEXT_VALUE(&base_dir), &data);
  if (finder != INVALID_HANDLE_VALUE) {
    do {
      if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        continue;
      }
      if (strcmp(data.cFileName, ".") == 0) {
        continue;
      }
      if (strcmp(data.cFileName, "..") == 0) {
        continue;
      }

      grn_obj sub_path;
      GRN_TEXT_INIT(&sub_path, 0);
      grn_windows_append_path(ctx, &sub_path, relative_path);
      grn_windows_append_path(ctx, &sub_path, "/");
      grn_windows_append_path(ctx, &sub_path, data.cFileName);
      GRN_TEXT_PUTC(ctx, &sub_path, '\0');
      grn_windows_symbol_add_search_path(ctx,
                                         search_path,
                                         GRN_TEXT_VALUE(&sub_path));
      grn_windows_symbol_add_search_paths(ctx,
                                          search_path,
                                          GRN_TEXT_VALUE(&sub_path));
      GRN_OBJ_FIN(ctx, &sub_path);
    } while (FindNextFile(finder, &data) != 0);
    FindClose(finder);
  }
  GRN_OBJ_FIN(ctx, &base_dir);
}

bool
grn_windows_symbol_initialize(grn_ctx *ctx, HANDLE process)
{
  CRITICAL_SECTION_ENTER(grn_windows_symbol_critical_section);
  SymSetOptions(SYMOPT_ALLOW_ABSOLUTE_SYMBOLS |
                SYMOPT_ALLOW_ZERO_ADDRESS |
                SYMOPT_AUTO_PUBLICS |
                SYMOPT_DEBUG |
                SYMOPT_DEFERRED_LOADS |
                SYMOPT_LOAD_LINES |
                SYMOPT_NO_PROMPTS);
  if (!SymInitialize(process, NULL, TRUE)) {
    CRITICAL_SECTION_LEAVE(grn_windows_symbol_critical_section);
    return false;
  }

  char current_search_path[MAX_PATH];
  grn_obj new_search_path;
  GRN_TEXT_INIT(&new_search_path, 0);
  if (SymGetSearchPath(process,
                       current_search_path,
                       sizeof(current_search_path))) {
    GRN_TEXT_PUTS(ctx, &new_search_path, current_search_path);
  }

  grn_windows_symbol_add_search_path(ctx, &new_search_path, "bin");
  grn_windows_symbol_add_search_paths(ctx,
                                      &new_search_path,
                                      "lib/" PACKAGE "/plugins");
  GRN_TEXT_PUTC(ctx, &new_search_path, '\0');

  const BOOL success =
    SymSetSearchPath(process, GRN_TEXT_VALUE(&new_search_path));

  GRN_OBJ_FIN(ctx, &new_search_path);

  if (!success) {
    SymCleanup(process);
    CRITICAL_SECTION_LEAVE(grn_windows_symbol_critical_section);
  }

  return success;
}

bool
grn_windows_symbol_cleanup(grn_ctx *ctx, HANDLE process)
{
  const BOOL success = SymCleanup(process);
  CRITICAL_SECTION_LEAVE(grn_windows_symbol_critical_section);
  return success;
}

void
grn_windows_log_back_trace_entry(grn_ctx *ctx,
                                 grn_log_level level,
                                 HANDLE process,
                                 DWORD64 address)
{
  IMAGEHLP_MODULE64 module;
  char *buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
  SYMBOL_INFO *symbol = (SYMBOL_INFO *)buffer;
  DWORD line_displacement = 0;
  IMAGEHLP_LINE64 line;
  bool have_module_name = false;
  bool have_symbol_name = false;
  bool have_location = false;

  module.SizeOfStruct = sizeof(IMAGEHLP_MODULE64);
  if (SymGetModuleInfo64(process, address, &module)) {
    have_module_name = true;
  }

  symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
  symbol->MaxNameLen = MAX_SYM_NAME;
  if (SymFromAddr(process, address, NULL, symbol)) {
    have_symbol_name = true;
  }

  line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
  if (SymGetLineFromAddr64(process, address, &line_displacement, &line)) {
    have_location = true;
  }

  {
    const char *unknown = "(unknown)";
    const char *grn_encoding_image_name = unknown;
    if (have_module_name) {
      grn_encoding_image_name =
        grn_encoding_convert_from_locale(ctx, module.ImageName, -1, NULL);
    }
    GRN_LOG(ctx, level,
            "%s:%lu:%lu: %.*s(): <%s>: <%s>",
            (have_location ? line.FileName : unknown),
            (have_location ? line.LineNumber : 0),
            (have_location ? line_displacement : 0),
            (int)(have_symbol_name ? symbol->NameLen : strlen(unknown)),
            (have_symbol_name ? symbol->Name : unknown),
            (have_module_name ? module.ModuleName : unknown),
            grn_encoding_image_name);
    if (have_module_name) {
      grn_encoding_converted_free(ctx, grn_encoding_image_name);
    }
  }
}

void
grn_windows_log_back_trace(grn_ctx *ctx,
                           grn_log_level level,
                           HANDLE process,
                           HANDLE thread,
                           CONTEXT *context)
{
  if (grn_windows_trace_logging_get()) {
    GRN_LOG(ctx, level, "recursive trace logging isn't supported");
    return;
  }

  grn_windows_trace_logging_set(true);

  if (!grn_windows_symbol_initialize(ctx, process)) {
    GRN_LOG(ctx, level, "failed to initialize symbol handler");
    grn_windows_trace_logging_set(false);
    return;
  }

  STACKFRAME64 frame;
  DWORD machine_type;
  memset(&frame, 0, sizeof(STACKFRAME64));
  frame.AddrPC.Mode = AddrModeFlat;
  frame.AddrReturn.Mode = AddrModeFlat;
  frame.AddrFrame.Mode = AddrModeFlat;
  frame.AddrStack.Mode = AddrModeFlat;
  frame.AddrBStore.Mode = AddrModeFlat;
# ifdef _M_IX86
  machine_type = IMAGE_FILE_MACHINE_I386;
  frame.AddrPC.Offset = context->Eip;
  frame.AddrFrame.Offset = context->Ebp;
  frame.AddrStack.Offset = context->Esp;
# elif defined(_M_IA64) /* _M_IX86 */
  machine_type = IMAGE_FILE_MACHINE_IA64;
  frame.AddrPC.Offset = context->StIIP;
  frame.AddrStack.Offset = context->IntSP; /* SP is IntSP? */
  frame.AddrBStore.Offset = context->RsBSP;
# elif defined(_M_AMD64) /* _M_IX86 */
  machine_type = IMAGE_FILE_MACHINE_AMD64;
  frame.AddrPC.Offset = context->Rip;
  frame.AddrFrame.Offset = context->Rbp;
  frame.AddrStack.Offset = context->Rsp;
# else /* _M_IX86 */
#  error "Intel x86, Intel Itanium and x64 are only supported architectures"
# endif /* _M_IX86 */

  DWORD64 previous_address = 0;
  while (true) {
    if (!StackWalk64(machine_type,
                     process,
                     thread,
                     &frame,
                     context,
                     NULL,
                     NULL,
                     NULL,
                     NULL)) {
      break;
    }

    DWORD64 address = frame.AddrPC.Offset;
    if (previous_address != 0 && address == previous_address) {
      break;
    }

    grn_windows_log_back_trace_entry(ctx, level, process, address);
    previous_address = address;
  }

  grn_windows_symbol_cleanup(ctx, process);

  grn_windows_trace_logging_set(false);
}
#endif /* WIN32 */
