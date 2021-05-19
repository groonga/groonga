/*
  Copyright(C) 2010-2018 Brazil

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

#include <stdio.h>
#include <getopt.h>
#include <unistd.h>
#include <string.h>

#define U_HIDE_DEPRECATED_API

#include <unicode/utf.h>
#include <unicode/uchar.h>
#include <unicode/unorm2.h>
#include <unicode/ustring.h>

#define MAX_UNICODE 0x110000
#define BUF_SIZE 0x100

static int
ucs2utf(unsigned int i, unsigned char *buf)
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

void
blockcode(void)
{
  UChar32 ch;
  unsigned char *p, src[7];
  UBlockCode code, lc = -1;
  for (ch = 1; ch < MAX_UNICODE; ch++) {
    if (!U_IS_UNICODE_CHAR(ch)) { continue; }
    code = ublock_getCode(ch);
    if (code != lc) {
      ucs2utf(ch, src);
      for (p = src; *p; p++) {
        printf("%x:", *p);
      }
      printf("\t%04x\t%d\n", ch, code);
    }
    lc = code;
  }
}

int
normalize(const char *str, char *res, const UNormalizer2 *normalizer)
{
  UErrorCode rc;
  int32_t ulen, nlen;
  UChar ubuf[BUF_SIZE], nbuf[BUF_SIZE];

  rc = U_ZERO_ERROR;
  u_strFromUTF8(ubuf, BUF_SIZE, &ulen, str, -1, &rc);
  if (rc != U_ZERO_ERROR /*&& rc != U_STRING_NOT_TERMINATED_WARNING*/) {
    return -1;
  }
  rc = U_ZERO_ERROR;
  nlen = unorm2_normalize(normalizer, ubuf, ulen, nbuf, BUF_SIZE, &rc);
  if (rc != U_ZERO_ERROR /*&& rc != U_STRING_NOT_TERMINATED_WARNING*/) {
    return -1;
  }
  rc = U_ZERO_ERROR;
  u_strToUTF8(res, BUF_SIZE, NULL, nbuf, nlen, &rc);
  if (rc != U_ZERO_ERROR /*&& rc != U_BUFFER_OVERFLOW_ERROR*/) {
    return -1;
  }
  return 0;
}

void
dump(const UNormalizer2 *normalizer)
{
  UChar32 ch;
  char str[7], norm[BUF_SIZE];
  for (ch = 1; ch < MAX_UNICODE; ch++) {
    if (!U_IS_UNICODE_CHAR(ch)) { continue; }
    ucs2utf(ch, (unsigned char *)str);
    if (normalize(str, norm, normalizer)) {
      printf("ch=%04x error occure\n", ch);
      continue;
    }
    if (strcmp(norm, str)) {
      printf("%04x\t%s\t%s\n", ch, str, norm);
    }
  }
}

void
ccdump(void)
{
  UChar32 ch;
  char str[7], nfd[BUF_SIZE], nfc[BUF_SIZE];
  for (ch = 1; ch < MAX_UNICODE; ch++) {
    UErrorCode error_code = U_ZERO_ERROR;
    if (!U_IS_UNICODE_CHAR(ch)) { continue; }
    ucs2utf(ch, (unsigned char *)str);
    if (normalize(str, nfd, unorm2_getNFDInstance(&error_code))) {
      printf("ch=%04x error occure\n", ch);
      continue;
    }
    if (normalize(str, nfc, unorm2_getNFCInstance(&error_code))) {
      printf("ch=%04x error occure\n", ch);
      continue;
    }
    if (strcmp(nfd, nfc)) {
      printf("%04x\t%s\t%s\n", ch, nfd, nfc);
    }
  }
}

enum {
  CTYPE_NULL = 0,
  CTYPE_ALPHA,
  CTYPE_DIGIT,
  CTYPE_SYMBOL,
  CTYPE_HIRAGANA,
  CTYPE_KATAKANA,
  CTYPE_KANJI,
  CTYPE_OTHERS,
  CTYPE_EMOJI
};

static const char *CTYPES[] = {
  "GRN_CHAR_NULL",
  "GRN_CHAR_ALPHA",
  "GRN_CHAR_DIGIT",
  "GRN_CHAR_SYMBOL",
  "GRN_CHAR_HIRAGANA",
  "GRN_CHAR_KATAKANA",
  "GRN_CHAR_KANJI",
  "GRN_CHAR_OTHERS",
  "GRN_CHAR_EMOJI"
};

void
gcdump(void)
{
  UChar32 ch;
  unsigned char *p, src[7];
  int last_ctype = CTYPE_NULL;

  for (ch = 1; ch < MAX_UNICODE; ch++) {
    int ctype = CTYPE_NULL;
    UBlockCode code;

    if (!U_IS_UNICODE_CHAR(ch)) { continue; }
    code = ublock_getCode(ch);
    switch (code) {
    case UBLOCK_NO_BLOCK : /*[none]*/
    case UBLOCK_BASIC_LATIN : /*[0000]*/
    case UBLOCK_LATIN_1_SUPPLEMENT : /*[0080]*/
    case UBLOCK_LATIN_EXTENDED_A : /*[0100]*/
    case UBLOCK_LATIN_EXTENDED_B : /*[0180]*/
    case UBLOCK_IPA_EXTENSIONS : /*[0250]*/
    case UBLOCK_SPACING_MODIFIER_LETTERS : /*[02B0]*/
    case UBLOCK_COMBINING_DIACRITICAL_MARKS : /*[0300]*/
    case UBLOCK_GREEK : /*[0370]*/
    case UBLOCK_CYRILLIC : /*[0400]*/
    case UBLOCK_ARMENIAN : /*[0530]*/
    case UBLOCK_HEBREW : /*[0590]*/
    case UBLOCK_ARABIC : /*[0600]*/
    case UBLOCK_SYRIAC : /*[0700]*/
    case UBLOCK_THAANA : /*[0780]*/
    case UBLOCK_DEVANAGARI : /*[0900]*/
    case UBLOCK_BENGALI : /*[0980]*/
    case UBLOCK_GURMUKHI : /*[0A00]*/
    case UBLOCK_GUJARATI : /*[0A80]*/
    case UBLOCK_ORIYA : /*[0B00]*/
    case UBLOCK_TAMIL : /*[0B80]*/
    case UBLOCK_TELUGU : /*[0C00]*/
    case UBLOCK_KANNADA : /*[0C80]*/
    case UBLOCK_MALAYALAM : /*[0D00]*/
    case UBLOCK_SINHALA : /*[0D80]*/
    case UBLOCK_THAI : /*[0E00]*/
    case UBLOCK_LAO : /*[0E80]*/
    case UBLOCK_TIBETAN : /*[0F00]*/
    case UBLOCK_MYANMAR : /*[1000]*/
    case UBLOCK_GEORGIAN : /*[10A0]*/
      break;
    case UBLOCK_HANGUL_JAMO : /*[1100]*/
      /* TODO : incompatible */
      ctype = CTYPE_KANJI;
      break;
    case UBLOCK_ETHIOPIC : /*[1200]*/
    case UBLOCK_CHEROKEE : /*[13A0]*/
    case UBLOCK_UNIFIED_CANADIAN_ABORIGINAL_SYLLABICS : /*[1400]*/
    case UBLOCK_OGHAM : /*[1680]*/
    case UBLOCK_RUNIC : /*[16A0]*/
    case UBLOCK_KHMER : /*[1780]*/
    case UBLOCK_MONGOLIAN : /*[1800]*/
    case UBLOCK_LATIN_EXTENDED_ADDITIONAL : /*[1E00]*/
    case UBLOCK_GREEK_EXTENDED : /*[1F00]*/
    case UBLOCK_GENERAL_PUNCTUATION : /*[2000]*/
    case UBLOCK_SUPERSCRIPTS_AND_SUBSCRIPTS : /*[2070]*/
    case UBLOCK_CURRENCY_SYMBOLS : /*[20A0]*/
    case UBLOCK_COMBINING_MARKS_FOR_SYMBOLS : /*[20D0]*/
    case UBLOCK_LETTERLIKE_SYMBOLS : /*[2100]*/
    case UBLOCK_NUMBER_FORMS : /*[2150]*/
      break;
    case UBLOCK_ARROWS : /*[2190]*/
    case UBLOCK_MATHEMATICAL_OPERATORS : /*[2200]*/
    case UBLOCK_MISCELLANEOUS_TECHNICAL : /*[2300]*/
    case UBLOCK_CONTROL_PICTURES : /*[2400]*/
      /* TODO: ? ctypes = CTYPE_SYMBOL; */
      break;
    case UBLOCK_OPTICAL_CHARACTER_RECOGNITION : /*[2440]*/
    case UBLOCK_ENCLOSED_ALPHANUMERICS : /*[2460]*/
    case UBLOCK_BOX_DRAWING : /*[2500]*/
    case UBLOCK_BLOCK_ELEMENTS : /*[2580]*/
    case UBLOCK_GEOMETRIC_SHAPES : /*[25A0]*/
    case UBLOCK_MISCELLANEOUS_SYMBOLS : /*[2600]*/
    case UBLOCK_DINGBATS : /*[2700]*/
      /* TODO: ? ctypes = CTYPE_SYMBOL; */
      break;
    case UBLOCK_BRAILLE_PATTERNS : /*[2800]*/
      break;
    case UBLOCK_CJK_RADICALS_SUPPLEMENT : /*[2E80]*/
    case UBLOCK_KANGXI_RADICALS : /*[2F00]*/
      ctype = CTYPE_KANJI;
      break;
    case UBLOCK_IDEOGRAPHIC_DESCRIPTION_CHARACTERS : /*[2FF0]*/
      break;
    case UBLOCK_CJK_SYMBOLS_AND_PUNCTUATION : /*[3000]*/
      /* symbols ex. JIS mark */
      ctype = CTYPE_SYMBOL;
      break;
    case UBLOCK_HIRAGANA : /*[3040]*/
      ctype = CTYPE_HIRAGANA;
      break;
    case UBLOCK_KATAKANA : /*[30A0]*/
      ctype = CTYPE_KATAKANA;
      break;
    case UBLOCK_BOPOMOFO : /*[3100]*/
    case UBLOCK_HANGUL_COMPATIBILITY_JAMO : /*[3130]*/
    case UBLOCK_KANBUN : /*[3190]*/
      /* kaeri ten used in kanbun ex. re-ten */
    case UBLOCK_BOPOMOFO_EXTENDED : /*[31A0]*/
      ctype = CTYPE_KANJI;
      break;
    case UBLOCK_ENCLOSED_CJK_LETTERS_AND_MONTHS : /*[3200]*/
      /* ex. (kabu) */
      ctype = CTYPE_SYMBOL;
      break;
    case UBLOCK_CJK_COMPATIBILITY : /*[3300]*/
      /* ex. ton doll */
      ctype = CTYPE_SYMBOL;
      break;
    case UBLOCK_CJK_UNIFIED_IDEOGRAPHS_EXTENSION_A : /*[3400]*/
    case UBLOCK_CJK_UNIFIED_IDEOGRAPHS : /*[4E00]*/
    case UBLOCK_YI_SYLLABLES : /*[A000]*/
    case UBLOCK_YI_RADICALS : /*[A490]*/
    case UBLOCK_HANGUL_SYLLABLES : /*[AC00]*/
      ctype = CTYPE_KANJI;
      break;
    case UBLOCK_HIGH_SURROGATES : /*[D800]*/
    case UBLOCK_HIGH_PRIVATE_USE_SURROGATES : /*[DB80]*/
    case UBLOCK_LOW_SURROGATES : /*[DC00]*/
    case UBLOCK_PRIVATE_USE_AREA : /*[E000]*/
      break;
    case UBLOCK_CJK_COMPATIBILITY_IDEOGRAPHS : /*[F900]*/
      ctype = CTYPE_KANJI;
      break;
    case UBLOCK_ALPHABETIC_PRESENTATION_FORMS : /*[FB00]*/
    case UBLOCK_ARABIC_PRESENTATION_FORMS_A : /*[FB50]*/
    case UBLOCK_COMBINING_HALF_MARKS : /*[FE20]*/
      break;
    case UBLOCK_CJK_COMPATIBILITY_FORMS : /*[FE30]*/
      /* ex. tategaki kagi-kakko */
      ctype = CTYPE_SYMBOL;
      break;
    case UBLOCK_SMALL_FORM_VARIANTS : /*[FE50]*/
    case UBLOCK_ARABIC_PRESENTATION_FORMS_B : /*[FE70]*/
    case UBLOCK_SPECIALS : /*[FFF0]*/
    case UBLOCK_HALFWIDTH_AND_FULLWIDTH_FORMS : /*[FF00]*/
    case UBLOCK_OLD_ITALIC : /*[10300]*/
    case UBLOCK_GOTHIC : /*[10330]*/
    case UBLOCK_DESERET : /*[10400]*/
      break;
    case UBLOCK_BYZANTINE_MUSICAL_SYMBOLS : /*[1D000]*/
    case UBLOCK_MUSICAL_SYMBOLS : /*[1D1000]*/
    case UBLOCK_MATHEMATICAL_ALPHANUMERIC_SYMBOLS : /*[1D400]*/
      /* TODO: ? ctype = CTYPE_SYMBOL; */
      break;
    case UBLOCK_CJK_UNIFIED_IDEOGRAPHS_EXTENSION_B : /*[20000]*/
    case UBLOCK_CJK_COMPATIBILITY_IDEOGRAPHS_SUPPLEMENT : /*[2F800]*/
      ctype = CTYPE_KANJI;
      break;
    case UBLOCK_TAGS : /*[E0000]*/
    case UBLOCK_CYRILLIC_SUPPLEMENT : /*[0500]*/
    case UBLOCK_TAGALOG : /*[1700]*/
    case UBLOCK_HANUNOO : /*[1720]*/
    case UBLOCK_BUHID : /*[1740]*/
    case UBLOCK_TAGBANWA : /*[1760]*/
      break;
    case UBLOCK_MISCELLANEOUS_MATHEMATICAL_SYMBOLS_A : /*[27C0]*/
    case UBLOCK_SUPPLEMENTAL_ARROWS_A : /*[27F0]*/
    case UBLOCK_SUPPLEMENTAL_ARROWS_B : /*[2900]*/
    case UBLOCK_MISCELLANEOUS_MATHEMATICAL_SYMBOLS_B : /*[2980]*/
    case UBLOCK_SUPPLEMENTAL_MATHEMATICAL_OPERATORS : /*[2A00]*/
      /* TODO: ? ctype = CTYPE_SYMBOL; */
      break;
    case UBLOCK_KATAKANA_PHONETIC_EXTENSIONS : /*[31F0]*/
      ctype = CTYPE_KATAKANA;
      break;
    case UBLOCK_VARIATION_SELECTORS : /*[FE00]*/
    case UBLOCK_SUPPLEMENTARY_PRIVATE_USE_AREA_A : /*[F0000]*/
    case UBLOCK_SUPPLEMENTARY_PRIVATE_USE_AREA_B : /*[100000]*/
      break;
    case UBLOCK_LIMBU : /*[1900]*/
    case UBLOCK_TAI_LE : /*[1950]*/
      break;
    case UBLOCK_KHMER_SYMBOLS : /*[19E0]*/
      /* TODO: ? ctype = CTYPE_SYMBOL; */
      break;
    case UBLOCK_PHONETIC_EXTENSIONS : /*[1D00]*/
      /* TODO: ? ctype = CTYPE_ALPHA; */
      break;
    case UBLOCK_MISCELLANEOUS_SYMBOLS_AND_ARROWS : /*[2B00]*/
      /* TODO: ? ctype = CTYPE_SYMBOL; */
      break;
    case UBLOCK_YIJING_HEXAGRAM_SYMBOLS : /*[4DC0]*/
      ctype = CTYPE_SYMBOL;
      break;
    case UBLOCK_LINEAR_B_SYLLABARY : /*[10000]*/
    case UBLOCK_LINEAR_B_IDEOGRAMS : /*[10080]*/
    case UBLOCK_AEGEAN_NUMBERS : /*[10100]*/
    case UBLOCK_UGARITIC : /*[10380]*/
    case UBLOCK_SHAVIAN : /*[10450]*/
    case UBLOCK_OSMANYA : /*[10480]*/
    case UBLOCK_CYPRIOT_SYLLABARY : /*[10800]*/
    case UBLOCK_TAI_XUAN_JING_SYMBOLS : /*[1D300]*/
    case UBLOCK_VARIATION_SELECTORS_SUPPLEMENT : /*[E0100]*/
    case UBLOCK_ANCIENT_GREEK_MUSICAL_NOTATION : /*[1D200]*/
    case UBLOCK_ANCIENT_GREEK_NUMBERS : /*[10140]*/
    case UBLOCK_ARABIC_SUPPLEMENT : /*[0750]*/
    case UBLOCK_BUGINESE : /*[1A00]*/
      break;
    case UBLOCK_CJK_STROKES : /*[31C0]*/
      ctype = CTYPE_KANJI;
      break;
    case UBLOCK_COMBINING_DIACRITICAL_MARKS_SUPPLEMENT : /*[1DC0]*/
    case UBLOCK_COPTIC : /*[2C80]*/
    case UBLOCK_ETHIOPIC_EXTENDED : /*[2D80]*/
    case UBLOCK_ETHIOPIC_SUPPLEMENT : /*[1380]*/
    case UBLOCK_GEORGIAN_SUPPLEMENT : /*[2D00]*/
    case UBLOCK_GLAGOLITIC : /*[2C00]*/
    case UBLOCK_KHAROSHTHI : /*[10A00]*/
    case UBLOCK_MODIFIER_TONE_LETTERS : /*[A700]*/
    case UBLOCK_NEW_TAI_LUE : /*[1980]*/
    case UBLOCK_OLD_PERSIAN : /*[103A0]*/
    case UBLOCK_PHONETIC_EXTENSIONS_SUPPLEMENT : /*[1D80]*/
    case UBLOCK_SUPPLEMENTAL_PUNCTUATION : /*[2E00]*/
    case UBLOCK_SYLOTI_NAGRI : /*[A800]*/
    case UBLOCK_TIFINAGH : /*[2D30]*/
    case UBLOCK_VERTICAL_FORMS : /*[FE10]*/
    case UBLOCK_NKO : /*[07C0]*/
    case UBLOCK_BALINESE : /*[1B00]*/
    case UBLOCK_LATIN_EXTENDED_C : /*[2C60]*/
    case UBLOCK_LATIN_EXTENDED_D : /*[A720]*/
    case UBLOCK_PHAGS_PA : /*[A840]*/
    case UBLOCK_PHOENICIAN : /*[10900]*/
    case UBLOCK_CUNEIFORM : /*[12000]*/
    case UBLOCK_CUNEIFORM_NUMBERS_AND_PUNCTUATION : /*[12400]*/
    case UBLOCK_COUNTING_ROD_NUMERALS : /*[1D360]*/
    case UBLOCK_SUNDANESE : /*[1B80]*/
    case UBLOCK_LEPCHA : /*[1C00]*/
    case UBLOCK_OL_CHIKI : /*[1C50]*/
    case UBLOCK_CYRILLIC_EXTENDED_A : /*[2DE0]*/
    case UBLOCK_VAI : /*[A500]*/
    case UBLOCK_CYRILLIC_EXTENDED_B : /*[A640]*/
    case UBLOCK_SAURASHTRA : /*[A880]*/
    case UBLOCK_KAYAH_LI : /*[A900]*/
    case UBLOCK_REJANG : /*[A930]*/
    case UBLOCK_CHAM : /*[AA00]*/
    case UBLOCK_ANCIENT_SYMBOLS : /*[10190]*/
    case UBLOCK_PHAISTOS_DISC : /*[101D0]*/
    case UBLOCK_LYCIAN : /*[10280]*/
    case UBLOCK_CARIAN : /*[102A0]*/
    case UBLOCK_LYDIAN : /*[10920]*/
    case UBLOCK_MAHJONG_TILES : /*[1F000]*/
    case UBLOCK_DOMINO_TILES : /*[1F030]*/
    case UBLOCK_SAMARITAN : /*[0800]*/
    case UBLOCK_UNIFIED_CANADIAN_ABORIGINAL_SYLLABICS_EXTENDED : /*[18B0]*/
    case UBLOCK_TAI_THAM : /*[1A20]*/
    case UBLOCK_VEDIC_EXTENSIONS : /*[1CD0]*/
    case UBLOCK_LISU : /*[A4D0]*/
    case UBLOCK_BAMUM : /*[A6A0]*/
    case UBLOCK_COMMON_INDIC_NUMBER_FORMS : /*[A830]*/
    case UBLOCK_DEVANAGARI_EXTENDED : /*[A8E0]*/
      break;
    case UBLOCK_HANGUL_JAMO_EXTENDED_A : /*[A960]*/
      ctype = CTYPE_KANJI;
      break;
    case UBLOCK_JAVANESE : /*[A980]*/
    case UBLOCK_MYANMAR_EXTENDED_A : /*[AA60]*/
    case UBLOCK_TAI_VIET : /*[AA80]*/
    case UBLOCK_MEETEI_MAYEK : /*[ABC0]*/
      break;
    case UBLOCK_HANGUL_JAMO_EXTENDED_B : /*[D7B0]*/
      ctype = CTYPE_KANJI;
      break;
    case UBLOCK_IMPERIAL_ARAMAIC : /*[10840]*/
    case UBLOCK_OLD_SOUTH_ARABIAN : /*[10A60]*/
    case UBLOCK_AVESTAN : /*[10B00]*/
    case UBLOCK_INSCRIPTIONAL_PARTHIAN : /*[10B40]*/
    case UBLOCK_INSCRIPTIONAL_PAHLAVI : /*[10B60]*/
    case UBLOCK_OLD_TURKIC : /*[10C00]*/
    case UBLOCK_RUMI_NUMERAL_SYMBOLS : /*[10E60]*/
    case UBLOCK_KAITHI : /*[11080]*/
    case UBLOCK_EGYPTIAN_HIEROGLYPHS : /*[13000]*/
    case UBLOCK_ENCLOSED_ALPHANUMERIC_SUPPLEMENT : /*[1F100]*/
    case UBLOCK_ENCLOSED_IDEOGRAPHIC_SUPPLEMENT : /*[1F200]*/
      break;
    case UBLOCK_CJK_UNIFIED_IDEOGRAPHS_EXTENSION_C : /*[2A700]*/
      ctype = CTYPE_KANJI;
      break;
    case UBLOCK_MANDAIC : /*[0840]*/
    case UBLOCK_BATAK : /*[1BC0]*/
    case UBLOCK_ETHIOPIC_EXTENDED_A : /*[AB00]*/
    case UBLOCK_BRAHMI : /*[11000]*/
    case UBLOCK_BAMUM_SUPPLEMENT : /*[16800]*/
      break;
    case UBLOCK_KANA_SUPPLEMENT : /*[1B000]*/
      if (ch == 0x1B000) { /* KATAKANA LETTER ARCHAIC E */
        ctype = CTYPE_KATAKANA;
      } else {
        ctype = CTYPE_HIRAGANA;
      }
      break;
    case UBLOCK_PLAYING_CARDS : /*[1F0A0]*/
      break;
    case UBLOCK_MISCELLANEOUS_SYMBOLS_AND_PICTOGRAPHS : /*[1F300]*/
    case UBLOCK_EMOTICONS : /*[1F600]*/
    case UBLOCK_TRANSPORT_AND_MAP_SYMBOLS : /*[1F680]*/
      ctype = CTYPE_EMOJI;
      break;
    case UBLOCK_ALCHEMICAL_SYMBOLS : /*[1F700]*/
      break;
    case UBLOCK_CJK_UNIFIED_IDEOGRAPHS_EXTENSION_D : /*[2B740]*/
      ctype = CTYPE_KANJI;
      break;
    case UBLOCK_ARABIC_EXTENDED_A : /*[08A0]*/
    case UBLOCK_ARABIC_MATHEMATICAL_ALPHABETIC_SYMBOLS : /*[1EE00]*/
    case UBLOCK_CHAKMA : /*[11100]*/
    case UBLOCK_MEETEI_MAYEK_EXTENSIONS : /*[AAE0]*/
    case UBLOCK_MEROITIC_CURSIVE : /*[109A0]*/
    case UBLOCK_MEROITIC_HIEROGLYPHS : /*[10980]*/
    case UBLOCK_MIAO : /*[16F00]*/
    case UBLOCK_SHARADA : /*[11180]*/
    case UBLOCK_SORA_SOMPENG : /*[110D0]*/
    case UBLOCK_SUNDANESE_SUPPLEMENT : /*[1CC0]*/
    case UBLOCK_TAKRI : /*[11680]*/
    case UBLOCK_BASSA_VAH : /*[16AD0]*/
    case UBLOCK_CAUCASIAN_ALBANIAN : /*[10530]*/
    case UBLOCK_COPTIC_EPACT_NUMBERS : /*[102E0]*/
    case UBLOCK_COMBINING_DIACRITICAL_MARKS_EXTENDED : /*[1AB0]*/
    case UBLOCK_DUPLOYAN : /*[1BC00]*/
    case UBLOCK_ELBASAN : /*[10500]*/
    case UBLOCK_GEOMETRIC_SHAPES_EXTENDED : /*[1F780]*/
    case UBLOCK_GRANTHA : /*[11300]*/
    case UBLOCK_KHOJKI : /*[11200]*/
    case UBLOCK_KHUDAWADI : /*[112B0]*/
    case UBLOCK_LATIN_EXTENDED_E : /*[AB30]*/
    case UBLOCK_LINEAR_A : /*[10600]*/
    case UBLOCK_MAHAJANI : /*[11150]*/
    case UBLOCK_MANICHAEAN : /*[10AC0]*/
    case UBLOCK_MENDE_KIKAKUI : /*[1E800]*/
    case UBLOCK_MODI : /*[11600]*/
    case UBLOCK_MRO : /*[16A40]*/
    case UBLOCK_MYANMAR_EXTENDED_B : /*[A9E0]*/
    case UBLOCK_NABATAEAN : /*[10880]*/
    case UBLOCK_OLD_NORTH_ARABIAN : /*[10A80]*/
    case UBLOCK_OLD_PERMIC : /*[10350]*/
    case UBLOCK_ORNAMENTAL_DINGBATS : /*[1F650]*/
    case UBLOCK_PAHAWH_HMONG : /*[16B00]*/
    case UBLOCK_PALMYRENE : /*[10860]*/
    case UBLOCK_PAU_CIN_HAU : /*[11AC0]*/
    case UBLOCK_PSALTER_PAHLAVI : /*[10B80]*/
    case UBLOCK_SHORTHAND_FORMAT_CONTROLS : /*[1BCA0]*/
    case UBLOCK_SIDDHAM : /*[11580]*/
    case UBLOCK_SINHALA_ARCHAIC_NUMBERS : /*[111E0]*/
    case UBLOCK_SUPPLEMENTAL_ARROWS_C : /*[1F800]*/
    case UBLOCK_TIRHUTA : /*[11480]*/
    case UBLOCK_WARANG_CITI : /*[118A0]*/
    case UBLOCK_AHOM : /*[11700]*/
    case UBLOCK_ANATOLIAN_HIEROGLYPHS : /*[14400]*/
    case UBLOCK_CHEROKEE_SUPPLEMENT : /*[AB70]*/
      break;
    case UBLOCK_CJK_UNIFIED_IDEOGRAPHS_EXTENSION_E : /*[2B820]*/
      ctype = CTYPE_KANJI;
      break;
    case UBLOCK_EARLY_DYNASTIC_CUNEIFORM : /*[12480]*/
    case UBLOCK_HATRAN : /*[108E0]*/
    case UBLOCK_MULTANI : /*[11280]*/
    case UBLOCK_OLD_HUNGARIAN : /*[10C80]*/
    case UBLOCK_SUPPLEMENTAL_SYMBOLS_AND_PICTOGRAPHS : /*[1F900]*/
    case UBLOCK_SUTTON_SIGNWRITING : /*[1D800]*/
    case UBLOCK_ADLAM : /*[1E900]*/
    case UBLOCK_BHAIKSUKI : /*[11C00]*/
    case UBLOCK_CYRILLIC_EXTENDED_C : /*[1C80]*/
    case UBLOCK_GLAGOLITIC_SUPPLEMENT : /*[1E000]*/
    case UBLOCK_IDEOGRAPHIC_SYMBOLS_AND_PUNCTUATION : /*[16FE0]*/
    case UBLOCK_MARCHEN : /*[11C70]*/
    case UBLOCK_MONGOLIAN_SUPPLEMENT : /*[11660]*/
    case UBLOCK_NEWA : /*[11400]*/
    case UBLOCK_OSAGE : /*[104B0]*/
    case UBLOCK_TANGUT : /*[17000]*/
    case UBLOCK_TANGUT_COMPONENTS : /*[18800]*/
      break;
    case UBLOCK_CJK_UNIFIED_IDEOGRAPHS_EXTENSION_F : /*[2CEB0]*/
      ctype = CTYPE_KANJI;
      break;
    case UBLOCK_KANA_EXTENDED_A : /*[1B100]*/
      ctype = CTYPE_HIRAGANA;
      break;
    case UBLOCK_MASARAM_GONDI : /*[11D00]*/
    case UBLOCK_NUSHU : /*[1B170]*/
    case UBLOCK_SOYOMBO : /*[11A50]*/
    case UBLOCK_SYRIAC_SUPPLEMENT : /*[0860]*/
    case UBLOCK_ZANABAZAR_SQUARE : /*[11A00]*/
      break;
    case UBLOCK_INVALID_CODE :
      break;
    }

    if (ctype == CTYPE_NULL) {
      UCharCategory category;
      category = u_charType(ch);
      switch (category) {
      case U_GENERAL_OTHER_TYPES:
        break;
      case U_UPPERCASE_LETTER:
      case U_LOWERCASE_LETTER:
      case U_TITLECASE_LETTER:
      case U_MODIFIER_LETTER:
      case U_OTHER_LETTER:
        ctype = CTYPE_ALPHA;
        break;
      case U_NON_SPACING_MARK:
      case U_ENCLOSING_MARK:
      case U_COMBINING_SPACING_MARK:
        break;
      case U_DECIMAL_DIGIT_NUMBER:
      case U_LETTER_NUMBER:
      case U_OTHER_NUMBER:
        ctype = CTYPE_DIGIT;
        break;
      case U_SPACE_SEPARATOR:
      case U_LINE_SEPARATOR:
      case U_PARAGRAPH_SEPARATOR:
        break;
      case U_CONTROL_CHAR:
      case U_FORMAT_CHAR:
      case U_PRIVATE_USE_CHAR:
      case U_SURROGATE:
        break;
      case U_DASH_PUNCTUATION:
      case U_START_PUNCTUATION:
      case U_END_PUNCTUATION:
      case U_CONNECTOR_PUNCTUATION:
      case U_OTHER_PUNCTUATION:
      case U_MATH_SYMBOL:
      case U_CURRENCY_SYMBOL:
      case U_MODIFIER_SYMBOL:
      case U_OTHER_SYMBOL:
        ctype = CTYPE_SYMBOL;
        break;
      case U_INITIAL_PUNCTUATION:
      case U_FINAL_PUNCTUATION:
        break;
      case U_CHAR_CATEGORY_COUNT:
        break;
      }
    }

    if (ctype == CTYPE_SYMBOL && ch >= 0x80) {
      if (u_hasBinaryProperty(ch, UCHAR_EMOJI) ||
          u_hasBinaryProperty(ch, UCHAR_EMOJI_PRESENTATION) ||
          u_hasBinaryProperty(ch, UCHAR_EMOJI_MODIFIER) ||
          u_hasBinaryProperty(ch, UCHAR_EMOJI_COMPONENT)) {
        ctype = CTYPE_EMOJI;
      }
    }

    if (ctype == CTYPE_NULL) {
      ctype = CTYPE_OTHERS;
    }

    if (ctype != last_ctype) {
      ucs2utf(ch, src);
      for (p = src; *p; p++) {
        printf("%x:", *p);
      }
      printf("\t%04x\t%s\n", ch, CTYPES[ctype]);
    }
    last_ctype = ctype;
  }
}

struct option options[] = {
  {"bc", 0, NULL, 'b'},
  {"nfd", 0, NULL, 'd'},
  {"nfkd", 0, NULL, 'D'},
  {"nfc", 0, NULL, 'c'},
  {"nfkc", 0, NULL, 'C'},
  {"cc", 0, NULL, 'o'},
  {"gc", 0, NULL, 'g'},
  {"version", 0, NULL, 'v'},
};

int
main(int argc, char **argv)
{
  UErrorCode error_code = U_ZERO_ERROR;
  switch (getopt_long(argc, argv, "bdDcCogv", options, NULL)) {
  case 'b' :
    blockcode();
    break;
  case 'd' :
    dump(unorm2_getNFDInstance(&error_code));
    break;
  case 'D' :
    dump(unorm2_getNFKDInstance(&error_code));
    break;
  case 'c' :
    dump(unorm2_getNFCInstance(&error_code));
    break;
  case 'C' :
    dump(unorm2_getNFKCInstance(&error_code));
    break;
  case 'o' :
    ccdump();
    break;
  case 'g' :
    gcdump();
    break;
  case 'v' :
    printf("%s\n", U_UNICODE_VERSION);
    break;
  default :
    fputs("usage: icudump --[bc|nfd|nfkd|nfc|nfkc|cc|gc|version]\n", stderr);
    break;
  }
  return 0;
}
