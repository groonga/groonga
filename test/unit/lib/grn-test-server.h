/*
  Copyright (C) 2009  Kouhei Sutou <kou@clear-code.com>

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

#ifndef __GRN_TEST_SERVER_H__
#define __GRN_TEST_SERVER_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define GRN_TEST_SERVER_ERROR           (grn_test_server_error_quark())

#define GRN_TYPE_TEST_SERVER            (grn_test_server_get_type())
#define GRN_TEST_SERVER(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), GRN_TYPE_TEST_SERVER, GrnTestServer))
#define GRN_TEST_SERVER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), GRN_TYPE_TEST_SERVER, GrnTestServerClass))
#define GRN_IS_TEST_SERVER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), GRN_TYPE_TEST_SERVER))
#define GRN_IS_TEST_SERVER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GRN_TYPE_TEST_SERVER))
#define GRN_TEST_SERVER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GRN_TYPE_TEST_SERVER, GrnTestServerClass))

typedef struct _GrnTestServer         GrnTestServer;
typedef struct _GrnTestServerClass    GrnTestServerClass;

struct _GrnTestServer
{
  GObject object;
};

struct _GrnTestServerClass
{
  GObjectClass parent_class;
};

typedef enum
{
  GRN_TEST_SERVER_ERROR_IO,
  GRN_TEST_SERVER_ERROR_ALREADY_STARTED,
  GRN_TEST_SERVER_ERROR_NOT_STARTED
} GrnTestServerError;

GQuark              grn_test_server_error_quark (void);

GType               grn_test_server_get_type (void) G_GNUC_CONST;

GrnTestServer      *grn_test_server_new      (void);

gboolean            grn_test_server_start    (GrnTestServer  *server,
                                              GError        **error);
gboolean            grn_test_server_stop     (GrnTestServer  *server,
                                              GError        **error);

const gchar        *grn_test_server_get_database_path
                                             (GrnTestServer  *server,
                                              GError        **error);
void                grn_test_server_set_database_path
                                             (GrnTestServer *server,
                                              const gchar   *path);

const gchar        *grn_test_server_get_address
                                             (GrnTestServer *server);
void                grn_test_server_set_address
                                             (GrnTestServer *server,
                                              const gchar   *address);

guint               grn_test_server_get_port (GrnTestServer *server);
void                grn_test_server_set_port (GrnTestServer *server,
                                              guint          port);

const gchar        *grn_test_server_get_encoding
                                             (GrnTestServer *server);
void                grn_test_server_set_encoding
                                             (GrnTestServer *server,
                                              const gchar   *encoding);

const gchar        *grn_test_server_get_http_uri_base
                                             (GrnTestServer *server);
const gchar        *grn_test_server_get_memcached_address
                                             (GrnTestServer *server);

G_END_DECLS

#endif
