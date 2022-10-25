/*
  Copyright(C) 2022  Sutou Kouhei <kou@clear-code.com>

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

/*
  Don't edit this file by hand.
  This is generated automatically by normalizer_html_expand_named_char_ref.rb.
*/

#include "grn_str.h"
#include <string.h>

/* Mapping table:
   https://html.spec.whatwg.org/multipage/named-characters.html#named-character-references */
static bool
html_expand_named_char_ref(grn_ctx *ctx,
                           grn_obj *buffer,
                           const char *name,
                           size_t length)
{
  switch (length) {
  case 2 :
    switch (name[0]) {
    case 'D' :
      if (strncmp(name + 1, "D", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8517);
        return true;
      }
      break;
    case 'G' :
      if (strncmp(name + 1, "T", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 62);
        return true;
      }
      if (strncmp(name + 1, "g", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8921);
        return true;
      }
      if (strncmp(name + 1, "t", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8811);
        return true;
      }
      break;
    case 'I' :
      if (strncmp(name + 1, "m", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8465);
        return true;
      }
      break;
    case 'L' :
      if (strncmp(name + 1, "T", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 60);
        return true;
      }
      if (strncmp(name + 1, "l", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8920);
        return true;
      }
      if (strncmp(name + 1, "t", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8810);
        return true;
      }
      break;
    case 'M' :
      if (strncmp(name + 1, "u", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 924);
        return true;
      }
      break;
    case 'N' :
      if (strncmp(name + 1, "u", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 925);
        return true;
      }
      break;
    case 'O' :
      if (strncmp(name + 1, "r", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10836);
        return true;
      }
      break;
    case 'P' :
      if (strncmp(name + 1, "i", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 928);
        return true;
      }
      if (strncmp(name + 1, "r", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10939);
        return true;
      }
      break;
    case 'R' :
      if (strncmp(name + 1, "e", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8476);
        return true;
      }
      break;
    case 'S' :
      if (strncmp(name + 1, "c", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10940);
        return true;
      }
      break;
    case 'X' :
      if (strncmp(name + 1, "i", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 926);
        return true;
      }
      break;
    case 'a' :
      if (strncmp(name + 1, "c", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8766);
        return true;
      }
      if (strncmp(name + 1, "f", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8289);
        return true;
      }
      if (strncmp(name + 1, "p", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8776);
        return true;
      }
      break;
    case 'd' :
      if (strncmp(name + 1, "d", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8518);
        return true;
      }
      break;
    case 'e' :
      if (strncmp(name + 1, "e", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8519);
        return true;
      }
      if (strncmp(name + 1, "g", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10906);
        return true;
      }
      if (strncmp(name + 1, "l", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10905);
        return true;
      }
      break;
    case 'g' :
      if (strncmp(name + 1, "E", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8807);
        return true;
      }
      if (strncmp(name + 1, "e", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8805);
        return true;
      }
      if (strncmp(name + 1, "g", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8811);
        return true;
      }
      if (strncmp(name + 1, "l", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8823);
        return true;
      }
      if (strncmp(name + 1, "t", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 62);
        return true;
      }
      break;
    case 'i' :
      if (strncmp(name + 1, "c", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8291);
        return true;
      }
      if (strncmp(name + 1, "i", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8520);
        return true;
      }
      if (strncmp(name + 1, "n", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8712);
        return true;
      }
      if (strncmp(name + 1, "t", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8290);
        return true;
      }
      break;
    case 'l' :
      if (strncmp(name + 1, "E", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8806);
        return true;
      }
      if (strncmp(name + 1, "e", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8804);
        return true;
      }
      if (strncmp(name + 1, "g", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8822);
        return true;
      }
      if (strncmp(name + 1, "l", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8810);
        return true;
      }
      if (strncmp(name + 1, "t", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 60);
        return true;
      }
      break;
    case 'm' :
      if (strncmp(name + 1, "p", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8723);
        return true;
      }
      if (strncmp(name + 1, "u", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 956);
        return true;
      }
      break;
    case 'n' :
      if (strncmp(name + 1, "e", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8800);
        return true;
      }
      if (strncmp(name + 1, "i", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8715);
        return true;
      }
      if (strncmp(name + 1, "u", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 957);
        return true;
      }
      break;
    case 'o' :
      if (strncmp(name + 1, "S", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9416);
        return true;
      }
      if (strncmp(name + 1, "r", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8744);
        return true;
      }
      break;
    case 'p' :
      if (strncmp(name + 1, "i", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 960);
        return true;
      }
      if (strncmp(name + 1, "m", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 177);
        return true;
      }
      if (strncmp(name + 1, "r", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8826);
        return true;
      }
      break;
    case 'r' :
      if (strncmp(name + 1, "x", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8478);
        return true;
      }
      break;
    case 's' :
      if (strncmp(name + 1, "c", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8827);
        return true;
      }
      break;
    case 'w' :
      if (strncmp(name + 1, "p", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8472);
        return true;
      }
      if (strncmp(name + 1, "r", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8768);
        return true;
      }
      break;
    case 'x' :
      if (strncmp(name + 1, "i", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 958);
        return true;
      }
      break;
    default :
      break;
    }
    break;
  case 3 :
    switch (name[0]) {
    case 'A' :
      if (strncmp(name + 1, "MP", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 38);
        return true;
      }
      if (strncmp(name + 1, "cy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1040);
        return true;
      }
      if (strncmp(name + 1, "fr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120068);
        return true;
      }
      if (strncmp(name + 1, "nd", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10835);
        return true;
      }
      break;
    case 'B' :
      if (strncmp(name + 1, "cy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1041);
        return true;
      }
      if (strncmp(name + 1, "fr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120069);
        return true;
      }
      break;
    case 'C' :
      if (strncmp(name + 1, "ap", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8914);
        return true;
      }
      if (strncmp(name + 1, "fr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8493);
        return true;
      }
      if (strncmp(name + 1, "hi", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 935);
        return true;
      }
      if (strncmp(name + 1, "up", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8915);
        return true;
      }
      break;
    case 'D' :
      if (strncmp(name + 1, "cy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1044);
        return true;
      }
      if (strncmp(name + 1, "el", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8711);
        return true;
      }
      if (strncmp(name + 1, "fr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120071);
        return true;
      }
      if (strncmp(name + 1, "ot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 168);
        return true;
      }
      break;
    case 'E' :
      if (strncmp(name + 1, "NG", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 330);
        return true;
      }
      if (strncmp(name + 1, "TH", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 208);
        return true;
      }
      if (strncmp(name + 1, "cy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1069);
        return true;
      }
      if (strncmp(name + 1, "fr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120072);
        return true;
      }
      if (strncmp(name + 1, "ta", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 919);
        return true;
      }
      break;
    case 'F' :
      if (strncmp(name + 1, "cy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1060);
        return true;
      }
      if (strncmp(name + 1, "fr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120073);
        return true;
      }
      break;
    case 'G' :
      if (strncmp(name + 1, "cy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1043);
        return true;
      }
      if (strncmp(name + 1, "fr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120074);
        return true;
      }
      break;
    case 'H' :
      if (strncmp(name + 1, "at", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 94);
        return true;
      }
      if (strncmp(name + 1, "fr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8460);
        return true;
      }
      break;
    case 'I' :
      if (strncmp(name + 1, "cy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1048);
        return true;
      }
      if (strncmp(name + 1, "fr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8465);
        return true;
      }
      if (strncmp(name + 1, "nt", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8748);
        return true;
      }
      break;
    case 'J' :
      if (strncmp(name + 1, "cy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1049);
        return true;
      }
      if (strncmp(name + 1, "fr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120077);
        return true;
      }
      break;
    case 'K' :
      if (strncmp(name + 1, "cy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1050);
        return true;
      }
      if (strncmp(name + 1, "fr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120078);
        return true;
      }
      break;
    case 'L' :
      if (strncmp(name + 1, "cy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1051);
        return true;
      }
      if (strncmp(name + 1, "fr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120079);
        return true;
      }
      if (strncmp(name + 1, "sh", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8624);
        return true;
      }
      break;
    case 'M' :
      if (strncmp(name + 1, "ap", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10501);
        return true;
      }
      if (strncmp(name + 1, "cy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1052);
        return true;
      }
      if (strncmp(name + 1, "fr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120080);
        return true;
      }
      break;
    case 'N' :
      if (strncmp(name + 1, "cy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1053);
        return true;
      }
      if (strncmp(name + 1, "fr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120081);
        return true;
      }
      if (strncmp(name + 1, "ot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10988);
        return true;
      }
      break;
    case 'O' :
      if (strncmp(name + 1, "cy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1054);
        return true;
      }
      if (strncmp(name + 1, "fr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120082);
        return true;
      }
      break;
    case 'P' :
      if (strncmp(name + 1, "cy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1055);
        return true;
      }
      if (strncmp(name + 1, "fr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120083);
        return true;
      }
      if (strncmp(name + 1, "hi", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 934);
        return true;
      }
      if (strncmp(name + 1, "si", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 936);
        return true;
      }
      break;
    case 'Q' :
      if (strncmp(name + 1, "fr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120084);
        return true;
      }
      break;
    case 'R' :
      if (strncmp(name + 1, "EG", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 174);
        return true;
      }
      if (strncmp(name + 1, "cy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1056);
        return true;
      }
      if (strncmp(name + 1, "fr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8476);
        return true;
      }
      if (strncmp(name + 1, "ho", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 929);
        return true;
      }
      if (strncmp(name + 1, "sh", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8625);
        return true;
      }
      break;
    case 'S' :
      if (strncmp(name + 1, "cy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1057);
        return true;
      }
      if (strncmp(name + 1, "fr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120086);
        return true;
      }
      if (strncmp(name + 1, "ub", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8912);
        return true;
      }
      if (strncmp(name + 1, "um", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8721);
        return true;
      }
      if (strncmp(name + 1, "up", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8913);
        return true;
      }
      break;
    case 'T' :
      if (strncmp(name + 1, "ab", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9);
        return true;
      }
      if (strncmp(name + 1, "au", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 932);
        return true;
      }
      if (strncmp(name + 1, "cy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1058);
        return true;
      }
      if (strncmp(name + 1, "fr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120087);
        return true;
      }
      break;
    case 'U' :
      if (strncmp(name + 1, "cy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1059);
        return true;
      }
      if (strncmp(name + 1, "fr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120088);
        return true;
      }
      break;
    case 'V' :
      if (strncmp(name + 1, "cy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1042);
        return true;
      }
      if (strncmp(name + 1, "ee", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8897);
        return true;
      }
      if (strncmp(name + 1, "fr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120089);
        return true;
      }
      break;
    case 'W' :
      if (strncmp(name + 1, "fr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120090);
        return true;
      }
      break;
    case 'X' :
      if (strncmp(name + 1, "fr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120091);
        return true;
      }
      break;
    case 'Y' :
      if (strncmp(name + 1, "cy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1067);
        return true;
      }
      if (strncmp(name + 1, "fr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120092);
        return true;
      }
      break;
    case 'Z' :
      if (strncmp(name + 1, "cy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1047);
        return true;
      }
      if (strncmp(name + 1, "fr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8488);
        return true;
      }
      break;
    case 'a' :
      if (strncmp(name + 1, "cE", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8766);
        grn_text_code_point(ctx, buffer, 819);
        return true;
      }
      if (strncmp(name + 1, "cd", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8767);
        return true;
      }
      if (strncmp(name + 1, "cy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1072);
        return true;
      }
      if (strncmp(name + 1, "fr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120094);
        return true;
      }
      if (strncmp(name + 1, "mp", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 38);
        return true;
      }
      if (strncmp(name + 1, "nd", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8743);
        return true;
      }
      if (strncmp(name + 1, "ng", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8736);
        return true;
      }
      if (strncmp(name + 1, "pE", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10864);
        return true;
      }
      if (strncmp(name + 1, "pe", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8778);
        return true;
      }
      if (strncmp(name + 1, "st", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 42);
        return true;
      }
      break;
    case 'b' :
      if (strncmp(name + 1, "cy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1073);
        return true;
      }
      if (strncmp(name + 1, "fr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120095);
        return true;
      }
      if (strncmp(name + 1, "ne", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 61);
        grn_text_code_point(ctx, buffer, 8421);
        return true;
      }
      if (strncmp(name + 1, "ot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8869);
        return true;
      }
      break;
    case 'c' :
      if (strncmp(name + 1, "ap", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8745);
        return true;
      }
      if (strncmp(name + 1, "fr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120096);
        return true;
      }
      if (strncmp(name + 1, "hi", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 967);
        return true;
      }
      if (strncmp(name + 1, "ir", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9675);
        return true;
      }
      if (strncmp(name + 1, "up", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8746);
        return true;
      }
      break;
    case 'd' :
      if (strncmp(name + 1, "cy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1076);
        return true;
      }
      if (strncmp(name + 1, "eg", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 176);
        return true;
      }
      if (strncmp(name + 1, "fr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120097);
        return true;
      }
      if (strncmp(name + 1, "ie", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 168);
        return true;
      }
      if (strncmp(name + 1, "iv", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 247);
        return true;
      }
      if (strncmp(name + 1, "ot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 729);
        return true;
      }
      break;
    case 'e' :
      if (strncmp(name + 1, "cy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1101);
        return true;
      }
      if (strncmp(name + 1, "fr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120098);
        return true;
      }
      if (strncmp(name + 1, "gs", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10902);
        return true;
      }
      if (strncmp(name + 1, "ll", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8467);
        return true;
      }
      if (strncmp(name + 1, "ls", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10901);
        return true;
      }
      if (strncmp(name + 1, "ng", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 331);
        return true;
      }
      if (strncmp(name + 1, "ta", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 951);
        return true;
      }
      if (strncmp(name + 1, "th", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 240);
        return true;
      }
      break;
    case 'f' :
      if (strncmp(name + 1, "cy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1092);
        return true;
      }
      if (strncmp(name + 1, "fr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120099);
        return true;
      }
      break;
    case 'g' :
      if (strncmp(name + 1, "El", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10892);
        return true;
      }
      if (strncmp(name + 1, "ap", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10886);
        return true;
      }
      if (strncmp(name + 1, "cy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1075);
        return true;
      }
      if (strncmp(name + 1, "el", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8923);
        return true;
      }
      if (strncmp(name + 1, "eq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8805);
        return true;
      }
      if (strncmp(name + 1, "es", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10878);
        return true;
      }
      if (strncmp(name + 1, "fr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120100);
        return true;
      }
      if (strncmp(name + 1, "gg", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8921);
        return true;
      }
      if (strncmp(name + 1, "lE", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10898);
        return true;
      }
      if (strncmp(name + 1, "la", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10917);
        return true;
      }
      if (strncmp(name + 1, "lj", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10916);
        return true;
      }
      if (strncmp(name + 1, "nE", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8809);
        return true;
      }
      if (strncmp(name + 1, "ne", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10888);
        return true;
      }
      break;
    case 'h' :
      if (strncmp(name + 1, "fr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120101);
        return true;
      }
      break;
    case 'i' :
      if (strncmp(name + 1, "cy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1080);
        return true;
      }
      if (strncmp(name + 1, "ff", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8660);
        return true;
      }
      if (strncmp(name + 1, "fr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120102);
        return true;
      }
      if (strncmp(name + 1, "nt", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8747);
        return true;
      }
      break;
    case 'j' :
      if (strncmp(name + 1, "cy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1081);
        return true;
      }
      if (strncmp(name + 1, "fr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120103);
        return true;
      }
      break;
    case 'k' :
      if (strncmp(name + 1, "cy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1082);
        return true;
      }
      if (strncmp(name + 1, "fr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120104);
        return true;
      }
      break;
    case 'l' :
      if (strncmp(name + 1, "Eg", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10891);
        return true;
      }
      if (strncmp(name + 1, "ap", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10885);
        return true;
      }
      if (strncmp(name + 1, "at", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10923);
        return true;
      }
      if (strncmp(name + 1, "cy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1083);
        return true;
      }
      if (strncmp(name + 1, "eg", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8922);
        return true;
      }
      if (strncmp(name + 1, "eq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8804);
        return true;
      }
      if (strncmp(name + 1, "es", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10877);
        return true;
      }
      if (strncmp(name + 1, "fr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120105);
        return true;
      }
      if (strncmp(name + 1, "gE", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10897);
        return true;
      }
      if (strncmp(name + 1, "nE", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8808);
        return true;
      }
      if (strncmp(name + 1, "ne", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10887);
        return true;
      }
      if (strncmp(name + 1, "oz", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9674);
        return true;
      }
      if (strncmp(name + 1, "rm", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8206);
        return true;
      }
      if (strncmp(name + 1, "sh", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8624);
        return true;
      }
      break;
    case 'm' :
      if (strncmp(name + 1, "ap", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8614);
        return true;
      }
      if (strncmp(name + 1, "cy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1084);
        return true;
      }
      if (strncmp(name + 1, "fr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120106);
        return true;
      }
      if (strncmp(name + 1, "ho", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8487);
        return true;
      }
      if (strncmp(name + 1, "id", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8739);
        return true;
      }
      break;
    case 'n' :
      if (strncmp(name + 1, "Gg", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8921);
        grn_text_code_point(ctx, buffer, 824);
        return true;
      }
      if (strncmp(name + 1, "Gt", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8811);
        grn_text_code_point(ctx, buffer, 8402);
        return true;
      }
      if (strncmp(name + 1, "Ll", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8920);
        grn_text_code_point(ctx, buffer, 824);
        return true;
      }
      if (strncmp(name + 1, "Lt", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8810);
        grn_text_code_point(ctx, buffer, 8402);
        return true;
      }
      if (strncmp(name + 1, "ap", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8777);
        return true;
      }
      if (strncmp(name + 1, "cy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1085);
        return true;
      }
      if (strncmp(name + 1, "fr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120107);
        return true;
      }
      if (strncmp(name + 1, "gE", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8807);
        grn_text_code_point(ctx, buffer, 824);
        return true;
      }
      if (strncmp(name + 1, "ge", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8817);
        return true;
      }
      if (strncmp(name + 1, "gt", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8815);
        return true;
      }
      if (strncmp(name + 1, "is", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8956);
        return true;
      }
      if (strncmp(name + 1, "iv", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8715);
        return true;
      }
      if (strncmp(name + 1, "lE", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8806);
        grn_text_code_point(ctx, buffer, 824);
        return true;
      }
      if (strncmp(name + 1, "le", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8816);
        return true;
      }
      if (strncmp(name + 1, "lt", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8814);
        return true;
      }
      if (strncmp(name + 1, "ot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 172);
        return true;
      }
      if (strncmp(name + 1, "pr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8832);
        return true;
      }
      if (strncmp(name + 1, "sc", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8833);
        return true;
      }
      if (strncmp(name + 1, "um", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 35);
        return true;
      }
      break;
    case 'o' :
      if (strncmp(name + 1, "cy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1086);
        return true;
      }
      if (strncmp(name + 1, "fr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120108);
        return true;
      }
      if (strncmp(name + 1, "gt", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10689);
        return true;
      }
      if (strncmp(name + 1, "hm", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 937);
        return true;
      }
      if (strncmp(name + 1, "lt", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10688);
        return true;
      }
      if (strncmp(name + 1, "rd", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10845);
        return true;
      }
      if (strncmp(name + 1, "rv", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10843);
        return true;
      }
      break;
    case 'p' :
      if (strncmp(name + 1, "ar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8741);
        return true;
      }
      if (strncmp(name + 1, "cy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1087);
        return true;
      }
      if (strncmp(name + 1, "fr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120109);
        return true;
      }
      if (strncmp(name + 1, "hi", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 966);
        return true;
      }
      if (strncmp(name + 1, "iv", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 982);
        return true;
      }
      if (strncmp(name + 1, "rE", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10931);
        return true;
      }
      if (strncmp(name + 1, "re", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10927);
        return true;
      }
      if (strncmp(name + 1, "si", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 968);
        return true;
      }
      break;
    case 'q' :
      if (strncmp(name + 1, "fr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120110);
        return true;
      }
      break;
    case 'r' :
      if (strncmp(name + 1, "cy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1088);
        return true;
      }
      if (strncmp(name + 1, "eg", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 174);
        return true;
      }
      if (strncmp(name + 1, "fr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120111);
        return true;
      }
      if (strncmp(name + 1, "ho", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 961);
        return true;
      }
      if (strncmp(name + 1, "lm", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8207);
        return true;
      }
      if (strncmp(name + 1, "sh", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8625);
        return true;
      }
      break;
    case 's' :
      if (strncmp(name + 1, "cE", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10932);
        return true;
      }
      if (strncmp(name + 1, "ce", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10928);
        return true;
      }
      if (strncmp(name + 1, "cy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1089);
        return true;
      }
      if (strncmp(name + 1, "fr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120112);
        return true;
      }
      if (strncmp(name + 1, "hy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 173);
        return true;
      }
      if (strncmp(name + 1, "im", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8764);
        return true;
      }
      if (strncmp(name + 1, "mt", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10922);
        return true;
      }
      if (strncmp(name + 1, "ol", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 47);
        return true;
      }
      if (strncmp(name + 1, "qu", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9633);
        return true;
      }
      if (strncmp(name + 1, "ub", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8834);
        return true;
      }
      if (strncmp(name + 1, "um", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8721);
        return true;
      }
      if (strncmp(name + 1, "up", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8835);
        return true;
      }
      break;
    case 't' :
      if (strncmp(name + 1, "au", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 964);
        return true;
      }
      if (strncmp(name + 1, "cy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1090);
        return true;
      }
      if (strncmp(name + 1, "fr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120113);
        return true;
      }
      if (strncmp(name + 1, "op", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8868);
        return true;
      }
      break;
    case 'u' :
      if (strncmp(name + 1, "cy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1091);
        return true;
      }
      if (strncmp(name + 1, "fr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120114);
        return true;
      }
      if (strncmp(name + 1, "ml", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 168);
        return true;
      }
      break;
    case 'v' :
      if (strncmp(name + 1, "cy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1074);
        return true;
      }
      if (strncmp(name + 1, "ee", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8744);
        return true;
      }
      if (strncmp(name + 1, "fr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120115);
        return true;
      }
      break;
    case 'w' :
      if (strncmp(name + 1, "fr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120116);
        return true;
      }
      break;
    case 'x' :
      if (strncmp(name + 1, "fr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120117);
        return true;
      }
      break;
    case 'y' :
      if (strncmp(name + 1, "cy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1099);
        return true;
      }
      if (strncmp(name + 1, "en", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 165);
        return true;
      }
      if (strncmp(name + 1, "fr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120118);
        return true;
      }
      break;
    case 'z' :
      if (strncmp(name + 1, "cy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1079);
        return true;
      }
      if (strncmp(name + 1, "fr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120119);
        return true;
      }
      if (strncmp(name + 1, "wj", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8205);
        return true;
      }
      break;
    default :
      break;
    }
    break;
  case 4 :
    switch (name[0]) {
    case 'A' :
      if (strncmp(name + 1, "opf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120120);
        return true;
      }
      if (strncmp(name + 1, "scr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 119964);
        return true;
      }
      if (strncmp(name + 1, "uml", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 196);
        return true;
      }
      break;
    case 'B' :
      if (strncmp(name + 1, "arv", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10983);
        return true;
      }
      if (strncmp(name + 1, "eta", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 914);
        return true;
      }
      if (strncmp(name + 1, "opf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120121);
        return true;
      }
      if (strncmp(name + 1, "scr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8492);
        return true;
      }
      break;
    case 'C' :
      if (strncmp(name + 1, "Hcy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1063);
        return true;
      }
      if (strncmp(name + 1, "OPY", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 169);
        return true;
      }
      if (strncmp(name + 1, "dot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 266);
        return true;
      }
      if (strncmp(name + 1, "opf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8450);
        return true;
      }
      if (strncmp(name + 1, "scr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 119966);
        return true;
      }
      break;
    case 'D' :
      if (strncmp(name + 1, "Jcy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1026);
        return true;
      }
      if (strncmp(name + 1, "Scy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1029);
        return true;
      }
      if (strncmp(name + 1, "Zcy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1039);
        return true;
      }
      if (strncmp(name + 1, "arr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8609);
        return true;
      }
      if (strncmp(name + 1, "opf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120123);
        return true;
      }
      if (strncmp(name + 1, "scr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 119967);
        return true;
      }
      break;
    case 'E' :
      if (strncmp(name + 1, "dot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 278);
        return true;
      }
      if (strncmp(name + 1, "opf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120124);
        return true;
      }
      if (strncmp(name + 1, "scr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8496);
        return true;
      }
      if (strncmp(name + 1, "sim", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10867);
        return true;
      }
      if (strncmp(name + 1, "uml", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 203);
        return true;
      }
      break;
    case 'F' :
      if (strncmp(name + 1, "opf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120125);
        return true;
      }
      if (strncmp(name + 1, "scr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8497);
        return true;
      }
      break;
    case 'G' :
      if (strncmp(name + 1, "Jcy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1027);
        return true;
      }
      if (strncmp(name + 1, "dot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 288);
        return true;
      }
      if (strncmp(name + 1, "opf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120126);
        return true;
      }
      if (strncmp(name + 1, "scr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 119970);
        return true;
      }
      break;
    case 'H' :
      if (strncmp(name + 1, "opf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8461);
        return true;
      }
      if (strncmp(name + 1, "scr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8459);
        return true;
      }
      break;
    case 'I' :
      if (strncmp(name + 1, "Ecy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1045);
        return true;
      }
      if (strncmp(name + 1, "Ocy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1025);
        return true;
      }
      if (strncmp(name + 1, "dot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 304);
        return true;
      }
      if (strncmp(name + 1, "opf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120128);
        return true;
      }
      if (strncmp(name + 1, "ota", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 921);
        return true;
      }
      if (strncmp(name + 1, "scr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8464);
        return true;
      }
      if (strncmp(name + 1, "uml", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 207);
        return true;
      }
      break;
    case 'J' :
      if (strncmp(name + 1, "opf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120129);
        return true;
      }
      if (strncmp(name + 1, "scr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 119973);
        return true;
      }
      break;
    case 'K' :
      if (strncmp(name + 1, "Hcy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1061);
        return true;
      }
      if (strncmp(name + 1, "Jcy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1036);
        return true;
      }
      if (strncmp(name + 1, "opf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120130);
        return true;
      }
      if (strncmp(name + 1, "scr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 119974);
        return true;
      }
      break;
    case 'L' :
      if (strncmp(name + 1, "Jcy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1033);
        return true;
      }
      if (strncmp(name + 1, "ang", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10218);
        return true;
      }
      if (strncmp(name + 1, "arr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8606);
        return true;
      }
      if (strncmp(name + 1, "opf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120131);
        return true;
      }
      if (strncmp(name + 1, "scr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8466);
        return true;
      }
      break;
    case 'M' :
      if (strncmp(name + 1, "opf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120132);
        return true;
      }
      if (strncmp(name + 1, "scr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8499);
        return true;
      }
      break;
    case 'N' :
      if (strncmp(name + 1, "Jcy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1034);
        return true;
      }
      if (strncmp(name + 1, "opf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8469);
        return true;
      }
      if (strncmp(name + 1, "scr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 119977);
        return true;
      }
      break;
    case 'O' :
      if (strncmp(name + 1, "opf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120134);
        return true;
      }
      if (strncmp(name + 1, "scr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 119978);
        return true;
      }
      if (strncmp(name + 1, "uml", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 214);
        return true;
      }
      break;
    case 'P' :
      if (strncmp(name + 1, "opf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8473);
        return true;
      }
      if (strncmp(name + 1, "scr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 119979);
        return true;
      }
      break;
    case 'Q' :
      if (strncmp(name + 1, "UOT", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 34);
        return true;
      }
      if (strncmp(name + 1, "opf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8474);
        return true;
      }
      if (strncmp(name + 1, "scr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 119980);
        return true;
      }
      break;
    case 'R' :
      if (strncmp(name + 1, "ang", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10219);
        return true;
      }
      if (strncmp(name + 1, "arr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8608);
        return true;
      }
      if (strncmp(name + 1, "opf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8477);
        return true;
      }
      if (strncmp(name + 1, "scr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8475);
        return true;
      }
      break;
    case 'S' :
      if (strncmp(name + 1, "Hcy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1064);
        return true;
      }
      if (strncmp(name + 1, "opf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120138);
        return true;
      }
      if (strncmp(name + 1, "qrt", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8730);
        return true;
      }
      if (strncmp(name + 1, "scr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 119982);
        return true;
      }
      if (strncmp(name + 1, "tar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8902);
        return true;
      }
      break;
    case 'T' :
      if (strncmp(name + 1, "Scy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1062);
        return true;
      }
      if (strncmp(name + 1, "opf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120139);
        return true;
      }
      if (strncmp(name + 1, "scr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 119983);
        return true;
      }
      break;
    case 'U' :
      if (strncmp(name + 1, "arr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8607);
        return true;
      }
      if (strncmp(name + 1, "opf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120140);
        return true;
      }
      if (strncmp(name + 1, "psi", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 978);
        return true;
      }
      if (strncmp(name + 1, "scr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 119984);
        return true;
      }
      if (strncmp(name + 1, "uml", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 220);
        return true;
      }
      break;
    case 'V' :
      if (strncmp(name + 1, "bar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10987);
        return true;
      }
      if (strncmp(name + 1, "ert", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8214);
        return true;
      }
      if (strncmp(name + 1, "opf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120141);
        return true;
      }
      if (strncmp(name + 1, "scr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 119985);
        return true;
      }
      break;
    case 'W' :
      if (strncmp(name + 1, "opf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120142);
        return true;
      }
      if (strncmp(name + 1, "scr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 119986);
        return true;
      }
      break;
    case 'X' :
      if (strncmp(name + 1, "opf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120143);
        return true;
      }
      if (strncmp(name + 1, "scr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 119987);
        return true;
      }
      break;
    case 'Y' :
      if (strncmp(name + 1, "Acy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1071);
        return true;
      }
      if (strncmp(name + 1, "Icy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1031);
        return true;
      }
      if (strncmp(name + 1, "Ucy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1070);
        return true;
      }
      if (strncmp(name + 1, "opf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120144);
        return true;
      }
      if (strncmp(name + 1, "scr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 119988);
        return true;
      }
      if (strncmp(name + 1, "uml", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 376);
        return true;
      }
      break;
    case 'Z' :
      if (strncmp(name + 1, "Hcy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1046);
        return true;
      }
      if (strncmp(name + 1, "dot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 379);
        return true;
      }
      if (strncmp(name + 1, "eta", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 918);
        return true;
      }
      if (strncmp(name + 1, "opf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8484);
        return true;
      }
      if (strncmp(name + 1, "scr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 119989);
        return true;
      }
      break;
    case 'a' :
      if (strncmp(name + 1, "ndd", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10844);
        return true;
      }
      if (strncmp(name + 1, "ndv", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10842);
        return true;
      }
      if (strncmp(name + 1, "nge", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10660);
        return true;
      }
      if (strncmp(name + 1, "opf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120146);
        return true;
      }
      if (strncmp(name + 1, "pid", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8779);
        return true;
      }
      if (strncmp(name + 1, "pos", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 39);
        return true;
      }
      if (strncmp(name + 1, "scr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 119990);
        return true;
      }
      if (strncmp(name + 1, "uml", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 228);
        return true;
      }
      break;
    case 'b' :
      if (strncmp(name + 1, "Not", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10989);
        return true;
      }
      if (strncmp(name + 1, "brk", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9141);
        return true;
      }
      if (strncmp(name + 1, "eta", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 946);
        return true;
      }
      if (strncmp(name + 1, "eth", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8502);
        return true;
      }
      if (strncmp(name + 1, "not", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8976);
        return true;
      }
      if (strncmp(name + 1, "opf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120147);
        return true;
      }
      if (strncmp(name + 1, "oxH", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9552);
        return true;
      }
      if (strncmp(name + 1, "oxV", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9553);
        return true;
      }
      if (strncmp(name + 1, "oxh", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9472);
        return true;
      }
      if (strncmp(name + 1, "oxv", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9474);
        return true;
      }
      if (strncmp(name + 1, "scr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 119991);
        return true;
      }
      if (strncmp(name + 1, "sim", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8765);
        return true;
      }
      if (strncmp(name + 1, "sol", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 92);
        return true;
      }
      if (strncmp(name + 1, "ull", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8226);
        return true;
      }
      if (strncmp(name + 1, "ump", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8782);
        return true;
      }
      break;
    case 'c' :
      if (strncmp(name + 1, "aps", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8745);
        grn_text_code_point(ctx, buffer, 65024);
        return true;
      }
      if (strncmp(name + 1, "dot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 267);
        return true;
      }
      if (strncmp(name + 1, "ent", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 162);
        return true;
      }
      if (strncmp(name + 1, "hcy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1095);
        return true;
      }
      if (strncmp(name + 1, "irE", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10691);
        return true;
      }
      if (strncmp(name + 1, "irc", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 710);
        return true;
      }
      if (strncmp(name + 1, "ire", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8791);
        return true;
      }
      if (strncmp(name + 1, "omp", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8705);
        return true;
      }
      if (strncmp(name + 1, "ong", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8773);
        return true;
      }
      if (strncmp(name + 1, "opf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120148);
        return true;
      }
      if (strncmp(name + 1, "opy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 169);
        return true;
      }
      if (strncmp(name + 1, "scr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 119992);
        return true;
      }
      if (strncmp(name + 1, "sub", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10959);
        return true;
      }
      if (strncmp(name + 1, "sup", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10960);
        return true;
      }
      if (strncmp(name + 1, "ups", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8746);
        grn_text_code_point(ctx, buffer, 65024);
        return true;
      }
      break;
    case 'd' :
      if (strncmp(name + 1, "Arr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8659);
        return true;
      }
      if (strncmp(name + 1, "Har", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10597);
        return true;
      }
      if (strncmp(name + 1, "arr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8595);
        return true;
      }
      if (strncmp(name + 1, "ash", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8208);
        return true;
      }
      if (strncmp(name + 1, "iam", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8900);
        return true;
      }
      if (strncmp(name + 1, "jcy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1106);
        return true;
      }
      if (strncmp(name + 1, "opf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120149);
        return true;
      }
      if (strncmp(name + 1, "scr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 119993);
        return true;
      }
      if (strncmp(name + 1, "scy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1109);
        return true;
      }
      if (strncmp(name + 1, "sol", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10742);
        return true;
      }
      if (strncmp(name + 1, "tri", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9663);
        return true;
      }
      if (strncmp(name + 1, "zcy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1119);
        return true;
      }
      break;
    case 'e' :
      if (strncmp(name + 1, "Dot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8785);
        return true;
      }
      if (strncmp(name + 1, "cir", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8790);
        return true;
      }
      if (strncmp(name + 1, "dot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 279);
        return true;
      }
      if (strncmp(name + 1, "msp", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8195);
        return true;
      }
      if (strncmp(name + 1, "nsp", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8194);
        return true;
      }
      if (strncmp(name + 1, "opf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120150);
        return true;
      }
      if (strncmp(name + 1, "par", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8917);
        return true;
      }
      if (strncmp(name + 1, "psi", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 949);
        return true;
      }
      if (strncmp(name + 1, "scr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8495);
        return true;
      }
      if (strncmp(name + 1, "sim", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8770);
        return true;
      }
      if (strncmp(name + 1, "uml", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 235);
        return true;
      }
      if (strncmp(name + 1, "uro", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8364);
        return true;
      }
      if (strncmp(name + 1, "xcl", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 33);
        return true;
      }
      break;
    case 'f' :
      if (strncmp(name + 1, "lat", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9837);
        return true;
      }
      if (strncmp(name + 1, "nof", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 402);
        return true;
      }
      if (strncmp(name + 1, "opf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120151);
        return true;
      }
      if (strncmp(name + 1, "ork", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8916);
        return true;
      }
      if (strncmp(name + 1, "scr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 119995);
        return true;
      }
      break;
    case 'g' :
      if (strncmp(name + 1, "dot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 289);
        return true;
      }
      if (strncmp(name + 1, "eqq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8807);
        return true;
      }
      if (strncmp(name + 1, "esl", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8923);
        grn_text_code_point(ctx, buffer, 65024);
        return true;
      }
      if (strncmp(name + 1, "jcy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1107);
        return true;
      }
      if (strncmp(name + 1, "nap", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10890);
        return true;
      }
      if (strncmp(name + 1, "neq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10888);
        return true;
      }
      if (strncmp(name + 1, "opf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120152);
        return true;
      }
      if (strncmp(name + 1, "scr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8458);
        return true;
      }
      if (strncmp(name + 1, "sim", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8819);
        return true;
      }
      if (strncmp(name + 1, "tcc", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10919);
        return true;
      }
      if (strncmp(name + 1, "vnE", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8809);
        grn_text_code_point(ctx, buffer, 65024);
        return true;
      }
      break;
    case 'h' :
      if (strncmp(name + 1, "Arr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8660);
        return true;
      }
      if (strncmp(name + 1, "alf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 189);
        return true;
      }
      if (strncmp(name + 1, "arr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8596);
        return true;
      }
      if (strncmp(name + 1, "bar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8463);
        return true;
      }
      if (strncmp(name + 1, "opf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120153);
        return true;
      }
      if (strncmp(name + 1, "scr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 119997);
        return true;
      }
      break;
    case 'i' :
      if (strncmp(name + 1, "ecy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1077);
        return true;
      }
      if (strncmp(name + 1, "mof", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8887);
        return true;
      }
      if (strncmp(name + 1, "ocy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1105);
        return true;
      }
      if (strncmp(name + 1, "opf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120154);
        return true;
      }
      if (strncmp(name + 1, "ota", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 953);
        return true;
      }
      if (strncmp(name + 1, "scr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 119998);
        return true;
      }
      if (strncmp(name + 1, "sin", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8712);
        return true;
      }
      if (strncmp(name + 1, "uml", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 239);
        return true;
      }
      break;
    case 'j' :
      if (strncmp(name + 1, "opf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120155);
        return true;
      }
      if (strncmp(name + 1, "scr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 119999);
        return true;
      }
      break;
    case 'k' :
      if (strncmp(name + 1, "hcy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1093);
        return true;
      }
      if (strncmp(name + 1, "jcy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1116);
        return true;
      }
      if (strncmp(name + 1, "opf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120156);
        return true;
      }
      if (strncmp(name + 1, "scr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120000);
        return true;
      }
      break;
    case 'l' :
      if (strncmp(name + 1, "Arr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8656);
        return true;
      }
      if (strncmp(name + 1, "Har", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10594);
        return true;
      }
      if (strncmp(name + 1, "ang", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10216);
        return true;
      }
      if (strncmp(name + 1, "arr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8592);
        return true;
      }
      if (strncmp(name + 1, "ate", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10925);
        return true;
      }
      if (strncmp(name + 1, "cub", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 123);
        return true;
      }
      if (strncmp(name + 1, "dca", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10550);
        return true;
      }
      if (strncmp(name + 1, "dsh", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8626);
        return true;
      }
      if (strncmp(name + 1, "eqq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8806);
        return true;
      }
      if (strncmp(name + 1, "esg", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8922);
        grn_text_code_point(ctx, buffer, 65024);
        return true;
      }
      if (strncmp(name + 1, "jcy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1113);
        return true;
      }
      if (strncmp(name + 1, "nap", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10889);
        return true;
      }
      if (strncmp(name + 1, "neq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10887);
        return true;
      }
      if (strncmp(name + 1, "opf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120157);
        return true;
      }
      if (strncmp(name + 1, "ozf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10731);
        return true;
      }
      if (strncmp(name + 1, "par", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 40);
        return true;
      }
      if (strncmp(name + 1, "scr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120001);
        return true;
      }
      if (strncmp(name + 1, "sim", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8818);
        return true;
      }
      if (strncmp(name + 1, "sqb", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 91);
        return true;
      }
      if (strncmp(name + 1, "tcc", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10918);
        return true;
      }
      if (strncmp(name + 1, "tri", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9667);
        return true;
      }
      if (strncmp(name + 1, "vnE", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8808);
        grn_text_code_point(ctx, buffer, 65024);
        return true;
      }
      break;
    case 'm' :
      if (strncmp(name + 1, "acr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 175);
        return true;
      }
      if (strncmp(name + 1, "ale", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9794);
        return true;
      }
      if (strncmp(name + 1, "alt", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10016);
        return true;
      }
      if (strncmp(name + 1, "lcp", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10971);
        return true;
      }
      if (strncmp(name + 1, "ldr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8230);
        return true;
      }
      if (strncmp(name + 1, "opf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120158);
        return true;
      }
      if (strncmp(name + 1, "scr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120002);
        return true;
      }
      break;
    case 'n' :
      if (strncmp(name + 1, "Gtv", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8811);
        grn_text_code_point(ctx, buffer, 824);
        return true;
      }
      if (strncmp(name + 1, "Ltv", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8810);
        grn_text_code_point(ctx, buffer, 824);
        return true;
      }
      if (strncmp(name + 1, "ang", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8736);
        grn_text_code_point(ctx, buffer, 8402);
        return true;
      }
      if (strncmp(name + 1, "apE", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10864);
        grn_text_code_point(ctx, buffer, 824);
        return true;
      }
      if (strncmp(name + 1, "bsp", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 160);
        return true;
      }
      if (strncmp(name + 1, "cap", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10819);
        return true;
      }
      if (strncmp(name + 1, "cup", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10818);
        return true;
      }
      if (strncmp(name + 1, "geq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8817);
        return true;
      }
      if (strncmp(name + 1, "ges", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10878);
        grn_text_code_point(ctx, buffer, 824);
        return true;
      }
      if (strncmp(name + 1, "gtr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8815);
        return true;
      }
      if (strncmp(name + 1, "isd", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8954);
        return true;
      }
      if (strncmp(name + 1, "jcy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1114);
        return true;
      }
      if (strncmp(name + 1, "ldr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8229);
        return true;
      }
      if (strncmp(name + 1, "leq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8816);
        return true;
      }
      if (strncmp(name + 1, "les", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10877);
        grn_text_code_point(ctx, buffer, 824);
        return true;
      }
      if (strncmp(name + 1, "mid", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8740);
        return true;
      }
      if (strncmp(name + 1, "opf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120159);
        return true;
      }
      if (strncmp(name + 1, "par", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8742);
        return true;
      }
      if (strncmp(name + 1, "pre", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10927);
        grn_text_code_point(ctx, buffer, 824);
        return true;
      }
      if (strncmp(name + 1, "sce", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10928);
        grn_text_code_point(ctx, buffer, 824);
        return true;
      }
      if (strncmp(name + 1, "scr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120003);
        return true;
      }
      if (strncmp(name + 1, "sim", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8769);
        return true;
      }
      if (strncmp(name + 1, "sub", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8836);
        return true;
      }
      if (strncmp(name + 1, "sup", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8837);
        return true;
      }
      if (strncmp(name + 1, "tgl", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8825);
        return true;
      }
      if (strncmp(name + 1, "tlg", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8824);
        return true;
      }
      if (strncmp(name + 1, "vap", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8781);
        grn_text_code_point(ctx, buffer, 8402);
        return true;
      }
      if (strncmp(name + 1, "vge", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8805);
        grn_text_code_point(ctx, buffer, 8402);
        return true;
      }
      if (strncmp(name + 1, "vgt", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 62);
        grn_text_code_point(ctx, buffer, 8402);
        return true;
      }
      if (strncmp(name + 1, "vle", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8804);
        grn_text_code_point(ctx, buffer, 8402);
        return true;
      }
      if (strncmp(name + 1, "vlt", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 60);
        grn_text_code_point(ctx, buffer, 8402);
        return true;
      }
      break;
    case 'o' :
      if (strncmp(name + 1, "ast", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8859);
        return true;
      }
      if (strncmp(name + 1, "cir", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8858);
        return true;
      }
      if (strncmp(name + 1, "div", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10808);
        return true;
      }
      if (strncmp(name + 1, "dot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8857);
        return true;
      }
      if (strncmp(name + 1, "gon", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 731);
        return true;
      }
      if (strncmp(name + 1, "int", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8750);
        return true;
      }
      if (strncmp(name + 1, "mid", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10678);
        return true;
      }
      if (strncmp(name + 1, "opf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120160);
        return true;
      }
      if (strncmp(name + 1, "par", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10679);
        return true;
      }
      if (strncmp(name + 1, "rdf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 170);
        return true;
      }
      if (strncmp(name + 1, "rdm", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 186);
        return true;
      }
      if (strncmp(name + 1, "ror", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10838);
        return true;
      }
      if (strncmp(name + 1, "scr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8500);
        return true;
      }
      if (strncmp(name + 1, "sol", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8856);
        return true;
      }
      if (strncmp(name + 1, "uml", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 246);
        return true;
      }
      break;
    case 'p' :
      if (strncmp(name + 1, "ara", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 182);
        return true;
      }
      if (strncmp(name + 1, "art", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8706);
        return true;
      }
      if (strncmp(name + 1, "erp", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8869);
        return true;
      }
      if (strncmp(name + 1, "hiv", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 981);
        return true;
      }
      if (strncmp(name + 1, "lus", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 43);
        return true;
      }
      if (strncmp(name + 1, "opf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120161);
        return true;
      }
      if (strncmp(name + 1, "rap", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10935);
        return true;
      }
      if (strncmp(name + 1, "rec", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8826);
        return true;
      }
      if (strncmp(name + 1, "rnE", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10933);
        return true;
      }
      if (strncmp(name + 1, "rod", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8719);
        return true;
      }
      if (strncmp(name + 1, "rop", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8733);
        return true;
      }
      if (strncmp(name + 1, "scr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120005);
        return true;
      }
      break;
    case 'q' :
      if (strncmp(name + 1, "int", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10764);
        return true;
      }
      if (strncmp(name + 1, "opf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120162);
        return true;
      }
      if (strncmp(name + 1, "scr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120006);
        return true;
      }
      if (strncmp(name + 1, "uot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 34);
        return true;
      }
      break;
    case 'r' :
      if (strncmp(name + 1, "Arr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8658);
        return true;
      }
      if (strncmp(name + 1, "Har", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10596);
        return true;
      }
      if (strncmp(name + 1, "ace", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8765);
        grn_text_code_point(ctx, buffer, 817);
        return true;
      }
      if (strncmp(name + 1, "ang", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10217);
        return true;
      }
      if (strncmp(name + 1, "arr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8594);
        return true;
      }
      if (strncmp(name + 1, "cub", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 125);
        return true;
      }
      if (strncmp(name + 1, "dca", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10551);
        return true;
      }
      if (strncmp(name + 1, "dsh", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8627);
        return true;
      }
      if (strncmp(name + 1, "eal", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8476);
        return true;
      }
      if (strncmp(name + 1, "ect", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9645);
        return true;
      }
      if (strncmp(name + 1, "hov", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1009);
        return true;
      }
      if (strncmp(name + 1, "ing", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 730);
        return true;
      }
      if (strncmp(name + 1, "opf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120163);
        return true;
      }
      if (strncmp(name + 1, "par", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 41);
        return true;
      }
      if (strncmp(name + 1, "scr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120007);
        return true;
      }
      if (strncmp(name + 1, "sqb", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 93);
        return true;
      }
      if (strncmp(name + 1, "tri", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9657);
        return true;
      }
      break;
    case 's' :
      if (strncmp(name + 1, "cap", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10936);
        return true;
      }
      if (strncmp(name + 1, "cnE", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10934);
        return true;
      }
      if (strncmp(name + 1, "dot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8901);
        return true;
      }
      if (strncmp(name + 1, "ect", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 167);
        return true;
      }
      if (strncmp(name + 1, "emi", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 59);
        return true;
      }
      if (strncmp(name + 1, "ext", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10038);
        return true;
      }
      if (strncmp(name + 1, "hcy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1096);
        return true;
      }
      if (strncmp(name + 1, "ime", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8771);
        return true;
      }
      if (strncmp(name + 1, "img", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10910);
        return true;
      }
      if (strncmp(name + 1, "iml", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10909);
        return true;
      }
      if (strncmp(name + 1, "mid", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8739);
        return true;
      }
      if (strncmp(name + 1, "mte", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10924);
        return true;
      }
      if (strncmp(name + 1, "olb", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10692);
        return true;
      }
      if (strncmp(name + 1, "opf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120164);
        return true;
      }
      if (strncmp(name + 1, "par", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8741);
        return true;
      }
      if (strncmp(name + 1, "quf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9642);
        return true;
      }
      if (strncmp(name + 1, "scr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120008);
        return true;
      }
      if (strncmp(name + 1, "tar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9734);
        return true;
      }
      if (strncmp(name + 1, "ubE", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10949);
        return true;
      }
      if (strncmp(name + 1, "ube", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8838);
        return true;
      }
      if (strncmp(name + 1, "ucc", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8827);
        return true;
      }
      if (strncmp(name + 1, "ung", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9834);
        return true;
      }
      if (strncmp(name + 1, "up1", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 185);
        return true;
      }
      if (strncmp(name + 1, "up2", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 178);
        return true;
      }
      if (strncmp(name + 1, "up3", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 179);
        return true;
      }
      if (strncmp(name + 1, "upE", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10950);
        return true;
      }
      if (strncmp(name + 1, "upe", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8839);
        return true;
      }
      break;
    case 't' :
      if (strncmp(name + 1, "brk", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9140);
        return true;
      }
      if (strncmp(name + 1, "dot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8411);
        return true;
      }
      if (strncmp(name + 1, "int", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8749);
        return true;
      }
      if (strncmp(name + 1, "oea", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10536);
        return true;
      }
      if (strncmp(name + 1, "opf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120165);
        return true;
      }
      if (strncmp(name + 1, "osa", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10537);
        return true;
      }
      if (strncmp(name + 1, "rie", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8796);
        return true;
      }
      if (strncmp(name + 1, "scr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120009);
        return true;
      }
      if (strncmp(name + 1, "scy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1094);
        return true;
      }
      break;
    case 'u' :
      if (strncmp(name + 1, "Arr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8657);
        return true;
      }
      if (strncmp(name + 1, "Har", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10595);
        return true;
      }
      if (strncmp(name + 1, "arr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8593);
        return true;
      }
      if (strncmp(name + 1, "opf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120166);
        return true;
      }
      if (strncmp(name + 1, "psi", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 965);
        return true;
      }
      if (strncmp(name + 1, "scr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120010);
        return true;
      }
      if (strncmp(name + 1, "tri", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9653);
        return true;
      }
      if (strncmp(name + 1, "uml", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 252);
        return true;
      }
      break;
    case 'v' :
      if (strncmp(name + 1, "Arr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8661);
        return true;
      }
      if (strncmp(name + 1, "Bar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10984);
        return true;
      }
      if (strncmp(name + 1, "arr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8597);
        return true;
      }
      if (strncmp(name + 1, "ert", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 124);
        return true;
      }
      if (strncmp(name + 1, "opf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120167);
        return true;
      }
      if (strncmp(name + 1, "scr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120011);
        return true;
      }
      break;
    case 'w' :
      if (strncmp(name + 1, "opf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120168);
        return true;
      }
      if (strncmp(name + 1, "scr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120012);
        return true;
      }
      break;
    case 'x' :
      if (strncmp(name + 1, "cap", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8898);
        return true;
      }
      if (strncmp(name + 1, "cup", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8899);
        return true;
      }
      if (strncmp(name + 1, "map", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10236);
        return true;
      }
      if (strncmp(name + 1, "nis", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8955);
        return true;
      }
      if (strncmp(name + 1, "opf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120169);
        return true;
      }
      if (strncmp(name + 1, "scr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120013);
        return true;
      }
      if (strncmp(name + 1, "vee", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8897);
        return true;
      }
      break;
    case 'y' :
      if (strncmp(name + 1, "acy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1103);
        return true;
      }
      if (strncmp(name + 1, "icy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1111);
        return true;
      }
      if (strncmp(name + 1, "opf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120170);
        return true;
      }
      if (strncmp(name + 1, "scr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120014);
        return true;
      }
      if (strncmp(name + 1, "ucy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1102);
        return true;
      }
      if (strncmp(name + 1, "uml", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 255);
        return true;
      }
      break;
    case 'z' :
      if (strncmp(name + 1, "dot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 380);
        return true;
      }
      if (strncmp(name + 1, "eta", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 950);
        return true;
      }
      if (strncmp(name + 1, "hcy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1078);
        return true;
      }
      if (strncmp(name + 1, "opf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120171);
        return true;
      }
      if (strncmp(name + 1, "scr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 120015);
        return true;
      }
      if (strncmp(name + 1, "wnj", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8204);
        return true;
      }
      break;
    default :
      break;
    }
    break;
  case 5 :
    switch (name[0]) {
    case 'A' :
      if (strncmp(name + 1, "Elig", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 198);
        return true;
      }
      if (strncmp(name + 1, "circ", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 194);
        return true;
      }
      if (strncmp(name + 1, "lpha", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 913);
        return true;
      }
      if (strncmp(name + 1, "macr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 256);
        return true;
      }
      if (strncmp(name + 1, "ogon", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 260);
        return true;
      }
      if (strncmp(name + 1, "ring", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 197);
        return true;
      }
      break;
    case 'B' :
      if (strncmp(name + 1, "reve", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 728);
        return true;
      }
      break;
    case 'C' :
      if (strncmp(name + 1, "circ", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 264);
        return true;
      }
      if (strncmp(name + 1, "olon", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8759);
        return true;
      }
      if (strncmp(name + 1, "ross", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10799);
        return true;
      }
      break;
    case 'D' :
      if (strncmp(name + 1, "ashv", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10980);
        return true;
      }
      if (strncmp(name + 1, "elta", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 916);
        return true;
      }
      break;
    case 'E' :
      if (strncmp(name + 1, "circ", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 202);
        return true;
      }
      if (strncmp(name + 1, "macr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 274);
        return true;
      }
      if (strncmp(name + 1, "ogon", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 280);
        return true;
      }
      if (strncmp(name + 1, "qual", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10869);
        return true;
      }
      break;
    case 'G' :
      if (strncmp(name + 1, "amma", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 915);
        return true;
      }
      if (strncmp(name + 1, "circ", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 284);
        return true;
      }
      break;
    case 'H' :
      if (strncmp(name + 1, "acek", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 711);
        return true;
      }
      if (strncmp(name + 1, "circ", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 292);
        return true;
      }
      break;
    case 'I' :
      if (strncmp(name + 1, "Jlig", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 306);
        return true;
      }
      if (strncmp(name + 1, "circ", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 206);
        return true;
      }
      if (strncmp(name + 1, "macr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 298);
        return true;
      }
      if (strncmp(name + 1, "ogon", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 302);
        return true;
      }
      if (strncmp(name + 1, "ukcy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1030);
        return true;
      }
      break;
    case 'J' :
      if (strncmp(name + 1, "circ", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 308);
        return true;
      }
      if (strncmp(name + 1, "ukcy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1028);
        return true;
      }
      break;
    case 'K' :
      if (strncmp(name + 1, "appa", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 922);
        return true;
      }
      break;
    case 'O' :
      if (strncmp(name + 1, "Elig", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 338);
        return true;
      }
      if (strncmp(name + 1, "circ", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 212);
        return true;
      }
      if (strncmp(name + 1, "macr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 332);
        return true;
      }
      if (strncmp(name + 1, "mega", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 937);
        return true;
      }
      break;
    case 'P' :
      if (strncmp(name + 1, "rime", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8243);
        return true;
      }
      break;
    case 'R' :
      if (strncmp(name + 1, "Barr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10512);
        return true;
      }
      break;
    case 'S' :
      if (strncmp(name + 1, "circ", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 348);
        return true;
      }
      if (strncmp(name + 1, "igma", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 931);
        return true;
      }
      break;
    case 'T' :
      if (strncmp(name + 1, "HORN", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 222);
        return true;
      }
      if (strncmp(name + 1, "RADE", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8482);
        return true;
      }
      if (strncmp(name + 1, "SHcy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1035);
        return true;
      }
      if (strncmp(name + 1, "heta", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 920);
        return true;
      }
      if (strncmp(name + 1, "ilde", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8764);
        return true;
      }
      break;
    case 'U' :
      if (strncmp(name + 1, "brcy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1038);
        return true;
      }
      if (strncmp(name + 1, "circ", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 219);
        return true;
      }
      if (strncmp(name + 1, "macr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 362);
        return true;
      }
      if (strncmp(name + 1, "nion", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8899);
        return true;
      }
      if (strncmp(name + 1, "ogon", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 370);
        return true;
      }
      if (strncmp(name + 1, "pTee", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8869);
        return true;
      }
      if (strncmp(name + 1, "ring", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 366);
        return true;
      }
      break;
    case 'V' :
      if (strncmp(name + 1, "Dash", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8875);
        return true;
      }
      if (strncmp(name + 1, "dash", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8873);
        return true;
      }
      break;
    case 'W' :
      if (strncmp(name + 1, "circ", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 372);
        return true;
      }
      if (strncmp(name + 1, "edge", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8896);
        return true;
      }
      break;
    case 'Y' :
      if (strncmp(name + 1, "circ", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 374);
        return true;
      }
      break;
    case 'a' :
      if (strncmp(name + 1, "circ", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 226);
        return true;
      }
      if (strncmp(name + 1, "cute", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 180);
        return true;
      }
      if (strncmp(name + 1, "elig", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 230);
        return true;
      }
      if (strncmp(name + 1, "leph", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8501);
        return true;
      }
      if (strncmp(name + 1, "lpha", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 945);
        return true;
      }
      if (strncmp(name + 1, "macr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 257);
        return true;
      }
      if (strncmp(name + 1, "malg", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10815);
        return true;
      }
      if (strncmp(name + 1, "ngle", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8736);
        return true;
      }
      if (strncmp(name + 1, "ngrt", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8735);
        return true;
      }
      if (strncmp(name + 1, "ngst", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 197);
        return true;
      }
      if (strncmp(name + 1, "ogon", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 261);
        return true;
      }
      if (strncmp(name + 1, "ring", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 229);
        return true;
      }
      if (strncmp(name + 1, "symp", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8776);
        return true;
      }
      if (strncmp(name + 1, "wint", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10769);
        return true;
      }
      break;
    case 'b' :
      if (strncmp(name + 1, "cong", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8780);
        return true;
      }
      if (strncmp(name + 1, "dquo", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8222);
        return true;
      }
      if (strncmp(name + 1, "epsi", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1014);
        return true;
      }
      if (strncmp(name + 1, "lank", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9251);
        return true;
      }
      if (strncmp(name + 1, "lk12", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9618);
        return true;
      }
      if (strncmp(name + 1, "lk14", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9617);
        return true;
      }
      if (strncmp(name + 1, "lk34", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9619);
        return true;
      }
      if (strncmp(name + 1, "lock", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9608);
        return true;
      }
      if (strncmp(name + 1, "oxDL", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9559);
        return true;
      }
      if (strncmp(name + 1, "oxDR", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9556);
        return true;
      }
      if (strncmp(name + 1, "oxDl", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9558);
        return true;
      }
      if (strncmp(name + 1, "oxDr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9555);
        return true;
      }
      if (strncmp(name + 1, "oxHD", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9574);
        return true;
      }
      if (strncmp(name + 1, "oxHU", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9577);
        return true;
      }
      if (strncmp(name + 1, "oxHd", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9572);
        return true;
      }
      if (strncmp(name + 1, "oxHu", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9575);
        return true;
      }
      if (strncmp(name + 1, "oxUL", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9565);
        return true;
      }
      if (strncmp(name + 1, "oxUR", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9562);
        return true;
      }
      if (strncmp(name + 1, "oxUl", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9564);
        return true;
      }
      if (strncmp(name + 1, "oxUr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9561);
        return true;
      }
      if (strncmp(name + 1, "oxVH", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9580);
        return true;
      }
      if (strncmp(name + 1, "oxVL", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9571);
        return true;
      }
      if (strncmp(name + 1, "oxVR", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9568);
        return true;
      }
      if (strncmp(name + 1, "oxVh", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9579);
        return true;
      }
      if (strncmp(name + 1, "oxVl", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9570);
        return true;
      }
      if (strncmp(name + 1, "oxVr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9567);
        return true;
      }
      if (strncmp(name + 1, "oxdL", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9557);
        return true;
      }
      if (strncmp(name + 1, "oxdR", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9554);
        return true;
      }
      if (strncmp(name + 1, "oxdl", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9488);
        return true;
      }
      if (strncmp(name + 1, "oxdr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9484);
        return true;
      }
      if (strncmp(name + 1, "oxhD", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9573);
        return true;
      }
      if (strncmp(name + 1, "oxhU", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9576);
        return true;
      }
      if (strncmp(name + 1, "oxhd", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9516);
        return true;
      }
      if (strncmp(name + 1, "oxhu", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9524);
        return true;
      }
      if (strncmp(name + 1, "oxuL", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9563);
        return true;
      }
      if (strncmp(name + 1, "oxuR", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9560);
        return true;
      }
      if (strncmp(name + 1, "oxul", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9496);
        return true;
      }
      if (strncmp(name + 1, "oxur", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9492);
        return true;
      }
      if (strncmp(name + 1, "oxvH", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9578);
        return true;
      }
      if (strncmp(name + 1, "oxvL", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9569);
        return true;
      }
      if (strncmp(name + 1, "oxvR", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9566);
        return true;
      }
      if (strncmp(name + 1, "oxvh", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9532);
        return true;
      }
      if (strncmp(name + 1, "oxvl", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9508);
        return true;
      }
      if (strncmp(name + 1, "oxvr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9500);
        return true;
      }
      if (strncmp(name + 1, "reve", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 728);
        return true;
      }
      if (strncmp(name + 1, "semi", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8271);
        return true;
      }
      if (strncmp(name + 1, "sime", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8909);
        return true;
      }
      if (strncmp(name + 1, "solb", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10693);
        return true;
      }
      if (strncmp(name + 1, "umpE", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10926);
        return true;
      }
      if (strncmp(name + 1, "umpe", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8783);
        return true;
      }
      break;
    case 'c' :
      if (strncmp(name + 1, "aret", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8257);
        return true;
      }
      if (strncmp(name + 1, "aron", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 711);
        return true;
      }
      if (strncmp(name + 1, "caps", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10829);
        return true;
      }
      if (strncmp(name + 1, "circ", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 265);
        return true;
      }
      if (strncmp(name + 1, "cups", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10828);
        return true;
      }
      if (strncmp(name + 1, "edil", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 184);
        return true;
      }
      if (strncmp(name + 1, "heck", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10003);
        return true;
      }
      if (strncmp(name + 1, "lubs", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9827);
        return true;
      }
      if (strncmp(name + 1, "olon", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 58);
        return true;
      }
      if (strncmp(name + 1, "omma", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 44);
        return true;
      }
      if (strncmp(name + 1, "rarr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8629);
        return true;
      }
      if (strncmp(name + 1, "ross", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10007);
        return true;
      }
      if (strncmp(name + 1, "sube", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10961);
        return true;
      }
      if (strncmp(name + 1, "supe", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10962);
        return true;
      }
      if (strncmp(name + 1, "tdot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8943);
        return true;
      }
      if (strncmp(name + 1, "uepr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8926);
        return true;
      }
      if (strncmp(name + 1, "uesc", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8927);
        return true;
      }
      if (strncmp(name + 1, "upor", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10821);
        return true;
      }
      if (strncmp(name + 1, "uvee", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8910);
        return true;
      }
      if (strncmp(name + 1, "uwed", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8911);
        return true;
      }
      if (strncmp(name + 1, "wint", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8753);
        return true;
      }
      break;
    case 'd' :
      if (strncmp(name + 1, "ashv", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8867);
        return true;
      }
      if (strncmp(name + 1, "blac", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 733);
        return true;
      }
      if (strncmp(name + 1, "darr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8650);
        return true;
      }
      if (strncmp(name + 1, "elta", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 948);
        return true;
      }
      if (strncmp(name + 1, "harl", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8643);
        return true;
      }
      if (strncmp(name + 1, "harr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8642);
        return true;
      }
      if (strncmp(name + 1, "iams", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9830);
        return true;
      }
      if (strncmp(name + 1, "isin", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8946);
        return true;
      }
      if (strncmp(name + 1, "oteq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8784);
        return true;
      }
      if (strncmp(name + 1, "tdot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8945);
        return true;
      }
      if (strncmp(name + 1, "trif", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9662);
        return true;
      }
      if (strncmp(name + 1, "uarr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8693);
        return true;
      }
      if (strncmp(name + 1, "uhar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10607);
        return true;
      }
      break;
    case 'e' :
      if (strncmp(name + 1, "DDot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10871);
        return true;
      }
      if (strncmp(name + 1, "circ", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 234);
        return true;
      }
      if (strncmp(name + 1, "fDot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8786);
        return true;
      }
      if (strncmp(name + 1, "macr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 275);
        return true;
      }
      if (strncmp(name + 1, "mpty", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8709);
        return true;
      }
      if (strncmp(name + 1, "ogon", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 281);
        return true;
      }
      if (strncmp(name + 1, "plus", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10865);
        return true;
      }
      if (strncmp(name + 1, "psiv", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1013);
        return true;
      }
      if (strncmp(name + 1, "qsim", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8770);
        return true;
      }
      if (strncmp(name + 1, "quiv", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8801);
        return true;
      }
      if (strncmp(name + 1, "rDot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8787);
        return true;
      }
      if (strncmp(name + 1, "rarr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10609);
        return true;
      }
      if (strncmp(name + 1, "sdot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8784);
        return true;
      }
      if (strncmp(name + 1, "xist", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8707);
        return true;
      }
      break;
    case 'f' :
      if (strncmp(name + 1, "flig", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 64256);
        return true;
      }
      if (strncmp(name + 1, "ilig", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 64257);
        return true;
      }
      if (strncmp(name + 1, "jlig", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 102);
        grn_text_code_point(ctx, buffer, 106);
        return true;
      }
      if (strncmp(name + 1, "llig", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 64258);
        return true;
      }
      if (strncmp(name + 1, "ltns", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9649);
        return true;
      }
      if (strncmp(name + 1, "orkv", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10969);
        return true;
      }
      if (strncmp(name + 1, "rasl", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8260);
        return true;
      }
      if (strncmp(name + 1, "rown", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8994);
        return true;
      }
      break;
    case 'g' :
      if (strncmp(name + 1, "amma", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 947);
        return true;
      }
      if (strncmp(name + 1, "circ", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 285);
        return true;
      }
      if (strncmp(name + 1, "escc", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10921);
        return true;
      }
      if (strncmp(name + 1, "imel", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8503);
        return true;
      }
      if (strncmp(name + 1, "neqq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8809);
        return true;
      }
      if (strncmp(name + 1, "nsim", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8935);
        return true;
      }
      if (strncmp(name + 1, "rave", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 96);
        return true;
      }
      if (strncmp(name + 1, "sime", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10894);
        return true;
      }
      if (strncmp(name + 1, "siml", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10896);
        return true;
      }
      if (strncmp(name + 1, "tcir", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10874);
        return true;
      }
      if (strncmp(name + 1, "tdot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8919);
        return true;
      }
      break;
    case 'h' :
      if (strncmp(name + 1, "arrw", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8621);
        return true;
      }
      if (strncmp(name + 1, "circ", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 293);
        return true;
      }
      if (strncmp(name + 1, "oarr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8703);
        return true;
      }
      break;
    case 'i' :
      if (strncmp(name + 1, "circ", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 238);
        return true;
      }
      if (strncmp(name + 1, "excl", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 161);
        return true;
      }
      if (strncmp(name + 1, "iint", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8749);
        return true;
      }
      if (strncmp(name + 1, "iota", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8489);
        return true;
      }
      if (strncmp(name + 1, "jlig", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 307);
        return true;
      }
      if (strncmp(name + 1, "macr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 299);
        return true;
      }
      if (strncmp(name + 1, "mage", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8465);
        return true;
      }
      if (strncmp(name + 1, "math", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 305);
        return true;
      }
      if (strncmp(name + 1, "mped", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 437);
        return true;
      }
      if (strncmp(name + 1, "nfin", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8734);
        return true;
      }
      if (strncmp(name + 1, "ogon", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 303);
        return true;
      }
      if (strncmp(name + 1, "prod", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10812);
        return true;
      }
      if (strncmp(name + 1, "sinE", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8953);
        return true;
      }
      if (strncmp(name + 1, "sins", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8948);
        return true;
      }
      if (strncmp(name + 1, "sinv", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8712);
        return true;
      }
      if (strncmp(name + 1, "ukcy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1110);
        return true;
      }
      break;
    case 'j' :
      if (strncmp(name + 1, "circ", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 309);
        return true;
      }
      if (strncmp(name + 1, "math", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 567);
        return true;
      }
      if (strncmp(name + 1, "ukcy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1108);
        return true;
      }
      break;
    case 'k' :
      if (strncmp(name + 1, "appa", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 954);
        return true;
      }
      break;
    case 'l' :
      if (strncmp(name + 1, "Aarr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8666);
        return true;
      }
      if (strncmp(name + 1, "Barr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10510);
        return true;
      }
      if (strncmp(name + 1, "angd", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10641);
        return true;
      }
      if (strncmp(name + 1, "aquo", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 171);
        return true;
      }
      if (strncmp(name + 1, "arrb", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8676);
        return true;
      }
      if (strncmp(name + 1, "ates", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10925);
        grn_text_code_point(ctx, buffer, 65024);
        return true;
      }
      if (strncmp(name + 1, "barr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10508);
        return true;
      }
      if (strncmp(name + 1, "bbrk", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10098);
        return true;
      }
      if (strncmp(name + 1, "brke", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10635);
        return true;
      }
      if (strncmp(name + 1, "ceil", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8968);
        return true;
      }
      if (strncmp(name + 1, "dquo", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8220);
        return true;
      }
      if (strncmp(name + 1, "escc", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10920);
        return true;
      }
      if (strncmp(name + 1, "hard", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8637);
        return true;
      }
      if (strncmp(name + 1, "haru", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8636);
        return true;
      }
      if (strncmp(name + 1, "hblk", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9604);
        return true;
      }
      if (strncmp(name + 1, "larr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8647);
        return true;
      }
      if (strncmp(name + 1, "ltri", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9722);
        return true;
      }
      if (strncmp(name + 1, "neqq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8808);
        return true;
      }
      if (strncmp(name + 1, "nsim", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8934);
        return true;
      }
      if (strncmp(name + 1, "oang", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10220);
        return true;
      }
      if (strncmp(name + 1, "oarr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8701);
        return true;
      }
      if (strncmp(name + 1, "obrk", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10214);
        return true;
      }
      if (strncmp(name + 1, "opar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10629);
        return true;
      }
      if (strncmp(name + 1, "rarr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8646);
        return true;
      }
      if (strncmp(name + 1, "rhar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8651);
        return true;
      }
      if (strncmp(name + 1, "rtri", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8895);
        return true;
      }
      if (strncmp(name + 1, "sime", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10893);
        return true;
      }
      if (strncmp(name + 1, "simg", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10895);
        return true;
      }
      if (strncmp(name + 1, "squo", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8216);
        return true;
      }
      if (strncmp(name + 1, "tcir", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10873);
        return true;
      }
      if (strncmp(name + 1, "tdot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8918);
        return true;
      }
      if (strncmp(name + 1, "trie", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8884);
        return true;
      }
      if (strncmp(name + 1, "trif", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9666);
        return true;
      }
      break;
    case 'm' :
      if (strncmp(name + 1, "DDot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8762);
        return true;
      }
      if (strncmp(name + 1, "dash", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8212);
        return true;
      }
      if (strncmp(name + 1, "icro", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 181);
        return true;
      }
      if (strncmp(name + 1, "inus", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8722);
        return true;
      }
      if (strncmp(name + 1, "umap", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8888);
        return true;
      }
      break;
    case 'n' :
      if (strncmp(name + 1, "abla", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8711);
        return true;
      }
      if (strncmp(name + 1, "apid", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8779);
        grn_text_code_point(ctx, buffer, 824);
        return true;
      }
      if (strncmp(name + 1, "apos", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 329);
        return true;
      }
      if (strncmp(name + 1, "atur", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9838);
        return true;
      }
      if (strncmp(name + 1, "bump", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8782);
        grn_text_code_point(ctx, buffer, 824);
        return true;
      }
      if (strncmp(name + 1, "cong", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8775);
        return true;
      }
      if (strncmp(name + 1, "dash", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8211);
        return true;
      }
      if (strncmp(name + 1, "eArr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8663);
        return true;
      }
      if (strncmp(name + 1, "earr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8599);
        return true;
      }
      if (strncmp(name + 1, "edot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8784);
        grn_text_code_point(ctx, buffer, 824);
        return true;
      }
      if (strncmp(name + 1, "esim", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8770);
        grn_text_code_point(ctx, buffer, 824);
        return true;
      }
      if (strncmp(name + 1, "geqq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8807);
        grn_text_code_point(ctx, buffer, 824);
        return true;
      }
      if (strncmp(name + 1, "gsim", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8821);
        return true;
      }
      if (strncmp(name + 1, "hArr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8654);
        return true;
      }
      if (strncmp(name + 1, "harr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8622);
        return true;
      }
      if (strncmp(name + 1, "hpar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10994);
        return true;
      }
      if (strncmp(name + 1, "lArr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8653);
        return true;
      }
      if (strncmp(name + 1, "larr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8602);
        return true;
      }
      if (strncmp(name + 1, "leqq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8806);
        grn_text_code_point(ctx, buffer, 824);
        return true;
      }
      if (strncmp(name + 1, "less", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8814);
        return true;
      }
      if (strncmp(name + 1, "lsim", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8820);
        return true;
      }
      if (strncmp(name + 1, "ltri", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8938);
        return true;
      }
      if (strncmp(name + 1, "otin", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8713);
        return true;
      }
      if (strncmp(name + 1, "otni", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8716);
        return true;
      }
      if (strncmp(name + 1, "part", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8706);
        grn_text_code_point(ctx, buffer, 824);
        return true;
      }
      if (strncmp(name + 1, "prec", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8832);
        return true;
      }
      if (strncmp(name + 1, "rArr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8655);
        return true;
      }
      if (strncmp(name + 1, "rarr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8603);
        return true;
      }
      if (strncmp(name + 1, "rtri", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8939);
        return true;
      }
      if (strncmp(name + 1, "sime", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8772);
        return true;
      }
      if (strncmp(name + 1, "smid", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8740);
        return true;
      }
      if (strncmp(name + 1, "spar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8742);
        return true;
      }
      if (strncmp(name + 1, "subE", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10949);
        grn_text_code_point(ctx, buffer, 824);
        return true;
      }
      if (strncmp(name + 1, "sube", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8840);
        return true;
      }
      if (strncmp(name + 1, "succ", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8833);
        return true;
      }
      if (strncmp(name + 1, "supE", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10950);
        grn_text_code_point(ctx, buffer, 824);
        return true;
      }
      if (strncmp(name + 1, "supe", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8841);
        return true;
      }
      if (strncmp(name + 1, "umsp", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8199);
        return true;
      }
      if (strncmp(name + 1, "vsim", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8764);
        grn_text_code_point(ctx, buffer, 8402);
        return true;
      }
      if (strncmp(name + 1, "wArr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8662);
        return true;
      }
      if (strncmp(name + 1, "warr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8598);
        return true;
      }
      break;
    case 'o' :
      if (strncmp(name + 1, "circ", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 244);
        return true;
      }
      if (strncmp(name + 1, "dash", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8861);
        return true;
      }
      if (strncmp(name + 1, "elig", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 339);
        return true;
      }
      if (strncmp(name + 1, "fcir", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10687);
        return true;
      }
      if (strncmp(name + 1, "hbar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10677);
        return true;
      }
      if (strncmp(name + 1, "larr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8634);
        return true;
      }
      if (strncmp(name + 1, "lcir", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10686);
        return true;
      }
      if (strncmp(name + 1, "line", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8254);
        return true;
      }
      if (strncmp(name + 1, "macr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 333);
        return true;
      }
      if (strncmp(name + 1, "mega", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 969);
        return true;
      }
      if (strncmp(name + 1, "perp", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10681);
        return true;
      }
      if (strncmp(name + 1, "plus", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8853);
        return true;
      }
      if (strncmp(name + 1, "rarr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8635);
        return true;
      }
      if (strncmp(name + 1, "rder", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8500);
        return true;
      }
      if (strncmp(name + 1, "vbar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9021);
        return true;
      }
      break;
    case 'p' :
      if (strncmp(name + 1, "arsl", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 11005);
        return true;
      }
      if (strncmp(name + 1, "hone", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9742);
        return true;
      }
      if (strncmp(name + 1, "lusb", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8862);
        return true;
      }
      if (strncmp(name + 1, "luse", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10866);
        return true;
      }
      if (strncmp(name + 1, "ound", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 163);
        return true;
      }
      if (strncmp(name + 1, "rcue", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8828);
        return true;
      }
      if (strncmp(name + 1, "rime", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8242);
        return true;
      }
      if (strncmp(name + 1, "rnap", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10937);
        return true;
      }
      if (strncmp(name + 1, "rsim", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8830);
        return true;
      }
      break;
    case 'q' :
      if (strncmp(name + 1, "uest", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 63);
        return true;
      }
      break;
    case 'r' :
      if (strncmp(name + 1, "Aarr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8667);
        return true;
      }
      if (strncmp(name + 1, "Barr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10511);
        return true;
      }
      if (strncmp(name + 1, "adic", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8730);
        return true;
      }
      if (strncmp(name + 1, "angd", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10642);
        return true;
      }
      if (strncmp(name + 1, "ange", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10661);
        return true;
      }
      if (strncmp(name + 1, "aquo", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 187);
        return true;
      }
      if (strncmp(name + 1, "arrb", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8677);
        return true;
      }
      if (strncmp(name + 1, "arrc", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10547);
        return true;
      }
      if (strncmp(name + 1, "arrw", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8605);
        return true;
      }
      if (strncmp(name + 1, "atio", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8758);
        return true;
      }
      if (strncmp(name + 1, "barr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10509);
        return true;
      }
      if (strncmp(name + 1, "bbrk", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10099);
        return true;
      }
      if (strncmp(name + 1, "brke", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10636);
        return true;
      }
      if (strncmp(name + 1, "ceil", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8969);
        return true;
      }
      if (strncmp(name + 1, "dquo", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8221);
        return true;
      }
      if (strncmp(name + 1, "eals", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8477);
        return true;
      }
      if (strncmp(name + 1, "hard", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8641);
        return true;
      }
      if (strncmp(name + 1, "haru", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8640);
        return true;
      }
      if (strncmp(name + 1, "larr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8644);
        return true;
      }
      if (strncmp(name + 1, "lhar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8652);
        return true;
      }
      if (strncmp(name + 1, "nmid", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10990);
        return true;
      }
      if (strncmp(name + 1, "oang", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10221);
        return true;
      }
      if (strncmp(name + 1, "oarr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8702);
        return true;
      }
      if (strncmp(name + 1, "obrk", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10215);
        return true;
      }
      if (strncmp(name + 1, "opar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10630);
        return true;
      }
      if (strncmp(name + 1, "rarr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8649);
        return true;
      }
      if (strncmp(name + 1, "squo", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8217);
        return true;
      }
      if (strncmp(name + 1, "trie", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8885);
        return true;
      }
      if (strncmp(name + 1, "trif", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9656);
        return true;
      }
      break;
    case 's' :
      if (strncmp(name + 1, "bquo", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8218);
        return true;
      }
      if (strncmp(name + 1, "ccue", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8829);
        return true;
      }
      if (strncmp(name + 1, "circ", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 349);
        return true;
      }
      if (strncmp(name + 1, "cnap", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10938);
        return true;
      }
      if (strncmp(name + 1, "csim", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8831);
        return true;
      }
      if (strncmp(name + 1, "dotb", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8865);
        return true;
      }
      if (strncmp(name + 1, "dote", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10854);
        return true;
      }
      if (strncmp(name + 1, "eArr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8664);
        return true;
      }
      if (strncmp(name + 1, "earr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8600);
        return true;
      }
      if (strncmp(name + 1, "etmn", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8726);
        return true;
      }
      if (strncmp(name + 1, "harp", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9839);
        return true;
      }
      if (strncmp(name + 1, "igma", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 963);
        return true;
      }
      if (strncmp(name + 1, "imeq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8771);
        return true;
      }
      if (strncmp(name + 1, "imgE", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10912);
        return true;
      }
      if (strncmp(name + 1, "imlE", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10911);
        return true;
      }
      if (strncmp(name + 1, "imne", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8774);
        return true;
      }
      if (strncmp(name + 1, "larr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8592);
        return true;
      }
      if (strncmp(name + 1, "mile", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8995);
        return true;
      }
      if (strncmp(name + 1, "mtes", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10924);
        grn_text_code_point(ctx, buffer, 65024);
        return true;
      }
      if (strncmp(name + 1, "qcap", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8851);
        return true;
      }
      if (strncmp(name + 1, "qcup", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8852);
        return true;
      }
      if (strncmp(name + 1, "qsub", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8847);
        return true;
      }
      if (strncmp(name + 1, "qsup", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8848);
        return true;
      }
      if (strncmp(name + 1, "rarr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8594);
        return true;
      }
      if (strncmp(name + 1, "tarf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9733);
        return true;
      }
      if (strncmp(name + 1, "trns", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 175);
        return true;
      }
      if (strncmp(name + 1, "ubnE", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10955);
        return true;
      }
      if (strncmp(name + 1, "ubne", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8842);
        return true;
      }
      if (strncmp(name + 1, "upnE", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10956);
        return true;
      }
      if (strncmp(name + 1, "upne", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8843);
        return true;
      }
      if (strncmp(name + 1, "wArr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8665);
        return true;
      }
      if (strncmp(name + 1, "warr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8601);
        return true;
      }
      if (strncmp(name + 1, "zlig", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 223);
        return true;
      }
      break;
    case 't' :
      if (strncmp(name + 1, "heta", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 952);
        return true;
      }
      if (strncmp(name + 1, "hkap", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8776);
        return true;
      }
      if (strncmp(name + 1, "horn", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 254);
        return true;
      }
      if (strncmp(name + 1, "ilde", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 732);
        return true;
      }
      if (strncmp(name + 1, "imes", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 215);
        return true;
      }
      if (strncmp(name + 1, "rade", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8482);
        return true;
      }
      if (strncmp(name + 1, "risb", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10701);
        return true;
      }
      if (strncmp(name + 1, "shcy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1115);
        return true;
      }
      if (strncmp(name + 1, "wixt", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8812);
        return true;
      }
      break;
    case 'u' :
      if (strncmp(name + 1, "brcy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1118);
        return true;
      }
      if (strncmp(name + 1, "circ", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 251);
        return true;
      }
      if (strncmp(name + 1, "darr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8645);
        return true;
      }
      if (strncmp(name + 1, "dhar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10606);
        return true;
      }
      if (strncmp(name + 1, "harl", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8639);
        return true;
      }
      if (strncmp(name + 1, "harr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8638);
        return true;
      }
      if (strncmp(name + 1, "hblk", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9600);
        return true;
      }
      if (strncmp(name + 1, "ltri", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9720);
        return true;
      }
      if (strncmp(name + 1, "macr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 363);
        return true;
      }
      if (strncmp(name + 1, "ogon", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 371);
        return true;
      }
      if (strncmp(name + 1, "plus", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8846);
        return true;
      }
      if (strncmp(name + 1, "psih", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 978);
        return true;
      }
      if (strncmp(name + 1, "ring", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 367);
        return true;
      }
      if (strncmp(name + 1, "rtri", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9721);
        return true;
      }
      if (strncmp(name + 1, "tdot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8944);
        return true;
      }
      if (strncmp(name + 1, "trif", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9652);
        return true;
      }
      if (strncmp(name + 1, "uarr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8648);
        return true;
      }
      break;
    case 'v' :
      if (strncmp(name + 1, "Barv", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10985);
        return true;
      }
      if (strncmp(name + 1, "Dash", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8872);
        return true;
      }
      if (strncmp(name + 1, "arpi", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 982);
        return true;
      }
      if (strncmp(name + 1, "dash", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8866);
        return true;
      }
      if (strncmp(name + 1, "eeeq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8794);
        return true;
      }
      if (strncmp(name + 1, "ltri", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8882);
        return true;
      }
      if (strncmp(name + 1, "nsub", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8834);
        grn_text_code_point(ctx, buffer, 8402);
        return true;
      }
      if (strncmp(name + 1, "nsup", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8835);
        grn_text_code_point(ctx, buffer, 8402);
        return true;
      }
      if (strncmp(name + 1, "prop", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8733);
        return true;
      }
      if (strncmp(name + 1, "rtri", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8883);
        return true;
      }
      break;
    case 'w' :
      if (strncmp(name + 1, "circ", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 373);
        return true;
      }
      if (strncmp(name + 1, "edge", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8743);
        return true;
      }
      break;
    case 'x' :
      if (strncmp(name + 1, "circ", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9711);
        return true;
      }
      if (strncmp(name + 1, "dtri", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9661);
        return true;
      }
      if (strncmp(name + 1, "hArr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10234);
        return true;
      }
      if (strncmp(name + 1, "harr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10231);
        return true;
      }
      if (strncmp(name + 1, "lArr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10232);
        return true;
      }
      if (strncmp(name + 1, "larr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10229);
        return true;
      }
      if (strncmp(name + 1, "odot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10752);
        return true;
      }
      if (strncmp(name + 1, "rArr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10233);
        return true;
      }
      if (strncmp(name + 1, "rarr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10230);
        return true;
      }
      if (strncmp(name + 1, "utri", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9651);
        return true;
      }
      break;
    case 'y' :
      if (strncmp(name + 1, "circ", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 375);
        return true;
      }
      break;
    default :
      break;
    }
    break;
  case 6 :
    switch (name[0]) {
    case 'A' :
      if (strncmp(name + 1, "acute", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 193);
        return true;
      }
      if (strncmp(name + 1, "breve", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 258);
        return true;
      }
      if (strncmp(name + 1, "grave", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 192);
        return true;
      }
      if (strncmp(name + 1, "ssign", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8788);
        return true;
      }
      if (strncmp(name + 1, "tilde", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 195);
        return true;
      }
      break;
    case 'B' :
      if (strncmp(name + 1, "arwed", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8966);
        return true;
      }
      if (strncmp(name + 1, "umpeq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8782);
        return true;
      }
      break;
    case 'C' :
      if (strncmp(name + 1, "acute", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 262);
        return true;
      }
      if (strncmp(name + 1, "caron", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 268);
        return true;
      }
      if (strncmp(name + 1, "cedil", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 199);
        return true;
      }
      if (strncmp(name + 1, "olone", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10868);
        return true;
      }
      if (strncmp(name + 1, "onint", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8751);
        return true;
      }
      if (strncmp(name + 1, "upCap", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8781);
        return true;
      }
      break;
    case 'D' :
      if (strncmp(name + 1, "agger", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8225);
        return true;
      }
      if (strncmp(name + 1, "caron", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 270);
        return true;
      }
      if (strncmp(name + 1, "otDot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8412);
        return true;
      }
      if (strncmp(name + 1, "strok", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 272);
        return true;
      }
      break;
    case 'E' :
      if (strncmp(name + 1, "acute", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 201);
        return true;
      }
      if (strncmp(name + 1, "caron", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 282);
        return true;
      }
      if (strncmp(name + 1, "grave", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 200);
        return true;
      }
      if (strncmp(name + 1, "xists", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8707);
        return true;
      }
      break;
    case 'F' :
      if (strncmp(name + 1, "orAll", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8704);
        return true;
      }
      break;
    case 'G' :
      if (strncmp(name + 1, "ammad", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 988);
        return true;
      }
      if (strncmp(name + 1, "breve", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 286);
        return true;
      }
      if (strncmp(name + 1, "cedil", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 290);
        return true;
      }
      break;
    case 'H' :
      if (strncmp(name + 1, "ARDcy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1066);
        return true;
      }
      if (strncmp(name + 1, "strok", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 294);
        return true;
      }
      break;
    case 'I' :
      if (strncmp(name + 1, "acute", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 205);
        return true;
      }
      if (strncmp(name + 1, "grave", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 204);
        return true;
      }
      if (strncmp(name + 1, "tilde", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 296);
        return true;
      }
      break;
    case 'J' :
      if (strncmp(name + 1, "sercy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1032);
        return true;
      }
      break;
    case 'K' :
      if (strncmp(name + 1, "cedil", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 310);
        return true;
      }
      break;
    case 'L' :
      if (strncmp(name + 1, "acute", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 313);
        return true;
      }
      if (strncmp(name + 1, "ambda", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 923);
        return true;
      }
      if (strncmp(name + 1, "caron", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 317);
        return true;
      }
      if (strncmp(name + 1, "cedil", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 315);
        return true;
      }
      if (strncmp(name + 1, "midot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 319);
        return true;
      }
      if (strncmp(name + 1, "strok", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 321);
        return true;
      }
      break;
    case 'N' :
      if (strncmp(name + 1, "acute", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 323);
        return true;
      }
      if (strncmp(name + 1, "caron", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 327);
        return true;
      }
      if (strncmp(name + 1, "cedil", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 325);
        return true;
      }
      if (strncmp(name + 1, "tilde", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 209);
        return true;
      }
      break;
    case 'O' :
      if (strncmp(name + 1, "acute", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 211);
        return true;
      }
      if (strncmp(name + 1, "dblac", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 336);
        return true;
      }
      if (strncmp(name + 1, "grave", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 210);
        return true;
      }
      if (strncmp(name + 1, "slash", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 216);
        return true;
      }
      if (strncmp(name + 1, "tilde", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 213);
        return true;
      }
      if (strncmp(name + 1, "times", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10807);
        return true;
      }
      break;
    case 'R' :
      if (strncmp(name + 1, "acute", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 340);
        return true;
      }
      if (strncmp(name + 1, "arrtl", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10518);
        return true;
      }
      if (strncmp(name + 1, "caron", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 344);
        return true;
      }
      if (strncmp(name + 1, "cedil", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 342);
        return true;
      }
      break;
    case 'S' :
      if (strncmp(name + 1, "HCHcy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1065);
        return true;
      }
      if (strncmp(name + 1, "OFTcy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1068);
        return true;
      }
      if (strncmp(name + 1, "acute", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 346);
        return true;
      }
      if (strncmp(name + 1, "caron", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 352);
        return true;
      }
      if (strncmp(name + 1, "cedil", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 350);
        return true;
      }
      if (strncmp(name + 1, "quare", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9633);
        return true;
      }
      if (strncmp(name + 1, "ubset", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8912);
        return true;
      }
      if (strncmp(name + 1, "upset", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8913);
        return true;
      }
      break;
    case 'T' :
      if (strncmp(name + 1, "caron", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 356);
        return true;
      }
      if (strncmp(name + 1, "cedil", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 354);
        return true;
      }
      if (strncmp(name + 1, "strok", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 358);
        return true;
      }
      break;
    case 'U' :
      if (strncmp(name + 1, "acute", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 218);
        return true;
      }
      if (strncmp(name + 1, "breve", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 364);
        return true;
      }
      if (strncmp(name + 1, "dblac", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 368);
        return true;
      }
      if (strncmp(name + 1, "grave", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 217);
        return true;
      }
      if (strncmp(name + 1, "tilde", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 360);
        return true;
      }
      break;
    case 'V' :
      if (strncmp(name + 1, "dashl", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10982);
        return true;
      }
      if (strncmp(name + 1, "erbar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8214);
        return true;
      }
      if (strncmp(name + 1, "vdash", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8874);
        return true;
      }
      break;
    case 'Y' :
      if (strncmp(name + 1, "acute", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 221);
        return true;
      }
      break;
    case 'Z' :
      if (strncmp(name + 1, "acute", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 377);
        return true;
      }
      if (strncmp(name + 1, "caron", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 381);
        return true;
      }
      break;
    case 'a' :
      if (strncmp(name + 1, "acute", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 225);
        return true;
      }
      if (strncmp(name + 1, "breve", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 259);
        return true;
      }
      if (strncmp(name + 1, "grave", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 224);
        return true;
      }
      if (strncmp(name + 1, "ndand", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10837);
        return true;
      }
      if (strncmp(name + 1, "ngmsd", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8737);
        return true;
      }
      if (strncmp(name + 1, "ngsph", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8738);
        return true;
      }
      if (strncmp(name + 1, "pacir", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10863);
        return true;
      }
      if (strncmp(name + 1, "pprox", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8776);
        return true;
      }
      if (strncmp(name + 1, "tilde", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 227);
        return true;
      }
      break;
    case 'b' :
      if (strncmp(name + 1, "arvee", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8893);
        return true;
      }
      if (strncmp(name + 1, "arwed", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8965);
        return true;
      }
      if (strncmp(name + 1, "ecaus", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8757);
        return true;
      }
      if (strncmp(name + 1, "ernou", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8492);
        return true;
      }
      if (strncmp(name + 1, "igcap", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8898);
        return true;
      }
      if (strncmp(name + 1, "igcup", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8899);
        return true;
      }
      if (strncmp(name + 1, "igvee", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8897);
        return true;
      }
      if (strncmp(name + 1, "karow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10509);
        return true;
      }
      if (strncmp(name + 1, "ottom", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8869);
        return true;
      }
      if (strncmp(name + 1, "owtie", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8904);
        return true;
      }
      if (strncmp(name + 1, "oxbox", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10697);
        return true;
      }
      if (strncmp(name + 1, "prime", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8245);
        return true;
      }
      if (strncmp(name + 1, "rvbar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 166);
        return true;
      }
      if (strncmp(name + 1, "ullet", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8226);
        return true;
      }
      if (strncmp(name + 1, "umpeq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8783);
        return true;
      }
      break;
    case 'c' :
      if (strncmp(name + 1, "acute", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 263);
        return true;
      }
      if (strncmp(name + 1, "apand", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10820);
        return true;
      }
      if (strncmp(name + 1, "apcap", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10827);
        return true;
      }
      if (strncmp(name + 1, "apcup", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10823);
        return true;
      }
      if (strncmp(name + 1, "apdot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10816);
        return true;
      }
      if (strncmp(name + 1, "caron", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 269);
        return true;
      }
      if (strncmp(name + 1, "cedil", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 231);
        return true;
      }
      if (strncmp(name + 1, "irceq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8791);
        return true;
      }
      if (strncmp(name + 1, "irmid", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10991);
        return true;
      }
      if (strncmp(name + 1, "olone", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8788);
        return true;
      }
      if (strncmp(name + 1, "ommat", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 64);
        return true;
      }
      if (strncmp(name + 1, "ompfn", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8728);
        return true;
      }
      if (strncmp(name + 1, "onint", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8750);
        return true;
      }
      if (strncmp(name + 1, "oprod", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8720);
        return true;
      }
      if (strncmp(name + 1, "opysr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8471);
        return true;
      }
      if (strncmp(name + 1, "ularr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8630);
        return true;
      }
      if (strncmp(name + 1, "upcap", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10822);
        return true;
      }
      if (strncmp(name + 1, "upcup", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10826);
        return true;
      }
      if (strncmp(name + 1, "updot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8845);
        return true;
      }
      if (strncmp(name + 1, "urarr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8631);
        return true;
      }
      if (strncmp(name + 1, "urren", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 164);
        return true;
      }
      if (strncmp(name + 1, "ylcty", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9005);
        return true;
      }
      break;
    case 'd' :
      if (strncmp(name + 1, "agger", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8224);
        return true;
      }
      if (strncmp(name + 1, "aleth", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8504);
        return true;
      }
      if (strncmp(name + 1, "caron", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 271);
        return true;
      }
      if (strncmp(name + 1, "fisht", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10623);
        return true;
      }
      if (strncmp(name + 1, "ivide", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 247);
        return true;
      }
      if (strncmp(name + 1, "ivonx", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8903);
        return true;
      }
      if (strncmp(name + 1, "lcorn", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8990);
        return true;
      }
      if (strncmp(name + 1, "lcrop", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8973);
        return true;
      }
      if (strncmp(name + 1, "ollar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 36);
        return true;
      }
      if (strncmp(name + 1, "rcorn", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8991);
        return true;
      }
      if (strncmp(name + 1, "rcrop", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8972);
        return true;
      }
      if (strncmp(name + 1, "strok", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 273);
        return true;
      }
      break;
    case 'e' :
      if (strncmp(name + 1, "acute", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 233);
        return true;
      }
      if (strncmp(name + 1, "aster", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10862);
        return true;
      }
      if (strncmp(name + 1, "caron", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 283);
        return true;
      }
      if (strncmp(name + 1, "colon", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8789);
        return true;
      }
      if (strncmp(name + 1, "grave", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 232);
        return true;
      }
      if (strncmp(name + 1, "gsdot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10904);
        return true;
      }
      if (strncmp(name + 1, "lsdot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10903);
        return true;
      }
      if (strncmp(name + 1, "mptyv", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8709);
        return true;
      }
      if (strncmp(name + 1, "msp13", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8196);
        return true;
      }
      if (strncmp(name + 1, "msp14", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8197);
        return true;
      }
      if (strncmp(name + 1, "parsl", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10723);
        return true;
      }
      if (strncmp(name + 1, "qcirc", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8790);
        return true;
      }
      if (strncmp(name + 1, "quals", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 61);
        return true;
      }
      if (strncmp(name + 1, "quest", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8799);
        return true;
      }
      break;
    case 'f' :
      if (strncmp(name + 1, "emale", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9792);
        return true;
      }
      if (strncmp(name + 1, "filig", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 64259);
        return true;
      }
      if (strncmp(name + 1, "fllig", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 64260);
        return true;
      }
      if (strncmp(name + 1, "orall", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8704);
        return true;
      }
      if (strncmp(name + 1, "rac12", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 189);
        return true;
      }
      if (strncmp(name + 1, "rac13", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8531);
        return true;
      }
      if (strncmp(name + 1, "rac14", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 188);
        return true;
      }
      if (strncmp(name + 1, "rac15", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8533);
        return true;
      }
      if (strncmp(name + 1, "rac16", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8537);
        return true;
      }
      if (strncmp(name + 1, "rac18", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8539);
        return true;
      }
      if (strncmp(name + 1, "rac23", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8532);
        return true;
      }
      if (strncmp(name + 1, "rac25", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8534);
        return true;
      }
      if (strncmp(name + 1, "rac34", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 190);
        return true;
      }
      if (strncmp(name + 1, "rac35", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8535);
        return true;
      }
      if (strncmp(name + 1, "rac38", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8540);
        return true;
      }
      if (strncmp(name + 1, "rac45", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8536);
        return true;
      }
      if (strncmp(name + 1, "rac56", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8538);
        return true;
      }
      if (strncmp(name + 1, "rac58", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8541);
        return true;
      }
      if (strncmp(name + 1, "rac78", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8542);
        return true;
      }
      break;
    case 'g' :
      if (strncmp(name + 1, "acute", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 501);
        return true;
      }
      if (strncmp(name + 1, "ammad", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 989);
        return true;
      }
      if (strncmp(name + 1, "breve", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 287);
        return true;
      }
      if (strncmp(name + 1, "esdot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10880);
        return true;
      }
      if (strncmp(name + 1, "esles", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10900);
        return true;
      }
      if (strncmp(name + 1, "tlPar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10645);
        return true;
      }
      if (strncmp(name + 1, "trarr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10616);
        return true;
      }
      if (strncmp(name + 1, "trdot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8919);
        return true;
      }
      if (strncmp(name + 1, "trsim", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8819);
        return true;
      }
      break;
    case 'h' :
      if (strncmp(name + 1, "airsp", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8202);
        return true;
      }
      if (strncmp(name + 1, "amilt", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8459);
        return true;
      }
      if (strncmp(name + 1, "ardcy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1098);
        return true;
      }
      if (strncmp(name + 1, "earts", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9829);
        return true;
      }
      if (strncmp(name + 1, "ellip", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8230);
        return true;
      }
      if (strncmp(name + 1, "ercon", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8889);
        return true;
      }
      if (strncmp(name + 1, "omtht", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8763);
        return true;
      }
      if (strncmp(name + 1, "orbar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8213);
        return true;
      }
      if (strncmp(name + 1, "slash", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8463);
        return true;
      }
      if (strncmp(name + 1, "strok", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 295);
        return true;
      }
      if (strncmp(name + 1, "ybull", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8259);
        return true;
      }
      if (strncmp(name + 1, "yphen", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8208);
        return true;
      }
      break;
    case 'i' :
      if (strncmp(name + 1, "acute", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 237);
        return true;
      }
      if (strncmp(name + 1, "grave", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 236);
        return true;
      }
      if (strncmp(name + 1, "iiint", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10764);
        return true;
      }
      if (strncmp(name + 1, "infin", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10716);
        return true;
      }
      if (strncmp(name + 1, "ncare", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8453);
        return true;
      }
      if (strncmp(name + 1, "nodot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 305);
        return true;
      }
      if (strncmp(name + 1, "ntcal", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8890);
        return true;
      }
      if (strncmp(name + 1, "quest", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 191);
        return true;
      }
      if (strncmp(name + 1, "sinsv", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8947);
        return true;
      }
      if (strncmp(name + 1, "tilde", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 297);
        return true;
      }
      break;
    case 'j' :
      if (strncmp(name + 1, "sercy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1112);
        return true;
      }
      break;
    case 'k' :
      if (strncmp(name + 1, "appav", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1008);
        return true;
      }
      if (strncmp(name + 1, "cedil", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 311);
        return true;
      }
      if (strncmp(name + 1, "green", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 312);
        return true;
      }
      break;
    case 'l' :
      if (strncmp(name + 1, "Atail", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10523);
        return true;
      }
      if (strncmp(name + 1, "acute", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 314);
        return true;
      }
      if (strncmp(name + 1, "agran", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8466);
        return true;
      }
      if (strncmp(name + 1, "ambda", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 955);
        return true;
      }
      if (strncmp(name + 1, "angle", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10216);
        return true;
      }
      if (strncmp(name + 1, "arrfs", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10525);
        return true;
      }
      if (strncmp(name + 1, "arrhk", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8617);
        return true;
      }
      if (strncmp(name + 1, "arrlp", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8619);
        return true;
      }
      if (strncmp(name + 1, "arrpl", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10553);
        return true;
      }
      if (strncmp(name + 1, "arrtl", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8610);
        return true;
      }
      if (strncmp(name + 1, "atail", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10521);
        return true;
      }
      if (strncmp(name + 1, "brace", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 123);
        return true;
      }
      if (strncmp(name + 1, "brack", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 91);
        return true;
      }
      if (strncmp(name + 1, "caron", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 318);
        return true;
      }
      if (strncmp(name + 1, "cedil", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 316);
        return true;
      }
      if (strncmp(name + 1, "dquor", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8222);
        return true;
      }
      if (strncmp(name + 1, "esdot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10879);
        return true;
      }
      if (strncmp(name + 1, "esges", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10899);
        return true;
      }
      if (strncmp(name + 1, "fisht", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10620);
        return true;
      }
      if (strncmp(name + 1, "floor", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8970);
        return true;
      }
      if (strncmp(name + 1, "harul", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10602);
        return true;
      }
      if (strncmp(name + 1, "lhard", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10603);
        return true;
      }
      if (strncmp(name + 1, "midot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 320);
        return true;
      }
      if (strncmp(name + 1, "moust", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9136);
        return true;
      }
      if (strncmp(name + 1, "oplus", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10797);
        return true;
      }
      if (strncmp(name + 1, "owast", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8727);
        return true;
      }
      if (strncmp(name + 1, "owbar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 95);
        return true;
      }
      if (strncmp(name + 1, "parlt", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10643);
        return true;
      }
      if (strncmp(name + 1, "rhard", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10605);
        return true;
      }
      if (strncmp(name + 1, "saquo", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8249);
        return true;
      }
      if (strncmp(name + 1, "squor", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8218);
        return true;
      }
      if (strncmp(name + 1, "strok", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 322);
        return true;
      }
      if (strncmp(name + 1, "three", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8907);
        return true;
      }
      if (strncmp(name + 1, "times", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8905);
        return true;
      }
      if (strncmp(name + 1, "tlarr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10614);
        return true;
      }
      if (strncmp(name + 1, "trPar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10646);
        return true;
      }
      break;
    case 'm' :
      if (strncmp(name + 1, "apsto", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8614);
        return true;
      }
      if (strncmp(name + 1, "arker", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9646);
        return true;
      }
      if (strncmp(name + 1, "comma", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10793);
        return true;
      }
      if (strncmp(name + 1, "idast", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 42);
        return true;
      }
      if (strncmp(name + 1, "idcir", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10992);
        return true;
      }
      if (strncmp(name + 1, "iddot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 183);
        return true;
      }
      if (strncmp(name + 1, "inusb", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8863);
        return true;
      }
      if (strncmp(name + 1, "inusd", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8760);
        return true;
      }
      if (strncmp(name + 1, "nplus", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8723);
        return true;
      }
      if (strncmp(name + 1, "odels", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8871);
        return true;
      }
      if (strncmp(name + 1, "stpos", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8766);
        return true;
      }
      break;
    case 'n' :
      if (strncmp(name + 1, "VDash", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8879);
        return true;
      }
      if (strncmp(name + 1, "Vdash", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8878);
        return true;
      }
      if (strncmp(name + 1, "acute", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 324);
        return true;
      }
      if (strncmp(name + 1, "bumpe", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8783);
        grn_text_code_point(ctx, buffer, 824);
        return true;
      }
      if (strncmp(name + 1, "caron", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 328);
        return true;
      }
      if (strncmp(name + 1, "cedil", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 326);
        return true;
      }
      if (strncmp(name + 1, "earhk", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10532);
        return true;
      }
      if (strncmp(name + 1, "equiv", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8802);
        return true;
      }
      if (strncmp(name + 1, "esear", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10536);
        return true;
      }
      if (strncmp(name + 1, "exist", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8708);
        return true;
      }
      if (strncmp(name + 1, "ltrie", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8940);
        return true;
      }
      if (strncmp(name + 1, "otinE", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8953);
        grn_text_code_point(ctx, buffer, 824);
        return true;
      }
      if (strncmp(name + 1, "parsl", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 11005);
        grn_text_code_point(ctx, buffer, 8421);
        return true;
      }
      if (strncmp(name + 1, "prcue", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8928);
        return true;
      }
      if (strncmp(name + 1, "rarrc", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10547);
        grn_text_code_point(ctx, buffer, 824);
        return true;
      }
      if (strncmp(name + 1, "rarrw", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8605);
        grn_text_code_point(ctx, buffer, 824);
        return true;
      }
      if (strncmp(name + 1, "rtrie", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8941);
        return true;
      }
      if (strncmp(name + 1, "sccue", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8929);
        return true;
      }
      if (strncmp(name + 1, "simeq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8772);
        return true;
      }
      if (strncmp(name + 1, "tilde", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 241);
        return true;
      }
      if (strncmp(name + 1, "umero", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8470);
        return true;
      }
      if (strncmp(name + 1, "vDash", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8877);
        return true;
      }
      if (strncmp(name + 1, "vHarr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10500);
        return true;
      }
      if (strncmp(name + 1, "vdash", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8876);
        return true;
      }
      if (strncmp(name + 1, "vlArr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10498);
        return true;
      }
      if (strncmp(name + 1, "vrArr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10499);
        return true;
      }
      if (strncmp(name + 1, "warhk", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10531);
        return true;
      }
      if (strncmp(name + 1, "wnear", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10535);
        return true;
      }
      break;
    case 'o' :
      if (strncmp(name + 1, "acute", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 243);
        return true;
      }
      if (strncmp(name + 1, "dblac", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 337);
        return true;
      }
      if (strncmp(name + 1, "dsold", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10684);
        return true;
      }
      if (strncmp(name + 1, "grave", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 242);
        return true;
      }
      if (strncmp(name + 1, "minus", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8854);
        return true;
      }
      if (strncmp(name + 1, "rigof", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8886);
        return true;
      }
      if (strncmp(name + 1, "slash", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 248);
        return true;
      }
      if (strncmp(name + 1, "tilde", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 245);
        return true;
      }
      if (strncmp(name + 1, "times", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8855);
        return true;
      }
      break;
    case 'p' :
      if (strncmp(name + 1, "arsim", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10995);
        return true;
      }
      if (strncmp(name + 1, "ercnt", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 37);
        return true;
      }
      if (strncmp(name + 1, "eriod", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 46);
        return true;
      }
      if (strncmp(name + 1, "ermil", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8240);
        return true;
      }
      if (strncmp(name + 1, "hmmat", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8499);
        return true;
      }
      if (strncmp(name + 1, "lanck", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8463);
        return true;
      }
      if (strncmp(name + 1, "lankv", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8463);
        return true;
      }
      if (strncmp(name + 1, "lusdo", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8724);
        return true;
      }
      if (strncmp(name + 1, "lusdu", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10789);
        return true;
      }
      if (strncmp(name + 1, "lusmn", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 177);
        return true;
      }
      if (strncmp(name + 1, "receq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10927);
        return true;
      }
      if (strncmp(name + 1, "rimes", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8473);
        return true;
      }
      if (strncmp(name + 1, "rnsim", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8936);
        return true;
      }
      if (strncmp(name + 1, "ropto", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8733);
        return true;
      }
      if (strncmp(name + 1, "rurel", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8880);
        return true;
      }
      if (strncmp(name + 1, "uncsp", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8200);
        return true;
      }
      break;
    case 'q' :
      if (strncmp(name + 1, "prime", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8279);
        return true;
      }
      break;
    case 'r' :
      if (strncmp(name + 1, "Atail", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10524);
        return true;
      }
      if (strncmp(name + 1, "acute", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 341);
        return true;
      }
      if (strncmp(name + 1, "angle", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10217);
        return true;
      }
      if (strncmp(name + 1, "arrap", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10613);
        return true;
      }
      if (strncmp(name + 1, "arrfs", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10526);
        return true;
      }
      if (strncmp(name + 1, "arrhk", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8618);
        return true;
      }
      if (strncmp(name + 1, "arrlp", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8620);
        return true;
      }
      if (strncmp(name + 1, "arrpl", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10565);
        return true;
      }
      if (strncmp(name + 1, "arrtl", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8611);
        return true;
      }
      if (strncmp(name + 1, "atail", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10522);
        return true;
      }
      if (strncmp(name + 1, "brace", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 125);
        return true;
      }
      if (strncmp(name + 1, "brack", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 93);
        return true;
      }
      if (strncmp(name + 1, "caron", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 345);
        return true;
      }
      if (strncmp(name + 1, "cedil", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 343);
        return true;
      }
      if (strncmp(name + 1, "dquor", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8221);
        return true;
      }
      if (strncmp(name + 1, "fisht", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10621);
        return true;
      }
      if (strncmp(name + 1, "floor", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8971);
        return true;
      }
      if (strncmp(name + 1, "harul", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10604);
        return true;
      }
      if (strncmp(name + 1, "moust", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9137);
        return true;
      }
      if (strncmp(name + 1, "oplus", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10798);
        return true;
      }
      if (strncmp(name + 1, "pargt", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10644);
        return true;
      }
      if (strncmp(name + 1, "saquo", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8250);
        return true;
      }
      if (strncmp(name + 1, "squor", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8217);
        return true;
      }
      if (strncmp(name + 1, "three", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8908);
        return true;
      }
      if (strncmp(name + 1, "times", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8906);
        return true;
      }
      break;
    case 's' :
      if (strncmp(name + 1, "acute", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 347);
        return true;
      }
      if (strncmp(name + 1, "caron", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 353);
        return true;
      }
      if (strncmp(name + 1, "cedil", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 351);
        return true;
      }
      if (strncmp(name + 1, "cnsim", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8937);
        return true;
      }
      if (strncmp(name + 1, "earhk", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10533);
        return true;
      }
      if (strncmp(name + 1, "eswar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10537);
        return true;
      }
      if (strncmp(name + 1, "frown", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8994);
        return true;
      }
      if (strncmp(name + 1, "hchcy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1097);
        return true;
      }
      if (strncmp(name + 1, "igmaf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 962);
        return true;
      }
      if (strncmp(name + 1, "igmav", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 962);
        return true;
      }
      if (strncmp(name + 1, "imdot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10858);
        return true;
      }
      if (strncmp(name + 1, "mashp", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10803);
        return true;
      }
      if (strncmp(name + 1, "oftcy", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1100);
        return true;
      }
      if (strncmp(name + 1, "olbar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9023);
        return true;
      }
      if (strncmp(name + 1, "pades", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9824);
        return true;
      }
      if (strncmp(name + 1, "qcaps", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8851);
        grn_text_code_point(ctx, buffer, 65024);
        return true;
      }
      if (strncmp(name + 1, "qcups", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8852);
        grn_text_code_point(ctx, buffer, 65024);
        return true;
      }
      if (strncmp(name + 1, "qsube", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8849);
        return true;
      }
      if (strncmp(name + 1, "qsupe", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8850);
        return true;
      }
      if (strncmp(name + 1, "quare", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9633);
        return true;
      }
      if (strncmp(name + 1, "quarf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9642);
        return true;
      }
      if (strncmp(name + 1, "setmn", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8726);
        return true;
      }
      if (strncmp(name + 1, "smile", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8995);
        return true;
      }
      if (strncmp(name + 1, "starf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8902);
        return true;
      }
      if (strncmp(name + 1, "ubdot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10941);
        return true;
      }
      if (strncmp(name + 1, "ubset", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8834);
        return true;
      }
      if (strncmp(name + 1, "ubsim", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10951);
        return true;
      }
      if (strncmp(name + 1, "ubsub", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10965);
        return true;
      }
      if (strncmp(name + 1, "ubsup", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10963);
        return true;
      }
      if (strncmp(name + 1, "ucceq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10928);
        return true;
      }
      if (strncmp(name + 1, "updot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10942);
        return true;
      }
      if (strncmp(name + 1, "upset", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8835);
        return true;
      }
      if (strncmp(name + 1, "upsim", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10952);
        return true;
      }
      if (strncmp(name + 1, "upsub", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10964);
        return true;
      }
      if (strncmp(name + 1, "upsup", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10966);
        return true;
      }
      if (strncmp(name + 1, "warhk", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10534);
        return true;
      }
      if (strncmp(name + 1, "wnwar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10538);
        return true;
      }
      break;
    case 't' :
      if (strncmp(name + 1, "arget", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8982);
        return true;
      }
      if (strncmp(name + 1, "caron", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 357);
        return true;
      }
      if (strncmp(name + 1, "cedil", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 355);
        return true;
      }
      if (strncmp(name + 1, "elrec", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8981);
        return true;
      }
      if (strncmp(name + 1, "here4", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8756);
        return true;
      }
      if (strncmp(name + 1, "hetav", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 977);
        return true;
      }
      if (strncmp(name + 1, "hinsp", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8201);
        return true;
      }
      if (strncmp(name + 1, "hksim", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8764);
        return true;
      }
      if (strncmp(name + 1, "imesb", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8864);
        return true;
      }
      if (strncmp(name + 1, "imesd", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10800);
        return true;
      }
      if (strncmp(name + 1, "opbot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9014);
        return true;
      }
      if (strncmp(name + 1, "opcir", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10993);
        return true;
      }
      if (strncmp(name + 1, "prime", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8244);
        return true;
      }
      if (strncmp(name + 1, "ridot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9708);
        return true;
      }
      if (strncmp(name + 1, "strok", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 359);
        return true;
      }
      break;
    case 'u' :
      if (strncmp(name + 1, "acute", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 250);
        return true;
      }
      if (strncmp(name + 1, "breve", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 365);
        return true;
      }
      if (strncmp(name + 1, "dblac", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 369);
        return true;
      }
      if (strncmp(name + 1, "fisht", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10622);
        return true;
      }
      if (strncmp(name + 1, "grave", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 249);
        return true;
      }
      if (strncmp(name + 1, "lcorn", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8988);
        return true;
      }
      if (strncmp(name + 1, "lcrop", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8975);
        return true;
      }
      if (strncmp(name + 1, "rcorn", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8989);
        return true;
      }
      if (strncmp(name + 1, "rcrop", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8974);
        return true;
      }
      if (strncmp(name + 1, "tilde", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 361);
        return true;
      }
      break;
    case 'v' :
      if (strncmp(name + 1, "angrt", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10652);
        return true;
      }
      if (strncmp(name + 1, "arphi", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 981);
        return true;
      }
      if (strncmp(name + 1, "arrho", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1009);
        return true;
      }
      if (strncmp(name + 1, "eebar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8891);
        return true;
      }
      if (strncmp(name + 1, "ellip", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8942);
        return true;
      }
      if (strncmp(name + 1, "erbar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 124);
        return true;
      }
      if (strncmp(name + 1, "subnE", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10955);
        grn_text_code_point(ctx, buffer, 65024);
        return true;
      }
      if (strncmp(name + 1, "subne", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8842);
        grn_text_code_point(ctx, buffer, 65024);
        return true;
      }
      if (strncmp(name + 1, "supnE", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10956);
        grn_text_code_point(ctx, buffer, 65024);
        return true;
      }
      if (strncmp(name + 1, "supne", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8843);
        grn_text_code_point(ctx, buffer, 65024);
        return true;
      }
      break;
    case 'w' :
      if (strncmp(name + 1, "edbar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10847);
        return true;
      }
      if (strncmp(name + 1, "edgeq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8793);
        return true;
      }
      if (strncmp(name + 1, "eierp", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8472);
        return true;
      }
      if (strncmp(name + 1, "reath", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8768);
        return true;
      }
      break;
    case 'x' :
      if (strncmp(name + 1, "oplus", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10753);
        return true;
      }
      if (strncmp(name + 1, "otime", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10754);
        return true;
      }
      if (strncmp(name + 1, "sqcup", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10758);
        return true;
      }
      if (strncmp(name + 1, "uplus", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10756);
        return true;
      }
      if (strncmp(name + 1, "wedge", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8896);
        return true;
      }
      break;
    case 'y' :
      if (strncmp(name + 1, "acute", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 253);
        return true;
      }
      break;
    case 'z' :
      if (strncmp(name + 1, "acute", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 378);
        return true;
      }
      if (strncmp(name + 1, "caron", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 382);
        return true;
      }
      if (strncmp(name + 1, "eetrf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8488);
        return true;
      }
      break;
    default :
      break;
    }
    break;
  case 7 :
    switch (name[0]) {
    case 'B' :
      if (strncmp(name + 1, "ecause", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8757);
        return true;
      }
      break;
    case 'C' :
      if (strncmp(name + 1, "ayleys", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8493);
        return true;
      }
      if (strncmp(name + 1, "conint", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8752);
        return true;
      }
      if (strncmp(name + 1, "edilla", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 184);
        return true;
      }
      break;
    case 'D' :
      if (strncmp(name + 1, "iamond", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8900);
        return true;
      }
      if (strncmp(name + 1, "ownTee", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8868);
        return true;
      }
      break;
    case 'E' :
      if (strncmp(name + 1, "lement", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8712);
        return true;
      }
      if (strncmp(name + 1, "psilon", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 917);
        return true;
      }
      break;
    case 'I' :
      if (strncmp(name + 1, "mplies", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8658);
        return true;
      }
      break;
    case 'L' :
      if (strncmp(name + 1, "eftTee", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8867);
        return true;
      }
      break;
    case 'N' :
      if (strncmp(name + 1, "ewLine", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10);
        return true;
      }
      if (strncmp(name + 1, "oBreak", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8288);
        return true;
      }
      if (strncmp(name + 1, "otLess", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8814);
        return true;
      }
      break;
    case 'O' :
      if (strncmp(name + 1, "micron", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 927);
        return true;
      }
      if (strncmp(name + 1, "verBar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8254);
        return true;
      }
      break;
    case 'P' :
      if (strncmp(name + 1, "roduct", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8719);
        return true;
      }
      break;
    case 'U' :
      if (strncmp(name + 1, "pArrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8593);
        return true;
      }
      if (strncmp(name + 1, "parrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8657);
        return true;
      }
      if (strncmp(name + 1, "psilon", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 933);
        return true;
      }
      break;
    case 'a' :
      if (strncmp(name + 1, "lefsym", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8501);
        return true;
      }
      if (strncmp(name + 1, "ngrtvb", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8894);
        return true;
      }
      if (strncmp(name + 1, "ngzarr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9084);
        return true;
      }
      if (strncmp(name + 1, "sympeq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8781);
        return true;
      }
      break;
    case 'b' :
      if (strncmp(name + 1, "acksim", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8765);
        return true;
      }
      if (strncmp(name + 1, "ecause", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8757);
        return true;
      }
      if (strncmp(name + 1, "emptyv", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10672);
        return true;
      }
      if (strncmp(name + 1, "etween", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8812);
        return true;
      }
      if (strncmp(name + 1, "igcirc", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9711);
        return true;
      }
      if (strncmp(name + 1, "igodot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10752);
        return true;
      }
      if (strncmp(name + 1, "igstar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9733);
        return true;
      }
      if (strncmp(name + 1, "nequiv", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8801);
        grn_text_code_point(ctx, buffer, 8421);
        return true;
      }
      if (strncmp(name + 1, "oxplus", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8862);
        return true;
      }
      break;
    case 'c' :
      if (strncmp(name + 1, "cupssm", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10832);
        return true;
      }
      if (strncmp(name + 1, "emptyv", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10674);
        return true;
      }
      if (strncmp(name + 1, "irscir", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10690);
        return true;
      }
      if (strncmp(name + 1, "oloneq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8788);
        return true;
      }
      if (strncmp(name + 1, "ongdot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10861);
        return true;
      }
      if (strncmp(name + 1, "udarrl", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10552);
        return true;
      }
      if (strncmp(name + 1, "udarrr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10549);
        return true;
      }
      if (strncmp(name + 1, "ularrp", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10557);
        return true;
      }
      if (strncmp(name + 1, "urarrm", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10556);
        return true;
      }
      break;
    case 'd' :
      if (strncmp(name + 1, "bkarow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10511);
        return true;
      }
      if (strncmp(name + 1, "dagger", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8225);
        return true;
      }
      if (strncmp(name + 1, "dotseq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10871);
        return true;
      }
      if (strncmp(name + 1, "emptyv", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10673);
        return true;
      }
      if (strncmp(name + 1, "iamond", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8900);
        return true;
      }
      if (strncmp(name + 1, "igamma", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 989);
        return true;
      }
      if (strncmp(name + 1, "otplus", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8724);
        return true;
      }
      if (strncmp(name + 1, "wangle", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10662);
        return true;
      }
      break;
    case 'e' :
      if (strncmp(name + 1, "psilon", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 949);
        return true;
      }
      if (strncmp(name + 1, "qcolon", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8789);
        return true;
      }
      if (strncmp(name + 1, "quivDD", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10872);
        return true;
      }
      break;
    case 'g' :
      if (strncmp(name + 1, "esdoto", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10882);
        return true;
      }
      if (strncmp(name + 1, "tquest", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10876);
        return true;
      }
      if (strncmp(name + 1, "trless", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8823);
        return true;
      }
      break;
    case 'h' :
      if (strncmp(name + 1, "arrcir", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10568);
        return true;
      }
      break;
    case 'i' :
      if (strncmp(name + 1, "ntprod", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10812);
        return true;
      }
      if (strncmp(name + 1, "sindot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8949);
        return true;
      }
      break;
    case 'l' :
      if (strncmp(name + 1, "arrbfs", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10527);
        return true;
      }
      if (strncmp(name + 1, "arrsim", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10611);
        return true;
      }
      if (strncmp(name + 1, "brksld", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10639);
        return true;
      }
      if (strncmp(name + 1, "brkslu", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10637);
        return true;
      }
      if (strncmp(name + 1, "drdhar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10599);
        return true;
      }
      if (strncmp(name + 1, "esdoto", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10881);
        return true;
      }
      if (strncmp(name + 1, "essdot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8918);
        return true;
      }
      if (strncmp(name + 1, "essgtr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8822);
        return true;
      }
      if (strncmp(name + 1, "esssim", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8818);
        return true;
      }
      if (strncmp(name + 1, "otimes", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10804);
        return true;
      }
      if (strncmp(name + 1, "ozenge", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9674);
        return true;
      }
      if (strncmp(name + 1, "tquest", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10875);
        return true;
      }
      if (strncmp(name + 1, "uruhar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10598);
        return true;
      }
      break;
    case 'm' :
      if (strncmp(name + 1, "altese", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10016);
        return true;
      }
      if (strncmp(name + 1, "inusdu", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10794);
        return true;
      }
      break;
    case 'n' :
      if (strncmp(name + 1, "approx", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8777);
        return true;
      }
      if (strncmp(name + 1, "atural", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9838);
        return true;
      }
      if (strncmp(name + 1, "earrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8599);
        return true;
      }
      if (strncmp(name + 1, "exists", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8708);
        return true;
      }
      if (strncmp(name + 1, "otinva", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8713);
        return true;
      }
      if (strncmp(name + 1, "otinvb", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8951);
        return true;
      }
      if (strncmp(name + 1, "otinvc", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8950);
        return true;
      }
      if (strncmp(name + 1, "otniva", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8716);
        return true;
      }
      if (strncmp(name + 1, "otnivb", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8958);
        return true;
      }
      if (strncmp(name + 1, "otnivc", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8957);
        return true;
      }
      if (strncmp(name + 1, "polint", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10772);
        return true;
      }
      if (strncmp(name + 1, "preceq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10927);
        grn_text_code_point(ctx, buffer, 824);
        return true;
      }
      if (strncmp(name + 1, "sqsube", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8930);
        return true;
      }
      if (strncmp(name + 1, "sqsupe", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8931);
        return true;
      }
      if (strncmp(name + 1, "subset", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8834);
        grn_text_code_point(ctx, buffer, 8402);
        return true;
      }
      if (strncmp(name + 1, "succeq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10928);
        grn_text_code_point(ctx, buffer, 824);
        return true;
      }
      if (strncmp(name + 1, "supset", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8835);
        grn_text_code_point(ctx, buffer, 8402);
        return true;
      }
      if (strncmp(name + 1, "vinfin", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10718);
        return true;
      }
      if (strncmp(name + 1, "vltrie", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8884);
        grn_text_code_point(ctx, buffer, 8402);
        return true;
      }
      if (strncmp(name + 1, "vrtrie", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8885);
        grn_text_code_point(ctx, buffer, 8402);
        return true;
      }
      if (strncmp(name + 1, "warrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8598);
        return true;
      }
      break;
    case 'o' :
      if (strncmp(name + 1, "lcross", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10683);
        return true;
      }
      if (strncmp(name + 1, "micron", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 959);
        return true;
      }
      if (strncmp(name + 1, "rderof", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8500);
        return true;
      }
      if (strncmp(name + 1, "rslope", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10839);
        return true;
      }
      break;
    case 'p' :
      if (strncmp(name + 1, "ertenk", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8241);
        return true;
      }
      if (strncmp(name + 1, "lanckh", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8462);
        return true;
      }
      if (strncmp(name + 1, "luscir", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10786);
        return true;
      }
      if (strncmp(name + 1, "lussim", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10790);
        return true;
      }
      if (strncmp(name + 1, "lustwo", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10791);
        return true;
      }
      if (strncmp(name + 1, "recsim", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8830);
        return true;
      }
      break;
    case 'q' :
      if (strncmp(name + 1, "uatint", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10774);
        return true;
      }
      if (strncmp(name + 1, "uesteq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8799);
        return true;
      }
      break;
    case 'r' :
      if (strncmp(name + 1, "arrbfs", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10528);
        return true;
      }
      if (strncmp(name + 1, "arrsim", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10612);
        return true;
      }
      if (strncmp(name + 1, "brksld", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10638);
        return true;
      }
      if (strncmp(name + 1, "brkslu", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10640);
        return true;
      }
      if (strncmp(name + 1, "dldhar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10601);
        return true;
      }
      if (strncmp(name + 1, "ealine", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8475);
        return true;
      }
      if (strncmp(name + 1, "otimes", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10805);
        return true;
      }
      if (strncmp(name + 1, "uluhar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10600);
        return true;
      }
      break;
    case 's' :
      if (strncmp(name + 1, "earrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8600);
        return true;
      }
      if (strncmp(name + 1, "implus", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10788);
        return true;
      }
      if (strncmp(name + 1, "imrarr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10610);
        return true;
      }
      if (strncmp(name + 1, "ubedot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10947);
        return true;
      }
      if (strncmp(name + 1, "ubmult", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10945);
        return true;
      }
      if (strncmp(name + 1, "ubplus", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10943);
        return true;
      }
      if (strncmp(name + 1, "ubrarr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10617);
        return true;
      }
      if (strncmp(name + 1, "uccsim", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8831);
        return true;
      }
      if (strncmp(name + 1, "updsub", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10968);
        return true;
      }
      if (strncmp(name + 1, "upedot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10948);
        return true;
      }
      if (strncmp(name + 1, "uphsol", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10185);
        return true;
      }
      if (strncmp(name + 1, "uphsub", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10967);
        return true;
      }
      if (strncmp(name + 1, "uplarr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10619);
        return true;
      }
      if (strncmp(name + 1, "upmult", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10946);
        return true;
      }
      if (strncmp(name + 1, "upplus", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10944);
        return true;
      }
      if (strncmp(name + 1, "warrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8601);
        return true;
      }
      break;
    case 't' :
      if (strncmp(name + 1, "opfork", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10970);
        return true;
      }
      if (strncmp(name + 1, "riplus", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10809);
        return true;
      }
      if (strncmp(name + 1, "ritime", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10811);
        return true;
      }
      break;
    case 'u' :
      if (strncmp(name + 1, "parrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8593);
        return true;
      }
      if (strncmp(name + 1, "psilon", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 965);
        return true;
      }
      if (strncmp(name + 1, "wangle", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10663);
        return true;
      }
      break;
    case 'v' :
      if (strncmp(name + 1, "zigzag", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10650);
        return true;
      }
      break;
    case 'z' :
      if (strncmp(name + 1, "igrarr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8669);
        return true;
      }
      break;
    default :
      break;
    }
    break;
  case 8 :
    switch (name[0]) {
    case 'D' :
      if (strncmp(name + 1, "Dotrahd", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10513);
        return true;
      }
      if (strncmp(name + 1, "otEqual", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8784);
        return true;
      }
      break;
    case 'I' :
      if (strncmp(name + 1, "ntegral", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8747);
        return true;
      }
      break;
    case 'L' :
      if (strncmp(name + 1, "essLess", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10913);
        return true;
      }
      break;
    case 'N' :
      if (strncmp(name + 1, "otEqual", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8800);
        return true;
      }
      if (strncmp(name + 1, "otTilde", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8769);
        return true;
      }
      break;
    case 'P' :
      if (strncmp(name + 1, "artialD", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8706);
        return true;
      }
      if (strncmp(name + 1, "recedes", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8826);
        return true;
      }
      break;
    case 'R' :
      if (strncmp(name + 1, "ightTee", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8866);
        return true;
      }
      break;
    case 'S' :
      if (strncmp(name + 1, "ucceeds", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8827);
        return true;
      }
      if (strncmp(name + 1, "uchThat", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8715);
        return true;
      }
      if (strncmp(name + 1, "uperset", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8835);
        return true;
      }
      break;
    case 'U' :
      if (strncmp(name + 1, "arrocir", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10569);
        return true;
      }
      if (strncmp(name + 1, "nderBar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 95);
        return true;
      }
      break;
    case 'a' :
      if (strncmp(name + 1, "ndslope", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10840);
        return true;
      }
      if (strncmp(name + 1, "ngmsdaa", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10664);
        return true;
      }
      if (strncmp(name + 1, "ngmsdab", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10665);
        return true;
      }
      if (strncmp(name + 1, "ngmsdac", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10666);
        return true;
      }
      if (strncmp(name + 1, "ngmsdad", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10667);
        return true;
      }
      if (strncmp(name + 1, "ngmsdae", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10668);
        return true;
      }
      if (strncmp(name + 1, "ngmsdaf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10669);
        return true;
      }
      if (strncmp(name + 1, "ngmsdag", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10670);
        return true;
      }
      if (strncmp(name + 1, "ngmsdah", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10671);
        return true;
      }
      if (strncmp(name + 1, "ngrtvbd", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10653);
        return true;
      }
      if (strncmp(name + 1, "pproxeq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8778);
        return true;
      }
      if (strncmp(name + 1, "wconint", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8755);
        return true;
      }
      break;
    case 'b' :
      if (strncmp(name + 1, "ackcong", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8780);
        return true;
      }
      if (strncmp(name + 1, "arwedge", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8965);
        return true;
      }
      if (strncmp(name + 1, "brktbrk", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9142);
        return true;
      }
      if (strncmp(name + 1, "igoplus", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10753);
        return true;
      }
      if (strncmp(name + 1, "igsqcup", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10758);
        return true;
      }
      if (strncmp(name + 1, "iguplus", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10756);
        return true;
      }
      if (strncmp(name + 1, "igwedge", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8896);
        return true;
      }
      if (strncmp(name + 1, "oxminus", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8863);
        return true;
      }
      if (strncmp(name + 1, "oxtimes", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8864);
        return true;
      }
      if (strncmp(name + 1, "solhsub", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10184);
        return true;
      }
      break;
    case 'c' :
      if (strncmp(name + 1, "apbrcup", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10825);
        return true;
      }
      if (strncmp(name + 1, "ircledR", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 174);
        return true;
      }
      if (strncmp(name + 1, "ircledS", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9416);
        return true;
      }
      if (strncmp(name + 1, "irfnint", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10768);
        return true;
      }
      if (strncmp(name + 1, "lubsuit", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9827);
        return true;
      }
      if (strncmp(name + 1, "upbrcap", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10824);
        return true;
      }
      if (strncmp(name + 1, "urlyvee", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8910);
        return true;
      }
      if (strncmp(name + 1, "wconint", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8754);
        return true;
      }
      break;
    case 'd' :
      if (strncmp(name + 1, "oteqdot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8785);
        return true;
      }
      if (strncmp(name + 1, "otminus", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8760);
        return true;
      }
      if (strncmp(name + 1, "rbkarow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10512);
        return true;
      }
      if (strncmp(name + 1, "zigrarr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10239);
        return true;
      }
      break;
    case 'e' :
      if (strncmp(name + 1, "linters", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9191);
        return true;
      }
      if (strncmp(name + 1, "mptyset", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8709);
        return true;
      }
      if (strncmp(name + 1, "qvparsl", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10725);
        return true;
      }
      break;
    case 'f' :
      if (strncmp(name + 1, "partint", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10765);
        return true;
      }
      break;
    case 'g' :
      if (strncmp(name + 1, "eqslant", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10878);
        return true;
      }
      if (strncmp(name + 1, "esdotol", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10884);
        return true;
      }
      if (strncmp(name + 1, "napprox", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10890);
        return true;
      }
      break;
    case 'h' :
      if (strncmp(name + 1, "ksearow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10533);
        return true;
      }
      if (strncmp(name + 1, "kswarow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10534);
        return true;
      }
      break;
    case 'i' :
      if (strncmp(name + 1, "magline", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8464);
        return true;
      }
      if (strncmp(name + 1, "magpart", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8465);
        return true;
      }
      if (strncmp(name + 1, "nfintie", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10717);
        return true;
      }
      if (strncmp(name + 1, "ntegers", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8484);
        return true;
      }
      if (strncmp(name + 1, "ntercal", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8890);
        return true;
      }
      if (strncmp(name + 1, "ntlarhk", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10775);
        return true;
      }
      break;
    case 'l' :
      if (strncmp(name + 1, "aemptyv", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10676);
        return true;
      }
      if (strncmp(name + 1, "drushar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10571);
        return true;
      }
      if (strncmp(name + 1, "eqslant", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10877);
        return true;
      }
      if (strncmp(name + 1, "esdotor", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10883);
        return true;
      }
      if (strncmp(name + 1, "lcorner", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8990);
        return true;
      }
      if (strncmp(name + 1, "napprox", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10889);
        return true;
      }
      if (strncmp(name + 1, "rcorner", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8991);
        return true;
      }
      if (strncmp(name + 1, "urdshar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10570);
        return true;
      }
      break;
    case 'm' :
      if (strncmp(name + 1, "apstoup", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8613);
        return true;
      }
      if (strncmp(name + 1, "ultimap", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8888);
        return true;
      }
      break;
    case 'n' :
      if (strncmp(name + 1, "aturals", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8469);
        return true;
      }
      if (strncmp(name + 1, "congdot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10861);
        grn_text_code_point(ctx, buffer, 824);
        return true;
      }
      if (strncmp(name + 1, "otindot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8949);
        grn_text_code_point(ctx, buffer, 824);
        return true;
      }
      break;
    case 'o' :
      if (strncmp(name + 1, "timesas", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10806);
        return true;
      }
      break;
    case 'p' :
      if (strncmp(name + 1, "arallel", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8741);
        return true;
      }
      if (strncmp(name + 1, "lusacir", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10787);
        return true;
      }
      if (strncmp(name + 1, "ointint", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10773);
        return true;
      }
      if (strncmp(name + 1, "recneqq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10933);
        return true;
      }
      if (strncmp(name + 1, "recnsim", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8936);
        return true;
      }
      if (strncmp(name + 1, "rofalar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9006);
        return true;
      }
      if (strncmp(name + 1, "rofline", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8978);
        return true;
      }
      if (strncmp(name + 1, "rofsurf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8979);
        return true;
      }
      break;
    case 'r' :
      if (strncmp(name + 1, "aemptyv", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10675);
        return true;
      }
      if (strncmp(name + 1, "ealpart", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8476);
        return true;
      }
      if (strncmp(name + 1, "ppolint", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10770);
        return true;
      }
      if (strncmp(name + 1, "triltri", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10702);
        return true;
      }
      break;
    case 's' :
      if (strncmp(name + 1, "cpolint", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10771);
        return true;
      }
      if (strncmp(name + 1, "etminus", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8726);
        return true;
      }
      if (strncmp(name + 1, "hortmid", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8739);
        return true;
      }
      if (strncmp(name + 1, "meparsl", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10724);
        return true;
      }
      if (strncmp(name + 1, "qsubset", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8847);
        return true;
      }
      if (strncmp(name + 1, "qsupset", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8848);
        return true;
      }
      if (strncmp(name + 1, "ubseteq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8838);
        return true;
      }
      if (strncmp(name + 1, "uccneqq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10934);
        return true;
      }
      if (strncmp(name + 1, "uccnsim", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8937);
        return true;
      }
      if (strncmp(name + 1, "upseteq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8839);
        return true;
      }
      break;
    case 't' :
      if (strncmp(name + 1, "hetasym", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 977);
        return true;
      }
      if (strncmp(name + 1, "hicksim", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8764);
        return true;
      }
      if (strncmp(name + 1, "imesbar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10801);
        return true;
      }
      if (strncmp(name + 1, "riangle", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9653);
        return true;
      }
      if (strncmp(name + 1, "riminus", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10810);
        return true;
      }
      if (strncmp(name + 1, "rpezium", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9186);
        return true;
      }
      break;
    case 'u' :
      if (strncmp(name + 1, "lcorner", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8988);
        return true;
      }
      if (strncmp(name + 1, "rcorner", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8989);
        return true;
      }
      break;
    case 'v' :
      if (strncmp(name + 1, "arkappa", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1008);
        return true;
      }
      if (strncmp(name + 1, "arsigma", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 962);
        return true;
      }
      if (strncmp(name + 1, "artheta", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 977);
        return true;
      }
      break;
    default :
      break;
    }
    break;
  case 9 :
    switch (name[0]) {
    case 'B' :
      if (strncmp(name + 1, "ackslash", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8726);
        return true;
      }
      break;
    case 'C' :
      if (strncmp(name + 1, "enterDot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 183);
        return true;
      }
      if (strncmp(name + 1, "ircleDot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8857);
        return true;
      }
      if (strncmp(name + 1, "ongruent", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8801);
        return true;
      }
      if (strncmp(name + 1, "oproduct", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8720);
        return true;
      }
      break;
    case 'D' :
      if (strncmp(name + 1, "oubleDot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 168);
        return true;
      }
      if (strncmp(name + 1, "ownArrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8595);
        return true;
      }
      if (strncmp(name + 1, "ownBreve", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 785);
        return true;
      }
      if (strncmp(name + 1, "ownarrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8659);
        return true;
      }
      break;
    case 'H' :
      if (strncmp(name + 1, "umpEqual", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8783);
        return true;
      }
      break;
    case 'L' :
      if (strncmp(name + 1, "eftArrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8592);
        return true;
      }
      if (strncmp(name + 1, "eftFloor", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8970);
        return true;
      }
      if (strncmp(name + 1, "eftarrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8656);
        return true;
      }
      if (strncmp(name + 1, "essTilde", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8818);
        return true;
      }
      break;
    case 'M' :
      if (strncmp(name + 1, "ellintrf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8499);
        return true;
      }
      if (strncmp(name + 1, "inusPlus", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8723);
        return true;
      }
      break;
    case 'N' :
      if (strncmp(name + 1, "otCupCap", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8813);
        return true;
      }
      if (strncmp(name + 1, "otExists", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8708);
        return true;
      }
      if (strncmp(name + 1, "otSubset", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8834);
        grn_text_code_point(ctx, buffer, 8402);
        return true;
      }
      break;
    case 'O' :
      if (strncmp(name + 1, "verBrace", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9182);
        return true;
      }
      break;
    case 'P' :
      if (strncmp(name + 1, "lusMinus", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 177);
        return true;
      }
      break;
    case 'T' :
      if (strncmp(name + 1, "herefore", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8756);
        return true;
      }
      if (strncmp(name + 1, "hinSpace", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8201);
        return true;
      }
      if (strncmp(name + 1, "ripleDot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8411);
        return true;
      }
      break;
    case 'U' :
      if (strncmp(name + 1, "nionPlus", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8846);
        return true;
      }
      break;
    case 'b' :
      if (strncmp(name + 1, "ackprime", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8245);
        return true;
      }
      if (strncmp(name + 1, "acksimeq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8909);
        return true;
      }
      if (strncmp(name + 1, "igotimes", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10754);
        return true;
      }
      break;
    case 'c' :
      if (strncmp(name + 1, "enterdot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 183);
        return true;
      }
      if (strncmp(name + 1, "heckmark", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10003);
        return true;
      }
      if (strncmp(name + 1, "omplexes", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8450);
        return true;
      }
      break;
    case 'd' :
      if (strncmp(name + 1, "otsquare", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8865);
        return true;
      }
      if (strncmp(name + 1, "ownarrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8595);
        return true;
      }
      break;
    case 'g' :
      if (strncmp(name + 1, "trapprox", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10886);
        return true;
      }
      if (strncmp(name + 1, "treqless", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8923);
        return true;
      }
      if (strncmp(name + 1, "vertneqq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8809);
        grn_text_code_point(ctx, buffer, 65024);
        return true;
      }
      break;
    case 'h' :
      if (strncmp(name + 1, "eartsuit", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9829);
        return true;
      }
      break;
    case 'l' :
      if (strncmp(name + 1, "eftarrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8592);
        return true;
      }
      if (strncmp(name + 1, "esseqgtr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8922);
        return true;
      }
      if (strncmp(name + 1, "vertneqq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8808);
        grn_text_code_point(ctx, buffer, 65024);
        return true;
      }
      break;
    case 'n' :
      if (strncmp(name + 1, "geqslant", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10878);
        grn_text_code_point(ctx, buffer, 824);
        return true;
      }
      if (strncmp(name + 1, "leqslant", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10877);
        grn_text_code_point(ctx, buffer, 824);
        return true;
      }
      if (strncmp(name + 1, "parallel", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8742);
        return true;
      }
      if (strncmp(name + 1, "shortmid", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8740);
        return true;
      }
      if (strncmp(name + 1, "subseteq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8840);
        return true;
      }
      if (strncmp(name + 1, "supseteq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8841);
        return true;
      }
      break;
    case 'p' :
      if (strncmp(name + 1, "itchfork", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8916);
        return true;
      }
      break;
    case 'r' :
      if (strncmp(name + 1, "ationals", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8474);
        return true;
      }
      break;
    case 's' :
      if (strncmp(name + 1, "padesuit", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9824);
        return true;
      }
      if (strncmp(name + 1, "ubseteqq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10949);
        return true;
      }
      if (strncmp(name + 1, "ubsetneq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8842);
        return true;
      }
      if (strncmp(name + 1, "upseteqq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10950);
        return true;
      }
      if (strncmp(name + 1, "upsetneq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8843);
        return true;
      }
      break;
    case 't' :
      if (strncmp(name + 1, "herefore", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8756);
        return true;
      }
      if (strncmp(name + 1, "riangleq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8796);
        return true;
      }
      break;
    case 'v' :
      if (strncmp(name + 1, "arpropto", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8733);
        return true;
      }
      break;
    default :
      break;
    }
    break;
  case 10 :
    switch (name[0]) {
    case 'B' :
      if (strncmp(name + 1, "ernoullis", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8492);
        return true;
      }
      break;
    case 'C' :
      if (strncmp(name + 1, "irclePlus", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8853);
        return true;
      }
      break;
    case 'E' :
      if (strncmp(name + 1, "qualTilde", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8770);
        return true;
      }
      break;
    case 'F' :
      if (strncmp(name + 1, "ouriertrf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8497);
        return true;
      }
      break;
    case 'I' :
      if (strncmp(name + 1, "maginaryI", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8520);
        return true;
      }
      break;
    case 'L' :
      if (strncmp(name + 1, "aplacetrf", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8466);
        return true;
      }
      if (strncmp(name + 1, "eftVector", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8636);
        return true;
      }
      if (strncmp(name + 1, "leftarrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8666);
        return true;
      }
      break;
    case 'N' :
      if (strncmp(name + 1, "otElement", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8713);
        return true;
      }
      if (strncmp(name + 1, "otGreater", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8815);
        return true;
      }
      break;
    case 'P' :
      if (strncmp(name + 1, "roportion", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8759);
        return true;
      }
      break;
    case 'R' :
      if (strncmp(name + 1, "ightArrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8594);
        return true;
      }
      if (strncmp(name + 1, "ightFloor", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8971);
        return true;
      }
      if (strncmp(name + 1, "ightarrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8658);
        return true;
      }
      break;
    case 'T' :
      if (strncmp(name + 1, "hickSpace", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8287);
        grn_text_code_point(ctx, buffer, 8202);
        return true;
      }
      if (strncmp(name + 1, "ildeEqual", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8771);
        return true;
      }
      if (strncmp(name + 1, "ildeTilde", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8776);
        return true;
      }
      break;
    case 'U' :
      if (strncmp(name + 1, "nderBrace", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9183);
        return true;
      }
      if (strncmp(name + 1, "pArrowBar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10514);
        return true;
      }
      if (strncmp(name + 1, "pTeeArrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8613);
        return true;
      }
      break;
    case 'c' :
      if (strncmp(name + 1, "ircledast", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8859);
        return true;
      }
      if (strncmp(name + 1, "omplement", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8705);
        return true;
      }
      if (strncmp(name + 1, "urlywedge", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8911);
        return true;
      }
      break;
    case 'e' :
      if (strncmp(name + 1, "qslantgtr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10902);
        return true;
      }
      break;
    case 'g' :
      if (strncmp(name + 1, "treqqless", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10892);
        return true;
      }
      break;
    case 'l' :
      if (strncmp(name + 1, "essapprox", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10885);
        return true;
      }
      if (strncmp(name + 1, "esseqqgtr", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10891);
        return true;
      }
      if (strncmp(name + 1, "moustache", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9136);
        return true;
      }
      if (strncmp(name + 1, "ongmapsto", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10236);
        return true;
      }
      break;
    case 'm' :
      if (strncmp(name + 1, "apstodown", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8615);
        return true;
      }
      if (strncmp(name + 1, "apstoleft", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8612);
        return true;
      }
      break;
    case 'n' :
      if (strncmp(name + 1, "Leftarrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8653);
        return true;
      }
      if (strncmp(name + 1, "leftarrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8602);
        return true;
      }
      if (strncmp(name + 1, "subseteqq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10949);
        grn_text_code_point(ctx, buffer, 824);
        return true;
      }
      if (strncmp(name + 1, "supseteqq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10950);
        grn_text_code_point(ctx, buffer, 824);
        return true;
      }
      break;
    case 'p' :
      if (strncmp(name + 1, "recapprox", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10935);
        return true;
      }
      break;
    case 'r' :
      if (strncmp(name + 1, "ightarrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8594);
        return true;
      }
      if (strncmp(name + 1, "moustache", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9137);
        return true;
      }
      break;
    case 's' :
      if (strncmp(name + 1, "qsubseteq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8849);
        return true;
      }
      if (strncmp(name + 1, "qsupseteq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8850);
        return true;
      }
      if (strncmp(name + 1, "ubsetneqq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10955);
        return true;
      }
      if (strncmp(name + 1, "uccapprox", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10936);
        return true;
      }
      if (strncmp(name + 1, "upsetneqq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10956);
        return true;
      }
      break;
    case 'u' :
      if (strncmp(name + 1, "puparrows", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8648);
        return true;
      }
      break;
    case 'v' :
      if (strncmp(name + 1, "arepsilon", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1013);
        return true;
      }
      if (strncmp(name + 1, "arnothing", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8709);
        return true;
      }
      break;
    default :
      break;
    }
    break;
  case 11 :
    switch (name[0]) {
    case 'C' :
      if (strncmp(name + 1, "ircleMinus", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8854);
        return true;
      }
      if (strncmp(name + 1, "ircleTimes", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8855);
        return true;
      }
      break;
    case 'E' :
      if (strncmp(name + 1, "quilibrium", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8652);
        return true;
      }
      break;
    case 'G' :
      if (strncmp(name + 1, "reaterLess", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8823);
        return true;
      }
      break;
    case 'L' :
      if (strncmp(name + 1, "eftCeiling", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8968);
        return true;
      }
      if (strncmp(name + 1, "essGreater", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8822);
        return true;
      }
      break;
    case 'M' :
      if (strncmp(name + 1, "ediumSpace", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8287);
        return true;
      }
      break;
    case 'N' :
      if (strncmp(name + 1, "otLessLess", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8810);
        grn_text_code_point(ctx, buffer, 824);
        return true;
      }
      if (strncmp(name + 1, "otPrecedes", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8832);
        return true;
      }
      if (strncmp(name + 1, "otSucceeds", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8833);
        return true;
      }
      if (strncmp(name + 1, "otSuperset", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8835);
        grn_text_code_point(ctx, buffer, 8402);
        return true;
      }
      break;
    case 'O' :
      if (strncmp(name + 1, "verBracket", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9140);
        return true;
      }
      break;
    case 'R' :
      if (strncmp(name + 1, "ightVector", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8640);
        return true;
      }
      if (strncmp(name + 1, "rightarrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8667);
        return true;
      }
      if (strncmp(name + 1, "uleDelayed", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10740);
        return true;
      }
      break;
    case 'S' :
      if (strncmp(name + 1, "mallCircle", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8728);
        return true;
      }
      if (strncmp(name + 1, "quareUnion", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8852);
        return true;
      }
      if (strncmp(name + 1, "ubsetEqual", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8838);
        return true;
      }
      break;
    case 'U' :
      if (strncmp(name + 1, "pDownArrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8597);
        return true;
      }
      if (strncmp(name + 1, "pdownarrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8661);
        return true;
      }
      break;
    case 'V' :
      if (strncmp(name + 1, "erticalBar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8739);
        return true;
      }
      break;
    case 'b' :
      if (strncmp(name + 1, "ackepsilon", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1014);
        return true;
      }
      if (strncmp(name + 1, "lacksquare", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9642);
        return true;
      }
      break;
    case 'c' :
      if (strncmp(name + 1, "ircledcirc", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8858);
        return true;
      }
      if (strncmp(name + 1, "ircleddash", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8861);
        return true;
      }
      if (strncmp(name + 1, "urlyeqprec", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8926);
        return true;
      }
      if (strncmp(name + 1, "urlyeqsucc", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8927);
        return true;
      }
      break;
    case 'd' :
      if (strncmp(name + 1, "iamondsuit", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9830);
        return true;
      }
      break;
    case 'e' :
      if (strncmp(name + 1, "qslantless", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10901);
        return true;
      }
      if (strncmp(name + 1, "xpectation", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8496);
        return true;
      }
      break;
    case 'n' :
      if (strncmp(name + 1, "Rightarrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8655);
        return true;
      }
      if (strncmp(name + 1, "rightarrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8603);
        return true;
      }
      break;
    case 'p' :
      if (strncmp(name + 1, "reccurlyeq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8828);
        return true;
      }
      if (strncmp(name + 1, "recnapprox", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10937);
        return true;
      }
      break;
    case 'q' :
      if (strncmp(name + 1, "uaternions", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8461);
        return true;
      }
      break;
    case 's' :
      if (strncmp(name + 1, "traightphi", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 981);
        return true;
      }
      if (strncmp(name + 1, "ucccurlyeq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8829);
        return true;
      }
      if (strncmp(name + 1, "uccnapprox", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10938);
        return true;
      }
      break;
    case 't' :
      if (strncmp(name + 1, "hickapprox", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8776);
        return true;
      }
      break;
    case 'u' :
      if (strncmp(name + 1, "pdownarrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8597);
        return true;
      }
      break;
    default :
      break;
    }
    break;
  case 12 :
    switch (name[0]) {
    case 'D' :
      if (strncmp(name + 1, "ownArrowBar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10515);
        return true;
      }
      if (strncmp(name + 1, "ownTeeArrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8615);
        return true;
      }
      break;
    case 'E' :
      if (strncmp(name + 1, "xponentialE", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8519);
        return true;
      }
      break;
    case 'G' :
      if (strncmp(name + 1, "reaterEqual", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8805);
        return true;
      }
      if (strncmp(name + 1, "reaterTilde", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8819);
        return true;
      }
      break;
    case 'H' :
      if (strncmp(name + 1, "ilbertSpace", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8459);
        return true;
      }
      if (strncmp(name + 1, "umpDownHump", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8782);
        return true;
      }
      break;
    case 'I' :
      if (strncmp(name + 1, "ntersection", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8898);
        return true;
      }
      break;
    case 'L' :
      if (strncmp(name + 1, "eftArrowBar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8676);
        return true;
      }
      if (strncmp(name + 1, "eftTeeArrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8612);
        return true;
      }
      if (strncmp(name + 1, "eftTriangle", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8882);
        return true;
      }
      if (strncmp(name + 1, "eftUpVector", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8639);
        return true;
      }
      break;
    case 'N' :
      if (strncmp(name + 1, "otCongruent", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8802);
        return true;
      }
      if (strncmp(name + 1, "otHumpEqual", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8783);
        grn_text_code_point(ctx, buffer, 824);
        return true;
      }
      if (strncmp(name + 1, "otLessEqual", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8816);
        return true;
      }
      if (strncmp(name + 1, "otLessTilde", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8820);
        return true;
      }
      break;
    case 'P' :
      if (strncmp(name + 1, "roportional", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8733);
        return true;
      }
      break;
    case 'R' :
      if (strncmp(name + 1, "ightCeiling", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8969);
        return true;
      }
      if (strncmp(name + 1, "oundImplies", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10608);
        return true;
      }
      break;
    case 'S' :
      if (strncmp(name + 1, "hortUpArrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8593);
        return true;
      }
      if (strncmp(name + 1, "quareSubset", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8847);
        return true;
      }
      break;
    case 'U' :
      if (strncmp(name + 1, "nderBracket", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9141);
        return true;
      }
      break;
    case 'V' :
      if (strncmp(name + 1, "erticalLine", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 124);
        return true;
      }
      break;
    case 'b' :
      if (strncmp(name + 1, "lacklozenge", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10731);
        return true;
      }
      break;
    case 'e' :
      if (strncmp(name + 1, "xponentiale", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8519);
        return true;
      }
      break;
    case 'r' :
      if (strncmp(name + 1, "isingdotseq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8787);
        return true;
      }
      break;
    case 't' :
      if (strncmp(name + 1, "riangledown", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9663);
        return true;
      }
      if (strncmp(name + 1, "riangleleft", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9667);
        return true;
      }
      break;
    case 'v' :
      if (strncmp(name + 1, "arsubsetneq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8842);
        grn_text_code_point(ctx, buffer, 65024);
        return true;
      }
      if (strncmp(name + 1, "arsupsetneq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8843);
        grn_text_code_point(ctx, buffer, 65024);
        return true;
      }
      break;
    default :
      break;
    }
    break;
  case 13 :
    switch (name[0]) {
    case 'A' :
      if (strncmp(name + 1, "pplyFunction", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8289);
        return true;
      }
      break;
    case 'D' :
      if (strncmp(name + 1, "ifferentialD", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8518);
        return true;
      }
      if (strncmp(name + 1, "oubleLeftTee", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10980);
        return true;
      }
      if (strncmp(name + 1, "oubleUpArrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8657);
        return true;
      }
      break;
    case 'L' :
      if (strncmp(name + 1, "eftTeeVector", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10586);
        return true;
      }
      if (strncmp(name + 1, "eftVectorBar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10578);
        return true;
      }
      if (strncmp(name + 1, "essFullEqual", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8806);
        return true;
      }
      if (strncmp(name + 1, "ongLeftArrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10229);
        return true;
      }
      if (strncmp(name + 1, "ongleftarrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10232);
        return true;
      }
      break;
    case 'N' :
      if (strncmp(name + 1, "otEqualTilde", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8770);
        grn_text_code_point(ctx, buffer, 824);
        return true;
      }
      if (strncmp(name + 1, "otTildeEqual", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8772);
        return true;
      }
      if (strncmp(name + 1, "otTildeTilde", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8777);
        return true;
      }
      break;
    case 'P' :
      if (strncmp(name + 1, "oincareplane", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8460);
        return true;
      }
      if (strncmp(name + 1, "recedesEqual", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10927);
        return true;
      }
      if (strncmp(name + 1, "recedesTilde", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8830);
        return true;
      }
      break;
    case 'R' :
      if (strncmp(name + 1, "ightArrowBar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8677);
        return true;
      }
      if (strncmp(name + 1, "ightTeeArrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8614);
        return true;
      }
      if (strncmp(name + 1, "ightTriangle", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8883);
        return true;
      }
      if (strncmp(name + 1, "ightUpVector", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8638);
        return true;
      }
      break;
    case 'S' :
      if (strncmp(name + 1, "ucceedsEqual", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10928);
        return true;
      }
      if (strncmp(name + 1, "ucceedsTilde", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8831);
        return true;
      }
      if (strncmp(name + 1, "upersetEqual", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8839);
        return true;
      }
      break;
    case 'U' :
      if (strncmp(name + 1, "pEquilibrium", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10606);
        return true;
      }
      break;
    case 'V' :
      if (strncmp(name + 1, "erticalTilde", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8768);
        return true;
      }
      if (strncmp(name + 1, "eryThinSpace", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8202);
        return true;
      }
      break;
    case 'b' :
      if (strncmp(name + 1, "igtriangleup", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9651);
        return true;
      }
      if (strncmp(name + 1, "lacktriangle", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9652);
        return true;
      }
      break;
    case 'd' :
      if (strncmp(name + 1, "ivideontimes", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8903);
        return true;
      }
      break;
    case 'f' :
      if (strncmp(name + 1, "allingdotseq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8786);
        return true;
      }
      break;
    case 'h' :
      if (strncmp(name + 1, "ookleftarrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8617);
        return true;
      }
      break;
    case 'l' :
      if (strncmp(name + 1, "eftarrowtail", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8610);
        return true;
      }
      if (strncmp(name + 1, "eftharpoonup", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8636);
        return true;
      }
      if (strncmp(name + 1, "ongleftarrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10229);
        return true;
      }
      if (strncmp(name + 1, "ooparrowleft", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8619);
        return true;
      }
      break;
    case 'm' :
      if (strncmp(name + 1, "easuredangle", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8737);
        return true;
      }
      break;
    case 'n' :
      if (strncmp(name + 1, "triangleleft", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8938);
        return true;
      }
      break;
    case 's' :
      if (strncmp(name + 1, "hortparallel", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8741);
        return true;
      }
      if (strncmp(name + 1, "mallsetminus", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8726);
        return true;
      }
      break;
    case 't' :
      if (strncmp(name + 1, "riangleright", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9657);
        return true;
      }
      break;
    case 'u' :
      if (strncmp(name + 1, "pharpoonleft", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8639);
        return true;
      }
      break;
    case 'v' :
      if (strncmp(name + 1, "arsubsetneqq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10955);
        grn_text_code_point(ctx, buffer, 65024);
        return true;
      }
      if (strncmp(name + 1, "arsupsetneqq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10956);
        grn_text_code_point(ctx, buffer, 65024);
        return true;
      }
      break;
    default :
      break;
    }
    break;
  case 14 :
    switch (name[0]) {
    case 'D' :
      if (strncmp(name + 1, "iacriticalDot", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 729);
        return true;
      }
      if (strncmp(name + 1, "oubleRightTee", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8872);
        return true;
      }
      if (strncmp(name + 1, "ownLeftVector", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8637);
        return true;
      }
      break;
    case 'G' :
      if (strncmp(name + 1, "reaterGreater", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10914);
        return true;
      }
      break;
    case 'H' :
      if (strncmp(name + 1, "orizontalLine", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9472);
        return true;
      }
      break;
    case 'I' :
      if (strncmp(name + 1, "nvisibleComma", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8291);
        return true;
      }
      if (strncmp(name + 1, "nvisibleTimes", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8290);
        return true;
      }
      break;
    case 'L' :
      if (strncmp(name + 1, "eftDownVector", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8643);
        return true;
      }
      if (strncmp(name + 1, "eftRightArrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8596);
        return true;
      }
      if (strncmp(name + 1, "eftrightarrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8660);
        return true;
      }
      if (strncmp(name + 1, "essSlantEqual", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10877);
        return true;
      }
      if (strncmp(name + 1, "ongRightArrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10230);
        return true;
      }
      if (strncmp(name + 1, "ongrightarrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10233);
        return true;
      }
      if (strncmp(name + 1, "owerLeftArrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8601);
        return true;
      }
      break;
    case 'N' :
      if (strncmp(name + 1, "estedLessLess", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8810);
        return true;
      }
      if (strncmp(name + 1, "otGreaterLess", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8825);
        return true;
      }
      if (strncmp(name + 1, "otLessGreater", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8824);
        return true;
      }
      if (strncmp(name + 1, "otSubsetEqual", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8840);
        return true;
      }
      if (strncmp(name + 1, "otVerticalBar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8740);
        return true;
      }
      break;
    case 'O' :
      if (strncmp(name + 1, "penCurlyQuote", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8216);
        return true;
      }
      break;
    case 'R' :
      if (strncmp(name + 1, "everseElement", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8715);
        return true;
      }
      if (strncmp(name + 1, "ightTeeVector", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10587);
        return true;
      }
      if (strncmp(name + 1, "ightVectorBar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10579);
        return true;
      }
      break;
    case 'S' :
      if (strncmp(name + 1, "hortDownArrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8595);
        return true;
      }
      if (strncmp(name + 1, "hortLeftArrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8592);
        return true;
      }
      if (strncmp(name + 1, "quareSuperset", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8848);
        return true;
      }
      break;
    case 'T' :
      if (strncmp(name + 1, "ildeFullEqual", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8773);
        return true;
      }
      break;
    case 'U' :
      if (strncmp(name + 1, "pperLeftArrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8598);
        return true;
      }
      break;
    case 'Z' :
      if (strncmp(name + 1, "eroWidthSpace", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8203);
        return true;
      }
      break;
    case 'c' :
      if (strncmp(name + 1, "urvearrowleft", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8630);
        return true;
      }
      break;
    case 'd' :
      if (strncmp(name + 1, "oublebarwedge", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8966);
        return true;
      }
      if (strncmp(name + 1, "owndownarrows", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8650);
        return true;
      }
      break;
    case 'h' :
      if (strncmp(name + 1, "ookrightarrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8618);
        return true;
      }
      break;
    case 'l' :
      if (strncmp(name + 1, "eftleftarrows", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8647);
        return true;
      }
      if (strncmp(name + 1, "eftrightarrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8596);
        return true;
      }
      if (strncmp(name + 1, "eftthreetimes", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8907);
        return true;
      }
      if (strncmp(name + 1, "ongrightarrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10230);
        return true;
      }
      if (strncmp(name + 1, "ooparrowright", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8620);
        return true;
      }
      break;
    case 'n' :
      if (strncmp(name + 1, "shortparallel", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8742);
        return true;
      }
      if (strncmp(name + 1, "triangleright", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8939);
        return true;
      }
      break;
    case 'r' :
      if (strncmp(name + 1, "ightarrowtail", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8611);
        return true;
      }
      if (strncmp(name + 1, "ightharpoonup", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8640);
        return true;
      }
      break;
    case 't' :
      if (strncmp(name + 1, "rianglelefteq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8884);
        return true;
      }
      break;
    case 'u' :
      if (strncmp(name + 1, "pharpoonright", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8638);
        return true;
      }
      break;
    default :
      break;
    }
    break;
  case 15 :
    switch (name[0]) {
    case 'C' :
      if (strncmp(name + 1, "loseCurlyQuote", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8217);
        return true;
      }
      if (strncmp(name + 1, "ontourIntegral", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8750);
        return true;
      }
      break;
    case 'D' :
      if (strncmp(name + 1, "oubleDownArrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8659);
        return true;
      }
      if (strncmp(name + 1, "oubleLeftArrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8656);
        return true;
      }
      if (strncmp(name + 1, "ownRightVector", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8641);
        return true;
      }
      break;
    case 'L' :
      if (strncmp(name + 1, "eftRightVector", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10574);
        return true;
      }
      if (strncmp(name + 1, "eftTriangleBar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10703);
        return true;
      }
      if (strncmp(name + 1, "eftUpTeeVector", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10592);
        return true;
      }
      if (strncmp(name + 1, "eftUpVectorBar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10584);
        return true;
      }
      if (strncmp(name + 1, "owerRightArrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8600);
        return true;
      }
      break;
    case 'N' :
      if (strncmp(name + 1, "otGreaterEqual", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8817);
        return true;
      }
      if (strncmp(name + 1, "otGreaterTilde", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8821);
        return true;
      }
      if (strncmp(name + 1, "otHumpDownHump", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8782);
        grn_text_code_point(ctx, buffer, 824);
        return true;
      }
      if (strncmp(name + 1, "otLeftTriangle", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8938);
        return true;
      }
      if (strncmp(name + 1, "otSquareSubset", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8847);
        grn_text_code_point(ctx, buffer, 824);
        return true;
      }
      break;
    case 'O' :
      if (strncmp(name + 1, "verParenthesis", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9180);
        return true;
      }
      break;
    case 'R' :
      if (strncmp(name + 1, "ightDownVector", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8642);
        return true;
      }
      break;
    case 'S' :
      if (strncmp(name + 1, "hortRightArrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8594);
        return true;
      }
      break;
    case 'U' :
      if (strncmp(name + 1, "pperRightArrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8599);
        return true;
      }
      break;
    case 'b' :
      if (strncmp(name + 1, "igtriangledown", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9661);
        return true;
      }
      break;
    case 'c' :
      if (strncmp(name + 1, "irclearrowleft", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8634);
        return true;
      }
      if (strncmp(name + 1, "urvearrowright", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8631);
        return true;
      }
      break;
    case 'd' :
      if (strncmp(name + 1, "ownharpoonleft", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8643);
        return true;
      }
      break;
    case 'l' :
      if (strncmp(name + 1, "eftharpoondown", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8637);
        return true;
      }
      if (strncmp(name + 1, "eftrightarrows", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8646);
        return true;
      }
      break;
    case 'n' :
      if (strncmp(name + 1, "Leftrightarrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8654);
        return true;
      }
      if (strncmp(name + 1, "leftrightarrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8622);
        return true;
      }
      if (strncmp(name + 1, "trianglelefteq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8940);
        return true;
      }
      break;
    case 'r' :
      if (strncmp(name + 1, "ightleftarrows", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8644);
        return true;
      }
      if (strncmp(name + 1, "ightsquigarrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8605);
        return true;
      }
      if (strncmp(name + 1, "ightthreetimes", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8908);
        return true;
      }
      break;
    case 's' :
      if (strncmp(name + 1, "traightepsilon", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 1013);
        return true;
      }
      break;
    case 't' :
      if (strncmp(name + 1, "rianglerighteq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8885);
        return true;
      }
      break;
    case 'v' :
      if (strncmp(name + 1, "artriangleleft", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8882);
        return true;
      }
      break;
    default :
      break;
    }
    break;
  case 16 :
    switch (name[0]) {
    case 'D' :
      if (strncmp(name + 1, "iacriticalAcute", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 180);
        return true;
      }
      if (strncmp(name + 1, "iacriticalGrave", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 96);
        return true;
      }
      if (strncmp(name + 1, "iacriticalTilde", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 732);
        return true;
      }
      if (strncmp(name + 1, "oubleRightArrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8658);
        return true;
      }
      if (strncmp(name + 1, "ownArrowUpArrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8693);
        return true;
      }
      break;
    case 'E' :
      if (strncmp(name + 1, "mptySmallSquare", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9723);
        return true;
      }
      break;
    case 'G' :
      if (strncmp(name + 1, "reaterEqualLess", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8923);
        return true;
      }
      if (strncmp(name + 1, "reaterFullEqual", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8807);
        return true;
      }
      break;
    case 'L' :
      if (strncmp(name + 1, "eftAngleBracket", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10216);
        return true;
      }
      if (strncmp(name + 1, "eftUpDownVector", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10577);
        return true;
      }
      if (strncmp(name + 1, "essEqualGreater", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8922);
        return true;
      }
      break;
    case 'N' :
      if (strncmp(name + 1, "onBreakingSpace", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 160);
        return true;
      }
      if (strncmp(name + 1, "otPrecedesEqual", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10927);
        grn_text_code_point(ctx, buffer, 824);
        return true;
      }
      if (strncmp(name + 1, "otRightTriangle", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8939);
        return true;
      }
      if (strncmp(name + 1, "otSucceedsEqual", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10928);
        grn_text_code_point(ctx, buffer, 824);
        return true;
      }
      if (strncmp(name + 1, "otSucceedsTilde", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8831);
        grn_text_code_point(ctx, buffer, 824);
        return true;
      }
      if (strncmp(name + 1, "otSupersetEqual", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8841);
        return true;
      }
      break;
    case 'R' :
      if (strncmp(name + 1, "ightTriangleBar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10704);
        return true;
      }
      if (strncmp(name + 1, "ightUpTeeVector", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10588);
        return true;
      }
      if (strncmp(name + 1, "ightUpVectorBar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10580);
        return true;
      }
      break;
    case 'U' :
      if (strncmp(name + 1, "nderParenthesis", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9181);
        return true;
      }
      if (strncmp(name + 1, "pArrowDownArrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8645);
        return true;
      }
      break;
    case 'c' :
      if (strncmp(name + 1, "irclearrowright", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8635);
        return true;
      }
      break;
    case 'd' :
      if (strncmp(name + 1, "ownharpoonright", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8642);
        return true;
      }
      break;
    case 'n' :
      if (strncmp(name + 1, "trianglerighteq", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8941);
        return true;
      }
      break;
    case 'r' :
      if (strncmp(name + 1, "ightharpoondown", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8641);
        return true;
      }
      if (strncmp(name + 1, "ightrightarrows", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8649);
        return true;
      }
      break;
    case 't' :
      if (strncmp(name + 1, "woheadleftarrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8606);
        return true;
      }
      break;
    case 'v' :
      if (strncmp(name + 1, "artriangleright", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8883);
        return true;
      }
      break;
    default :
      break;
    }
    break;
  case 17 :
    switch (name[0]) {
    case 'D' :
      if (strncmp(name + 1, "oubleUpDownArrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8661);
        return true;
      }
      if (strncmp(name + 1, "oubleVerticalBar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8741);
        return true;
      }
      if (strncmp(name + 1, "ownLeftTeeVector", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10590);
        return true;
      }
      if (strncmp(name + 1, "ownLeftVectorBar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10582);
        return true;
      }
      break;
    case 'F' :
      if (strncmp(name + 1, "illedSmallSquare", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9724);
        return true;
      }
      break;
    case 'G' :
      if (strncmp(name + 1, "reaterSlantEqual", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10878);
        return true;
      }
      break;
    case 'L' :
      if (strncmp(name + 1, "eftDoubleBracket", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10214);
        return true;
      }
      if (strncmp(name + 1, "eftDownTeeVector", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10593);
        return true;
      }
      if (strncmp(name + 1, "eftDownVectorBar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10585);
        return true;
      }
      if (strncmp(name + 1, "eftTriangleEqual", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8884);
        return true;
      }
      break;
    case 'N' :
      if (strncmp(name + 1, "egativeThinSpace", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8203);
        return true;
      }
      if (strncmp(name + 1, "otGreaterGreater", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8811);
        grn_text_code_point(ctx, buffer, 824);
        return true;
      }
      if (strncmp(name + 1, "otLessSlantEqual", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10877);
        grn_text_code_point(ctx, buffer, 824);
        return true;
      }
      if (strncmp(name + 1, "otNestedLessLess", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10913);
        grn_text_code_point(ctx, buffer, 824);
        return true;
      }
      if (strncmp(name + 1, "otReverseElement", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8716);
        return true;
      }
      if (strncmp(name + 1, "otSquareSuperset", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8848);
        grn_text_code_point(ctx, buffer, 824);
        return true;
      }
      if (strncmp(name + 1, "otTildeFullEqual", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8775);
        return true;
      }
      break;
    case 'R' :
      if (strncmp(name + 1, "ightAngleBracket", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10217);
        return true;
      }
      if (strncmp(name + 1, "ightUpDownVector", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10575);
        return true;
      }
      break;
    case 'S' :
      if (strncmp(name + 1, "quareSubsetEqual", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8849);
        return true;
      }
      break;
    case 'V' :
      if (strncmp(name + 1, "erticalSeparator", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10072);
        return true;
      }
      break;
    case 'b' :
      if (strncmp(name + 1, "lacktriangledown", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9662);
        return true;
      }
      if (strncmp(name + 1, "lacktriangleleft", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9666);
        return true;
      }
      break;
    case 'l' :
      if (strncmp(name + 1, "eftrightharpoons", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8651);
        return true;
      }
      break;
    case 'r' :
      if (strncmp(name + 1, "ightleftharpoons", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8652);
        return true;
      }
      break;
    case 't' :
      if (strncmp(name + 1, "woheadrightarrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8608);
        return true;
      }
      break;
    default :
      break;
    }
    break;
  case 18 :
    switch (name[0]) {
    case 'D' :
      if (strncmp(name + 1, "ownRightTeeVector", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10591);
        return true;
      }
      if (strncmp(name + 1, "ownRightVectorBar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10583);
        return true;
      }
      break;
    case 'L' :
      if (strncmp(name + 1, "ongLeftRightArrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10231);
        return true;
      }
      if (strncmp(name + 1, "ongleftrightarrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10234);
        return true;
      }
      break;
    case 'N' :
      if (strncmp(name + 1, "egativeThickSpace", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8203);
        return true;
      }
      if (strncmp(name + 1, "otLeftTriangleBar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10703);
        grn_text_code_point(ctx, buffer, 824);
        return true;
      }
      break;
    case 'P' :
      if (strncmp(name + 1, "recedesSlantEqual", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8828);
        return true;
      }
      break;
    case 'R' :
      if (strncmp(name + 1, "everseEquilibrium", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8651);
        return true;
      }
      if (strncmp(name + 1, "ightDoubleBracket", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10215);
        return true;
      }
      if (strncmp(name + 1, "ightDownTeeVector", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10589);
        return true;
      }
      if (strncmp(name + 1, "ightDownVectorBar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10581);
        return true;
      }
      if (strncmp(name + 1, "ightTriangleEqual", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8885);
        return true;
      }
      break;
    case 'S' :
      if (strncmp(name + 1, "quareIntersection", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8851);
        return true;
      }
      if (strncmp(name + 1, "ucceedsSlantEqual", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8829);
        return true;
      }
      break;
    case 'b' :
      if (strncmp(name + 1, "lacktriangleright", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9656);
        return true;
      }
      break;
    case 'l' :
      if (strncmp(name + 1, "ongleftrightarrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10231);
        return true;
      }
      break;
    default :
      break;
    }
    break;
  case 19 :
    switch (name[0]) {
    case 'D' :
      if (strncmp(name + 1, "oubleLongLeftArrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10232);
        return true;
      }
      if (strncmp(name + 1, "ownLeftRightVector", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10576);
        return true;
      }
      break;
    case 'L' :
      if (strncmp(name + 1, "eftArrowRightArrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8646);
        return true;
      }
      break;
    case 'N' :
      if (strncmp(name + 1, "egativeMediumSpace", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8203);
        return true;
      }
      if (strncmp(name + 1, "otGreaterFullEqual", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8807);
        grn_text_code_point(ctx, buffer, 824);
        return true;
      }
      if (strncmp(name + 1, "otRightTriangleBar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10704);
        grn_text_code_point(ctx, buffer, 824);
        return true;
      }
      break;
    case 'R' :
      if (strncmp(name + 1, "ightArrowLeftArrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8644);
        return true;
      }
      break;
    case 'S' :
      if (strncmp(name + 1, "quareSupersetEqual", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8850);
        return true;
      }
      break;
    case 'l' :
      if (strncmp(name + 1, "eftrightsquigarrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8621);
        return true;
      }
      break;
    default :
      break;
    }
    break;
  case 20 :
    switch (name[0]) {
    case 'C' :
      if (strncmp(name + 1, "apitalDifferentialD", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8517);
        return true;
      }
      break;
    case 'D' :
      if (strncmp(name + 1, "oubleLeftRightArrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8660);
        return true;
      }
      if (strncmp(name + 1, "oubleLongRightArrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10233);
        return true;
      }
      break;
    case 'E' :
      if (strncmp(name + 1, "mptyVerySmallSquare", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9643);
        return true;
      }
      break;
    case 'N' :
      if (strncmp(name + 1, "estedGreaterGreater", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8811);
        return true;
      }
      if (strncmp(name + 1, "otDoubleVerticalBar", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8742);
        return true;
      }
      if (strncmp(name + 1, "otGreaterSlantEqual", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10878);
        grn_text_code_point(ctx, buffer, 824);
        return true;
      }
      if (strncmp(name + 1, "otLeftTriangleEqual", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8940);
        return true;
      }
      if (strncmp(name + 1, "otSquareSubsetEqual", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8930);
        return true;
      }
      break;
    case 'O' :
      if (strncmp(name + 1, "penCurlyDoubleQuote", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8220);
        return true;
      }
      break;
    case 'R' :
      if (strncmp(name + 1, "everseUpEquilibrium", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10607);
        return true;
      }
      break;
    default :
      break;
    }
    break;
  case 21 :
    switch (name[0]) {
    case 'C' :
      if (strncmp(name + 1, "loseCurlyDoubleQuote", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8221);
        return true;
      }
      break;
    case 'D' :
      if (strncmp(name + 1, "oubleContourIntegral", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8751);
        return true;
      }
      break;
    case 'F' :
      if (strncmp(name + 1, "illedVerySmallSquare", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 9642);
        return true;
      }
      break;
    case 'N' :
      if (strncmp(name + 1, "egativeVeryThinSpace", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8203);
        return true;
      }
      if (strncmp(name + 1, "otPrecedesSlantEqual", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8928);
        return true;
      }
      if (strncmp(name + 1, "otRightTriangleEqual", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8941);
        return true;
      }
      if (strncmp(name + 1, "otSucceedsSlantEqual", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8929);
        return true;
      }
      break;
    default :
      break;
    }
    break;
  case 22 :
    switch (name[0]) {
    case 'D' :
      if (strncmp(name + 1, "iacriticalDoubleAcute", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 733);
        return true;
      }
      break;
    case 'N' :
      if (strncmp(name + 1, "otSquareSupersetEqual", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8931);
        return true;
      }
      break;
    default :
      break;
    }
    break;
  case 23 :
    switch (name[0]) {
    case 'N' :
      if (strncmp(name + 1, "otNestedGreaterGreater", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10914);
        grn_text_code_point(ctx, buffer, 824);
        return true;
      }
      break;
    default :
      break;
    }
    break;
  case 24 :
    switch (name[0]) {
    case 'C' :
      if (strncmp(name + 1, "lockwiseContourIntegral", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8754);
        return true;
      }
      break;
    case 'D' :
      if (strncmp(name + 1, "oubleLongLeftRightArrow", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 10234);
        return true;
      }
      break;
    default :
      break;
    }
    break;
  case 31 :
    switch (name[0]) {
    case 'C' :
      if (strncmp(name + 1, "ounterClockwiseContourIntegral", length - 1) == 0) {
        grn_text_code_point(ctx, buffer, 8755);
        return true;
      }
      break;
    default :
      break;
    }
    break;
  default :
    break;
  }
  return false;
}
