/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2016-2018 Brazil

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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define GRN_RAW_STRING_INIT(string) do {        \
    string.value = NULL;                        \
    string.length = 0;                          \
  } while (GRN_FALSE)

#define GRN_RAW_STRING_SET(string, bulk)         \
  if (bulk && GRN_TEXT_LEN(bulk) > 0) {          \
    string.value = GRN_TEXT_VALUE(bulk);         \
    string.length = GRN_TEXT_LEN(bulk);          \
  } else {                                       \
    string.value = NULL;                         \
    string.length = 0;                           \
  }

#define GRN_RAW_STRING_FILL(string, bulk)        \
  if (bulk && GRN_TEXT_LEN(bulk) > 0) {          \
    string.value = GRN_TEXT_VALUE(bulk);         \
    string.length = GRN_TEXT_LEN(bulk);          \
  }

#define GRN_RAW_STRING_EQUAL_CSTRING(string, cstring)           \
  (cstring ?                                                    \
   (string.length == strlen(cstring) &&                         \
    memcmp(string.value, cstring, string.length) == 0) :        \
   (string.length == 0))

#define GRN_RAW_STRING_SPRIT(string, separator, splited_string, n_splited_string)       \
  unsigned int i;                                                                       \
  splited_string->value = strtok(string->value, separator);                             \
  splited_string->length = strlen(splited_string->value);                               \
  splited_string++;                                                                     \
  for (i = 0; i < n_splited_string-1; i++) {                                            \
    splited_string->value = strtok(NULL, separator);                                    \
    splited_string->length = strlen(splited_string->value);                             \
    splited_string++;                                                                   \
  }                                                                                     

typedef struct {
  const char *value;
  size_t length;
} grn_raw_string;

GRN_API void grn_raw_string_lstrip(grn_ctx *ctx, grn_raw_string *string);

#ifdef __cplusplus
}
#endif
