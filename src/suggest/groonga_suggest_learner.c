/* -*- c-basic-offset: 2 -*- */
/* Copyright(C) 2010- Brazil

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
#include <zmq.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <msgpack.h>
#include <pthread.h>
#include <groonga.h>

#define DEFAULT_RECV_ENDPOINT "tcp://*:1234"
#define DEFAULT_SEND_ENDPOINT "tcp://*:1235"
#define SEND_WAIT 1000 /* 0.001sec */

int print_error(const char *format, ...);

#define CONST_STR_LEN(x) x, x ? sizeof(x) - 1 : 0

typedef struct {
  const char *db_path;
  const char *send_endpoint;
  pthread_t thd;
  void *zmq_ctx;
} send_thd_data;

static volatile sig_atomic_t loop = 1;

static void
load_to_groonga(grn_ctx *ctx,
                grn_obj *buf,
                const char *query, uint32_t query_len,
                const char *client_id, uint32_t client_id_len,
                const char *learn_target_name, uint32_t learn_target_name_len,
                uint64_t millisec,
                int submit)
{
  GRN_BULK_REWIND(buf);
  GRN_TEXT_PUTS(ctx, buf, "load --table event_");
  GRN_TEXT_PUT(ctx, buf, learn_target_name, learn_target_name_len);
  GRN_TEXT_PUTS(ctx, buf, " --each 'suggest_preparer(_id,type,item,sequence,time,pair_");
  GRN_TEXT_PUT(ctx, buf, learn_target_name, learn_target_name_len);
  GRN_TEXT_PUTS(ctx, buf, ")'");
  grn_ctx_send(ctx, GRN_TEXT_VALUE(buf), GRN_TEXT_LEN(buf), GRN_CTX_MORE);
  grn_ctx_send(ctx, CONST_STR_LEN("["), GRN_CTX_MORE);

  GRN_BULK_REWIND(buf);
  GRN_TEXT_PUTS(ctx, buf, "{\"item\":");
  grn_text_esc(ctx, buf, query, query_len);
  GRN_TEXT_PUTS(ctx, buf, ",\"sequence\":");
  grn_text_esc(ctx, buf, client_id, client_id_len);
  GRN_TEXT_PUTS(ctx, buf, ",\"time\":");
  grn_text_ftoa(ctx, buf, (double)millisec / 1000);
  if (submit) {
    GRN_TEXT_PUTS(ctx, buf, ",\"type\":\"submit\"}");
  } else {
    GRN_TEXT_PUTS(ctx, buf, "}");
  }
  /* printf("%.*s\n", GRN_TEXT_LEN(buf), GRN_TEXT_VALUE(buf)); */
  grn_ctx_send(ctx, GRN_TEXT_VALUE(buf), GRN_TEXT_LEN(buf), GRN_CTX_MORE);

  grn_ctx_send(ctx, CONST_STR_LEN("]"), 0);

  {
    char *res;
    int flags;
    unsigned int res_len;
    grn_ctx_recv(ctx, &res, &res_len, &flags);
  }
}

#define PACK_KEY_FROM_ID(id) \
{ \
  int _k_len; \
  char _k_buf[GRN_TABLE_MAX_KEY_SIZE]; \
  _k_len = grn_table_get_key(ctx, ref_table, (id), _k_buf, GRN_TABLE_MAX_KEY_SIZE); \
  msgpack_pack_raw(&pk, _k_len); \
  msgpack_pack_raw_body(&pk, _k_buf, _k_len); \
}

#define PACK_MAP_ITEM(col_name) \
{ \
  grn_obj _v; \
  msgpack_pack_raw(&pk, sizeof(#col_name) - 1); \
  msgpack_pack_raw_body(&pk, CONST_STR_LEN(#col_name)); \
  switch (col_##col_name->header.type) { \
  case GRN_COLUMN_FIX_SIZE: \
    GRN_VALUE_FIX_SIZE_INIT(&_v, 0, grn_obj_get_range(ctx, col_##col_name)); \
    break; \
  case GRN_COLUMN_VAR_SIZE: \
    if ((col_##col_name->header.flags & GRN_OBJ_COLUMN_TYPE_MASK) == GRN_OBJ_COLUMN_VECTOR) { \
      GRN_VALUE_FIX_SIZE_INIT(&_v, GRN_OBJ_VECTOR, grn_obj_get_range(ctx, col_##col_name)); \
    } else { \
      GRN_VALUE_VAR_SIZE_INIT(&_v, 0, grn_obj_get_range(ctx, col_##col_name)); \
    } \
    break; \
  } \
  grn_obj_get_value(ctx, col_##col_name, rec_id, &_v); \
 \
  switch (_v.header.type) { \
  case GRN_BULK: \
    switch (_v.header.domain) { \
    case GRN_DB_SHORT_TEXT: \
      msgpack_pack_raw(&pk, GRN_TEXT_LEN(&_v)); \
      msgpack_pack_raw_body(&pk, GRN_TEXT_VALUE(&_v), GRN_TEXT_LEN(&_v)); \
      break; \
    case GRN_DB_INT32: \
      msgpack_pack_int32(&pk, GRN_INT32_VALUE(&_v)); \
      break; \
    case GRN_DB_UINT32: \
      msgpack_pack_uint32(&pk, GRN_UINT32_VALUE(&_v)); \
      break; \
    case GRN_DB_TIME: \
      msgpack_pack_double(&pk, (double)GRN_TIME_VALUE(&_v) / GRN_TIME_USEC_PER_SEC); \
      break; \
    default: /* ref. to ShortText key */ \
      PACK_KEY_FROM_ID(GRN_RECORD_VALUE(&_v)); \
    } \
    break; \
  case GRN_UVECTOR: /* ref.s to ShortText key */ \
    { \
      grn_id *_idv = (grn_id *)GRN_BULK_HEAD(&_v), *_idve = (grn_id *)GRN_BULK_CURR(&_v); \
      msgpack_pack_array(&pk, _idve - _idv); \
      for (; _idv < _idve; _idv++) { \
        PACK_KEY_FROM_ID(*_idv); \
      } \
    } \
    break; \
  default: \
    print_error("invalid groonga object type(%d) for msgpack.", _v.header.type); \
    msgpack_pack_nil(&pk); \
    break; \
  } \
  grn_obj_close(ctx, &_v); \
}

static int
zmq_send_to_httpd(void *zmq_send_sock, void *data, size_t size)
{
  zmq_msg_t msg;
  if (!zmq_msg_init_size(&msg, size)) {
    memcpy((void *)zmq_msg_data(&msg), data, size);
    if (zmq_send(zmq_send_sock, &msg, 0)) {
      print_error("zmq_send() error");
      return -1;
    }
    zmq_msg_close(&msg);
  } else {
    print_error("zmq_msg_init_size() error");
  }
  return 0;
}

static void
send_handler(void *zmq_send_sock, grn_ctx *ctx)
{
  grn_table_cursor *cur;
  if ((cur = grn_table_cursor_open(ctx, grn_ctx_db(ctx), NULL, 0, NULL, 0,
       0, -1, 0))) {
    grn_id table_id;
    while (loop && (table_id = grn_table_cursor_next(ctx, cur)) != GRN_ID_NIL) {
      grn_obj *table;
      if ((table = grn_ctx_at(ctx, table_id))) {
        int name_len;
        char name_buf[GRN_TABLE_MAX_KEY_SIZE];

        name_len = grn_obj_name(ctx, table, name_buf,
                                GRN_TABLE_MAX_KEY_SIZE);

        if (name_len > 5) {
          if (table->header.type == GRN_TABLE_PAT_KEY &&
              !memcmp(name_buf, CONST_STR_LEN("item_"))) {
            /* ["_key","ShortText"],["last","Time"],["kana","kana"],["freq2","Int32"],["freq","Int32"],["co","pair_all"],["buzz","Int32"],["boost","Int32"] */
            grn_obj *ref_table;
            grn_table_cursor *tc;
            grn_obj *col_last, *col_kana, *col_freq, *col_freq2,
                    *col_buzz, *col_boost;

            col_kana = grn_obj_column(ctx, table, CONST_STR_LEN("kana"));
            col_freq = grn_obj_column(ctx, table, CONST_STR_LEN("freq"));
            col_last = grn_obj_column(ctx, table, CONST_STR_LEN("last"));
            col_boost = grn_obj_column(ctx, table, CONST_STR_LEN("boost"));
            col_freq2 = grn_obj_column(ctx, table, CONST_STR_LEN("freq2"));
            col_buzz = grn_obj_column(ctx, table, CONST_STR_LEN("buzz"));

            ref_table = grn_ctx_at(ctx, grn_obj_get_range(ctx, col_kana));

            if ((tc = grn_table_cursor_open(ctx, table, NULL, 0, NULL,
                                            0, 0, -1, 0))) {
              grn_id rec_id;
              while (loop && (rec_id = grn_table_cursor_next(ctx, tc))
                     != GRN_ID_NIL) {
                char *key;
                size_t key_len;
                msgpack_packer pk;
                msgpack_sbuffer sbuf;

                msgpack_sbuffer_init(&sbuf);
                msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);

                msgpack_pack_map(&pk, 8);

                /* ["_key","ShortText"],["last","Time"],["kana","kana"],["freq2","Int32"],["freq","Int32"],["co","pair_all"],["buzz","Int32"],["boost","Int32"] */
                msgpack_pack_raw(&pk, 6);
                msgpack_pack_raw_body(&pk, CONST_STR_LEN("target"));
                msgpack_pack_raw(&pk, name_len);
                msgpack_pack_raw_body(&pk, name_buf, name_len);

                msgpack_pack_raw(&pk, 4);
                msgpack_pack_raw_body(&pk, CONST_STR_LEN("_key"));
                key_len = grn_table_cursor_get_key(ctx, tc, (void **)&key);
                msgpack_pack_raw(&pk, key_len);
                msgpack_pack_raw_body(&pk, key, key_len);

                PACK_MAP_ITEM(last);
                PACK_MAP_ITEM(kana);
                PACK_MAP_ITEM(freq);
                PACK_MAP_ITEM(freq2);
                PACK_MAP_ITEM(buzz);
                PACK_MAP_ITEM(boost);

                zmq_send_to_httpd(zmq_send_sock, sbuf.data, sbuf.size);

                usleep(SEND_WAIT);

                msgpack_sbuffer_destroy(&sbuf);
              }
              grn_table_cursor_close(ctx, tc);
            }
          } else if (table->header.type == GRN_TABLE_HASH_KEY &&
                     !memcmp(name_buf, CONST_STR_LEN("pair_"))) {
            grn_obj *ref_table;
            grn_table_cursor *tc;
            grn_obj *col_pre, *col_post, *col_freq0, *col_freq1, *col_freq2;

            col_pre = grn_obj_column(ctx, table, CONST_STR_LEN("pre"));
            col_post = grn_obj_column(ctx, table, CONST_STR_LEN("post"));
            col_freq0 = grn_obj_column(ctx, table, CONST_STR_LEN("freq0"));
            col_freq1 = grn_obj_column(ctx, table, CONST_STR_LEN("freq1"));
            col_freq2 = grn_obj_column(ctx, table, CONST_STR_LEN("freq2"));

            ref_table = grn_ctx_at(ctx, grn_obj_get_range(ctx, col_pre));

            if ((tc = grn_table_cursor_open(ctx, table, NULL, 0, NULL,
                                            0, 0, -1, 0))) {
              grn_id rec_id;
              while (loop && (rec_id = grn_table_cursor_next(ctx, tc))
                     != GRN_ID_NIL) {
                uint64_t *key;
                msgpack_packer pk;
                msgpack_sbuffer sbuf;

                /* skip freq0 == 0 && freq1 == 0 && freq2 == 0 */
                {
                  grn_obj f;
                  grn_obj_get_value(ctx, col_freq0, rec_id, &f);
                  if (!GRN_INT32_VALUE(&f)) {
                    grn_obj_get_value(ctx, col_freq1, rec_id, &f);
                    if (!GRN_INT32_VALUE(&f)) {
                      grn_obj_get_value(ctx, col_freq2, rec_id, &f);
                      if (!GRN_INT32_VALUE(&f)) { continue; }
                    }
                  }
                }

                /* make pair_* message */
                msgpack_sbuffer_init(&sbuf);
                msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);

                msgpack_pack_map(&pk, 7);
                /* ["_key","UInt64"],["pre","item_all"],["post","item_all"],["freq2","Int32"],["freq1","Int32"],["freq0","Int32"] */

                msgpack_pack_raw(&pk, 6);
                msgpack_pack_raw_body(&pk, CONST_STR_LEN("target"));
                msgpack_pack_raw(&pk, name_len);
                msgpack_pack_raw_body(&pk, name_buf, name_len);

                msgpack_pack_raw(&pk, 4);
                msgpack_pack_raw_body(&pk, CONST_STR_LEN("_key"));
                grn_table_cursor_get_key(ctx, tc, (void **)&key);
                msgpack_pack_uint64(&pk, *key);

                PACK_MAP_ITEM(pre);
                PACK_MAP_ITEM(post);
                PACK_MAP_ITEM(freq0);
                PACK_MAP_ITEM(freq1);
                PACK_MAP_ITEM(freq2);

                zmq_send_to_httpd(zmq_send_sock, sbuf.data, sbuf.size);

                usleep(SEND_WAIT);

                msgpack_sbuffer_destroy(&sbuf);
              }
            }
          }
        }
      }
    }
  }
}

static void *
send_to_httpd(void *arg)
{
  send_thd_data *thd = arg;
  void *zmq_send_sock;
  if ((zmq_send_sock = zmq_socket(thd->zmq_ctx, ZMQ_PUB))) {
    if (!zmq_bind(zmq_send_sock, thd->send_endpoint)) {
      grn_ctx ctx;
      if (!(grn_ctx_init(&ctx, 0))) {
        grn_obj *db;
        if ((db = grn_db_open(&ctx, thd->db_path))) {
          while (loop) {
            uint64_t hwm = 1;
            zmq_setsockopt(zmq_send_sock, ZMQ_HWM, &hwm, sizeof(uint64_t));
            send_handler(zmq_send_sock, &ctx);
          }
          grn_obj_close(&ctx, db);
        } else {
          print_error("error in grn_db_open() on send thread.");
        }
        grn_ctx_fin(&ctx);
      } else {
        print_error("error in grn_ctx_init() on send thread.");
      }
    } else {
      print_error("cannot bind zmq_socket.");
    }
  } else {
    print_error("cannot create zmq_socket.");
  }
  return NULL;
}

static void
handle_msg(msgpack_object *obj, grn_ctx *ctx, grn_obj *buf)
{
  int submit_flag = 0;
  uint64_t millisec = 0;
  const char *query = NULL,
             *client_id = NULL, *learn_target_names = NULL;
  uint32_t query_len, client_id_len, learn_target_names_len;
  if (obj->type == MSGPACK_OBJECT_MAP) {
    int i;
    for (i = 0; i < obj->via.map.size; i++) {
      msgpack_object_kv *kv;
      kv = &(obj->via.map.ptr[i]);
      if (kv->key.type == MSGPACK_OBJECT_RAW && kv->key.via.raw.size) {
        switch (kv->key.via.raw.ptr[0]) {
        case 'i':
          if (kv->val.type == MSGPACK_OBJECT_RAW) {
            client_id_len = kv->val.via.raw.size;
            client_id = kv->val.via.raw.ptr;
          }
          break;
        case 'q':
          if (kv->val.type == MSGPACK_OBJECT_RAW) {
            query_len = kv->val.via.raw.size;
            query = kv->val.via.raw.ptr;
          }
          break;
        case 'l':
          if (kv->val.type == MSGPACK_OBJECT_RAW) {
            learn_target_names_len = kv->val.via.raw.size;
            learn_target_names = kv->val.via.raw.ptr;
          }
          break;
        case 's':
          if (kv->val.type == MSGPACK_OBJECT_POSITIVE_INTEGER) {
            millisec = kv->val.via.u64;
          }
          break;
        case 't':
          if (kv->val.type == MSGPACK_OBJECT_BOOLEAN) {
            submit_flag = (kv->val.via.boolean ? 1 : 0);
          }
          break;
        default:
          break;
        }
      }
    }
    if (millisec && query && client_id && learn_target_names) {
      unsigned int tn_len;
      const char *tn, *tnp, *tne;
      tn = tnp = learn_target_names;
      tne = learn_target_names + learn_target_names_len;
      while (tnp <= tne) {
        if (tnp == tne || *tnp == '|') {
          tn_len = tnp - tn;

          /*
          printf("sec: %llu query %.*s client_id: %.*s target: %.*s\n",
            millisec,
            query_len, query,
            client_id_len, client_id,
            tn_len, tn);
          */
          load_to_groonga(ctx, buf, query, query_len, client_id, client_id_len,
                          tn, tn_len, millisec, submit_flag);

          tn = ++tnp;
        } else {
          tnp++;
        }
      }
    }
  }
}

static void
event_loop(msgpack_zone *mempool, void *zmq_sock, grn_ctx *ctx)
{
  grn_obj buf;
  zmq_pollitem_t items[] = {
    { zmq_sock, 0, ZMQ_POLLIN, 0}
  };
  GRN_TEXT_INIT(&buf, 0);
  while (loop) {
    zmq_poll(items, 1, 10000);
    if (items[0].revents & ZMQ_POLLIN) { /* always true */
      zmq_msg_t msg;
      if (zmq_msg_init(&msg)) {
        print_error("cannot init zmq message.");
      } else {
        if (zmq_recv(zmq_sock, &msg, 0)) {
          print_error("cannot recv zmq message.");
        } else {
          msgpack_object obj;
          msgpack_unpack_return ret;
          ret = msgpack_unpack(zmq_msg_data(&msg), zmq_msg_size(&msg), NULL, mempool, &obj);
          if (MSGPACK_UNPACK_SUCCESS == ret) {
            /* msgpack_object_print(stdout, obj); */
            handle_msg(&obj, ctx, &buf);
          }
          msgpack_zone_clear(mempool);
        }
        zmq_msg_close(&msg);
      }
    }
  }
  grn_obj_unlink(ctx, &buf);
}

static void
usage(FILE *output)
{
  fprintf(output,
          "Usage: suggest-learner [options...] db_path\n"
          "options:\n"
          "  -r <recv endpoint>: recv endpoint (default: %s)\n"
          "  -s <send endpoint>: send endpoint (default: %s)\n",
          DEFAULT_RECV_ENDPOINT, DEFAULT_SEND_ENDPOINT);
}

static void
signal_handler(int sig)
{
  loop = 0;
}

int
main(int argc, char **argv)
{
  const char *recv_endpoint = DEFAULT_RECV_ENDPOINT,
             *send_endpoint = DEFAULT_SEND_ENDPOINT;

  /* parse options */
  {
    int ch;

    while ((ch = getopt(argc, argv, "r:s:")) != -1) {
      switch(ch) {
      case 'r':
        recv_endpoint = optarg;
        break;
      case 's':
        send_endpoint = optarg;
        break;
      }
    }
    argc -= optind; argv += optind;
  }

  /* main */
  if (argc != 1) {
    usage(stderr);
  } else {
    grn_ctx *ctx;
    msgpack_zone *mempool;

    grn_init();

    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGQUIT, signal_handler);

    ctx = grn_ctx_open(0);
    if (!(grn_db_open(ctx, argv[0]))) {
      print_error("cannot open database.");
    } else {
      if (!(mempool = msgpack_zone_new(MSGPACK_ZONE_CHUNK_SIZE))) {
        print_error("cannot create msgpack zone.");
      } else {
        void *zmq_ctx, *zmq_recv_sock;
        if (!(zmq_ctx = zmq_init(1))) {
          print_error("cannot create zmq context.");
        } else {
          if (!(zmq_recv_sock = zmq_socket(zmq_ctx, ZMQ_SUB))) {
            print_error("cannot create zmq_socket.");
          } else if (zmq_bind(zmq_recv_sock, recv_endpoint)) {
            print_error("cannot bind zmq_socket.");
          } else {
            send_thd_data thd;
            zmq_setsockopt(zmq_recv_sock, ZMQ_SUBSCRIBE, "", 0);
            thd.db_path = argv[0];
            thd.send_endpoint = send_endpoint;
            thd.zmq_ctx = zmq_ctx;

            if (pthread_create(&(thd.thd), NULL, send_to_httpd, &thd)) {
              print_error("error in pthread_create() for sending datas.");
            }
            event_loop(mempool, zmq_recv_sock, ctx);
            pthread_join(thd.thd, NULL);
          }
          zmq_term(zmq_ctx);
        }
        msgpack_zone_free(mempool);
      }
    }
    grn_obj_close(ctx, grn_ctx_db(ctx));
    grn_ctx_fin(ctx);
    grn_fin();
  }
  return 0;
}
