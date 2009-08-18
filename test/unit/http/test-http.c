/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/*
  Copyright (C) 2009  Yuto HAYAMIZU <y.hayamizu@gmail.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License version 2.1 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <glib/gstdio.h>
#include <libsoup/soup.h>
#include <gcutter.h> /* must be included after memcached.h */

#include <unistd.h> /* for exec */
#include <sys/types.h>
#include <signal.h>

#include "../lib/grn-assertions.h"

#define GROONGA_TEST_PORT "4545"

/* globals */
static gchar *tmp_directory;

static GCutEgg *egg;

void
cut_setup(void)
{
  GError *error = NULL;

  tmp_directory = g_build_filename(grn_test_get_base_dir(), "tmp", NULL);
  cut_remove_path(tmp_directory, NULL);
  if (g_mkdir_with_parents(tmp_directory, 0700) == -1) {
    cut_assert_errno();
  }

  egg = gcut_egg_new(GROONGA, "-s",
                     "-p", GROONGA_TEST_PORT,
                     "-n",
                     cut_take_printf("%s%s%s",
                                     tmp_directory,
                                     G_DIR_SEPARATOR_S,
                                     "http.db"),
                     NULL);
  gcut_egg_hatch(egg, &error);
  gcut_assert_error(error);

  sleep(1);
}

void
cut_teardown(void)
{
  if (egg) {
    g_object_unref(egg);
  }

  cut_remove_path(tmp_directory, NULL);
}

void
test_get_root(void)
{
    SoupSession *session;
    SoupMessage *message;
    guint status;

    session = soup_session_sync_new();
    message = soup_message_new("GET", "http://localhost:" GROONGA_TEST_PORT "/");
    gcut_take_object(G_OBJECT(session));
    gcut_take_object(G_OBJECT(message));
    
    status = soup_session_send_message(session, message);

    cut_assert_equal_uint(200, status);
}
