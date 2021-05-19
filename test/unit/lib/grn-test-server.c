/*
  Copyright (C) 2009-2019  Sutou Kouhei <kou@clear-code.com>

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

#include <errno.h>

#include <gcutter.h>

#include "grn-test-server.h"
#include "grn-test-utils.h"

#define GRN_TEST_SERVER_DEFAULT_PORT 5454

#define GRN_TEST_SERVER_GET_PRIVATE(obj)                        \
  ((GrnTestServerPrivate *)                                     \
   grn_test_server_get_instance_private(GRN_TEST_SERVER(obj)))

typedef struct _GrnTestServerPrivate	GrnTestServerPrivate;
struct _GrnTestServerPrivate
{
  GCutEgg *egg;
  gchar *base_directory;
  gchar *database_path;
  gboolean custom_database_path;
  gchar *address;
  guint port;
  gchar *encoding;
  gchar *http_uri_base;
  gchar *memcached_address;
};


G_DEFINE_TYPE_WITH_PRIVATE(GrnTestServer,
                           grn_test_server,
                           G_TYPE_OBJECT)

static void dispose         (GObject               *object);

static void
grn_test_server_class_init(GrnTestServerClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS(klass);

  gobject_class->dispose = dispose;
}

static void
grn_test_server_init(GrnTestServer *factory)
{
  GrnTestServerPrivate *priv;

  priv = GRN_TEST_SERVER_GET_PRIVATE(factory);

  priv->egg = NULL;
  priv->base_directory = NULL;
  priv->database_path = NULL;
  priv->custom_database_path = FALSE;
  priv->address = g_strdup("127.0.0.1");
  priv->port = GRN_TEST_SERVER_DEFAULT_PORT;
  priv->encoding = g_strdup("utf8");
  priv->http_uri_base = NULL;
  priv->memcached_address = NULL;
}

static void
dispose(GObject *object)
{
  GrnTestServerPrivate *priv;

  priv = GRN_TEST_SERVER_GET_PRIVATE(object);

  if (priv->egg) {
    g_object_unref(priv->egg);
    priv->egg = NULL;
  }

  if (!priv->custom_database_path) {
    if (priv->base_directory)
      cut_utils_remove_path_recursive_force(priv->base_directory);
  }

  if (priv->base_directory) {
    g_free(priv->base_directory);
    priv->base_directory = NULL;
  }

  if (priv->database_path) {
    g_free(priv->database_path);
    priv->database_path = NULL;
  }

  if (priv->address) {
    g_free(priv->address);
    priv->address = NULL;
  }

  if (priv->encoding) {
    g_free(priv->encoding);
    priv->encoding = NULL;
  }

  if (priv->http_uri_base) {
    g_free(priv->http_uri_base);
    priv->http_uri_base = NULL;
  }

  if (priv->memcached_address) {
    g_free(priv->memcached_address);
    priv->memcached_address = NULL;
  }

  G_OBJECT_CLASS(grn_test_server_parent_class)->dispose(object);
}

GQuark
grn_test_server_error_quark(void)
{
  return g_quark_from_static_string("grn-test-server-error-quark");
}

GrnTestServer *
grn_test_server_new(void)
{
  return g_object_new(GRN_TYPE_TEST_SERVER, NULL);
}

static gboolean
grn_test_server_ensure_base_directory(GrnTestServer *server, GError **error)
{
  GrnTestServerPrivate *priv;

  priv = GRN_TEST_SERVER_GET_PRIVATE(server);
  if (priv->base_directory)
    return TRUE;

  priv->base_directory = g_build_filename(grn_test_get_tmp_dir(),
                                          "tmp-server", NULL);
  cut_utils_remove_path_recursive_force(priv->base_directory);
  if (g_mkdir_with_parents(priv->base_directory, 0700) == -1) {
    g_set_error(error,
                GRN_TEST_SERVER_ERROR,
                GRN_TEST_SERVER_ERROR_IO,
                "failed to create base directory: %s", g_strerror(errno));
    return FALSE;
  }

  return TRUE;
}

gboolean
grn_test_server_start(GrnTestServer *server, GError **error)
{
  GrnTestServerPrivate *priv;
  const gchar *database_path;
  gchar *port_string;

  priv = GRN_TEST_SERVER_GET_PRIVATE(server);
  if (priv->egg) {
    g_set_error(error,
                GRN_TEST_SERVER_ERROR,
                GRN_TEST_SERVER_ERROR_ALREADY_STARTED,
                "server is already started");
    return FALSE;
  }

  database_path = grn_test_server_get_database_path(server, error);
  if (!database_path)
    return FALSE;

  port_string = g_strdup_printf("%u", priv->port);
  priv->egg = gcut_egg_new(GROONGA,
                           "-s",
                           "-i", priv->address,
                           "-p", port_string,
                           "-n", database_path,
                           "-e", priv->encoding,
                           NULL);
  g_free(port_string);
  if (!gcut_egg_hatch(priv->egg, error))
    return FALSE;

  g_usleep(1.5 * G_USEC_PER_SEC); /* FIXME: use select */

  return TRUE;
}

gboolean
grn_test_server_stop(GrnTestServer *server, GError **error)
{
  GrnTestServerPrivate *priv;

  priv = GRN_TEST_SERVER_GET_PRIVATE(server);
  if (!priv->egg) {
    g_set_error(error,
                GRN_TEST_SERVER_ERROR,
                GRN_TEST_SERVER_ERROR_NOT_STARTED,
                "server is not started");
    return FALSE;
  }

  /* FIXME: send shutdown command */

  g_object_unref(priv->egg);
  priv->egg = NULL;

  return TRUE;
}

const gchar *
grn_test_server_get_database_path(GrnTestServer *server, GError **error)
{
  GrnTestServerPrivate *priv;

  priv = GRN_TEST_SERVER_GET_PRIVATE(server);
  if (!priv->custom_database_path) {
    if (!grn_test_server_ensure_base_directory(server, error))
      return NULL;
    if (priv->database_path)
      g_free(priv->database_path);
    priv->database_path = g_build_filename(priv->base_directory,
                                           "server.db", NULL);
  }

  return priv->database_path;
}

void
grn_test_server_set_database_path (GrnTestServer *server, const gchar *path)
{
  GrnTestServerPrivate *priv;

  priv = GRN_TEST_SERVER_GET_PRIVATE(server);

  g_free(priv->database_path);
  if (path) {
    priv->custom_database_path = TRUE;
    priv->database_path = g_strdup(path);
  } else {
    priv->custom_database_path = FALSE;
    priv->database_path = NULL;
  }
}

const gchar *
grn_test_server_get_address(GrnTestServer *server)
{
  return GRN_TEST_SERVER_GET_PRIVATE(server)->address;
}

void
grn_test_server_set_address(GrnTestServer *server, const gchar *address)
{
  GrnTestServerPrivate *priv;

  priv = GRN_TEST_SERVER_GET_PRIVATE(server);

  g_free(priv->address);
  priv->address = g_strdup(address);
}

guint
grn_test_server_get_port(GrnTestServer *server)
{
  return GRN_TEST_SERVER_GET_PRIVATE(server)->port;
}

void
grn_test_server_set_port(GrnTestServer *server, guint port)
{
  GRN_TEST_SERVER_GET_PRIVATE(server)->port = port;
}

const gchar *
grn_test_server_get_encoding(GrnTestServer *server)
{
  return GRN_TEST_SERVER_GET_PRIVATE(server)->encoding;
}

void
grn_test_server_set_encoding(GrnTestServer *server, const gchar *encoding)
{
  GrnTestServerPrivate *priv;

  priv = GRN_TEST_SERVER_GET_PRIVATE(server);

  g_free(priv->encoding);
  priv->encoding = g_strdup(encoding);
}

const gchar *
grn_test_server_get_http_uri_base(GrnTestServer *server)
{
  GrnTestServerPrivate *priv;

  priv = GRN_TEST_SERVER_GET_PRIVATE(server);
  g_free(priv->http_uri_base);
  priv->http_uri_base =
    g_strdup_printf("http://%s:%u/", priv->address, priv->port);
  return priv->http_uri_base;
}

const gchar *
grn_test_server_get_memcached_address(GrnTestServer *server)
{
  GrnTestServerPrivate *priv;

  priv = GRN_TEST_SERVER_GET_PRIVATE(server);
  g_free(priv->memcached_address);
  priv->memcached_address = g_strdup_printf("%s:%u", priv->address, priv->port);
  return priv->memcached_address;
}
