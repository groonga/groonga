/*
  Copyright(C) 2018 Brazil

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

#ifdef WIN32
# define GROONGA_MAIN
#endif /* WIN32 */

#include "grn.h"
#include "grn_nfkc.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef GRN_WITH_NFKC

#define MAX_UNICODE 0x110000

static inline int
ucs2utf8(unsigned int i, unsigned char *buf)
{
  unsigned char *p = buf;
  if (i < 0x80) {
    *p++ = i;
  } else {
    if (i < 0x800) {
      *p++ = (i >> 6) | 0xc0;
    } else {
      if (i < 0x00010000) {
        *p++ = (i >> 12) | 0xe0;
      } else {
        if (i < 0x00200000) {
          *p++ = (i >> 18) | 0xf0;
        } else {
          if (i < 0x04000000) {
            *p++ = (i >> 24) | 0xf8;
          } else if (i < 0x80000000) {
            *p++ = (i >> 30) | 0xfc;
            *p++ = ((i >> 24) & 0x3f) | 0x80;
          }
          *p++ = ((i >> 18) & 0x3f) | 0x80;
        }
        *p++ = ((i >> 12) & 0x3f) | 0x80;
      }
      *p++ = ((i >> 6) & 0x3f) | 0x80;
    }
    *p++ = (0x3f & i) | 0x80;
  }
  *p = '\0';
  return (p - buf);
}

#endif /* GRN_WITH_NFKC */

int
main(int argc, char **argv)
{
  int unicode_version = 5;

  if (argc == 2) {
    if (strcmp(argv[1], "5") == 0) {
      unicode_version = 5;
    } else if (strcmp(argv[1], "10") == 0) {
      unicode_version = 10;
    } else {
      return EXIT_FAILURE;
    }
  }

#ifdef GRN_WITH_NFKC
  if (unicode_version == 5) {
    uint64_t code_point;
    char utf8[7];

    for (code_point = 1; code_point < MAX_UNICODE; code_point++) {
      ucs2utf8(code_point, (unsigned char *)utf8);
      printf("%#0" GRN_FMT_INT64X ": %s: <%s>\n",
             code_point,
             grn_char_type_to_string(grn_nfkc50_char_type(utf8)),
             utf8);
    }
  } else if (unicode_version == 10) {
    uint64_t code_point;
    char utf8[7];

    for (code_point = 1; code_point < MAX_UNICODE; code_point++) {
      ucs2utf8(code_point, (unsigned char *)utf8);
      printf("%#0" GRN_FMT_INT64X ": %s: <%s>\n",
             code_point,
             grn_char_type_to_string(grn_nfkc100_char_type(utf8)),
             utf8);
    }
  }
#endif

  return EXIT_SUCCESS;
}
