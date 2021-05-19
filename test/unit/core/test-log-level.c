/*
  Copyright (C) 2015  Kouhei Sutou <kou@clear-code.com>

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

#include <groonga.h>

#include <gcutter.h>
#include <glib/gstdio.h>

#include "../lib/grn-assertions.h"

void data_to_string(void);
void test_to_string(gconstpointer data);
void data_parse_success(void);
void test_parse_success(gconstpointer data);
void data_parse_failure(void);
void test_parse_failure(gconstpointer data);

void
data_to_string(void)
{
#define ADD_DATUM(expected, level)                                      \
  gcut_add_datum(#expected,                                             \
                 "expected", G_TYPE_STRING, expected,                   \
                 "level", G_TYPE_INT, level,                            \
                 NULL)

  ADD_DATUM("none",      GRN_LOG_NONE);
  ADD_DATUM("emergency", GRN_LOG_EMERG);
  ADD_DATUM("alert",     GRN_LOG_ALERT);
  ADD_DATUM("critical",  GRN_LOG_CRIT);
  ADD_DATUM("error",     GRN_LOG_ERROR);
  ADD_DATUM("warning",   GRN_LOG_WARNING);
  ADD_DATUM("notice",    GRN_LOG_NOTICE);
  ADD_DATUM("info",      GRN_LOG_INFO);
  ADD_DATUM("debug",     GRN_LOG_DEBUG);
  ADD_DATUM("dump",      GRN_LOG_DUMP);
  ADD_DATUM("unknown",   GRN_LOG_DUMP + 1);

#undef ADD_DATUM
}

void
test_to_string(gconstpointer data)
{
  const gchar *expected;
  grn_log_level level;

  expected = gcut_data_get_string(data, "expected");
  level = gcut_data_get_int(data, "level");
  cut_assert_equal_string(expected,
                          grn_log_level_to_string(level));
}

void
data_parse_success(void)
{
#define ADD_DATUM(label, expected, input)                               \
  gcut_add_datum(label,                                                 \
                 "expected", G_TYPE_INT, expected,                      \
                 "input", G_TYPE_STRING, input,                         \
                 NULL)

  ADD_DATUM("none",      GRN_LOG_NONE,   " ");
  ADD_DATUM("none",      GRN_LOG_NONE,   "none");
  ADD_DATUM("none",      GRN_LOG_NONE,   "NONE");
  ADD_DATUM("emergency", GRN_LOG_EMERG,  "E");
  ADD_DATUM("emergency", GRN_LOG_EMERG,  "emerg");
  ADD_DATUM("emergency", GRN_LOG_EMERG,  "emergency");
  ADD_DATUM("emergency", GRN_LOG_EMERG,  "EMERG");
  ADD_DATUM("emergency", GRN_LOG_EMERG,  "EMERGENCY");
  ADD_DATUM("alert",     GRN_LOG_ALERT,  "A");
  ADD_DATUM("alert",     GRN_LOG_ALERT,  "alert");
  ADD_DATUM("alert",     GRN_LOG_ALERT,  "ALERT");
  ADD_DATUM("critical",  GRN_LOG_CRIT,   "C");
  ADD_DATUM("critical",  GRN_LOG_CRIT,   "crit");
  ADD_DATUM("critical",  GRN_LOG_CRIT,   "critical");
  ADD_DATUM("critical",  GRN_LOG_CRIT,   "CRIT");
  ADD_DATUM("critical",  GRN_LOG_CRIT,   "CRITICAL");
  ADD_DATUM("error",     GRN_LOG_ERROR,  "e");
  ADD_DATUM("error",     GRN_LOG_ERROR,  "error");
  ADD_DATUM("error",     GRN_LOG_ERROR,  "ERROR");
  ADD_DATUM("warning",   GRN_LOG_WARNING, "w");
  ADD_DATUM("warning",   GRN_LOG_WARNING, "warn");
  ADD_DATUM("warning",   GRN_LOG_WARNING, "warning");
  ADD_DATUM("warning",   GRN_LOG_WARNING, "WARN");
  ADD_DATUM("warning",   GRN_LOG_WARNING, "WARNING");
  ADD_DATUM("notice",    GRN_LOG_NOTICE,  "n");
  ADD_DATUM("notice",    GRN_LOG_NOTICE,  "notice");
  ADD_DATUM("notice",    GRN_LOG_NOTICE,  "NOTICE");
  ADD_DATUM("info",      GRN_LOG_INFO,    "i");
  ADD_DATUM("info",      GRN_LOG_INFO,    "info");
  ADD_DATUM("info",      GRN_LOG_INFO,    "INFO");
  ADD_DATUM("debug",     GRN_LOG_DEBUG,   "d");
  ADD_DATUM("debug",     GRN_LOG_DEBUG,   "debug");
  ADD_DATUM("debug",     GRN_LOG_DEBUG,   "DEBUG");
  ADD_DATUM("dump",      GRN_LOG_DUMP,    "-");
  ADD_DATUM("dump",      GRN_LOG_DUMP,    "dump");
  ADD_DATUM("dump",      GRN_LOG_DUMP,    "DUMP");

#undef ADD_DATUM
}

void
test_parse_success(gconstpointer data)
{
  grn_log_level expected;
  const gchar *input;
  grn_log_level level;

  expected = gcut_data_get_int(data, "expected");
  input = gcut_data_get_string(data, "input");
  cut_assert_true(grn_log_level_parse(input, &level));
  cut_assert_equal_int(expected, level);
}

void
data_parse_failure(void)
{
#define ADD_DATUM(label, input)                                         \
  gcut_add_datum(label,                                                 \
                 "input", G_TYPE_STRING, input,                         \
                 NULL)

  ADD_DATUM("one character", "X");
  ADD_DATUM("unknown name",  "unknown");

#undef ADD_DATUM
}

void
test_parse_failure(gconstpointer data)
{
  const gchar *input;
  grn_log_level level;

  input = gcut_data_get_string(data, "input");
  cut_assert_false(grn_log_level_parse(input, &level));
}
