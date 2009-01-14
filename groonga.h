/* Copyright(C) 2009 Brazil

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifndef GROONGA_H
#define GROONGA_H

#ifdef  __cplusplus
extern "C" {
#endif

typedef unsigned snp_id;

#define SNP_ID_NIL 0
#define SNP_ID_MAX 0x3fffffff

typedef enum {
  snp_success = 0,
  snp_memory_exhausted,
  snp_invalid_format,
  snp_file_operation_error,
  snp_invalid_argument,
  snp_other_error,
  snp_external_error,
  snp_internal_error,
  snp_abnormal_error,
  snp_end_of_data
} snp_rc;

snp_rc snp_init(void);
snp_rc snp_fin(void);

typedef enum {
  snp_enc_default = 0,
  snp_enc_none,
  snp_enc_euc_jp,
  snp_enc_utf8,
  snp_enc_sjis,
  snp_enc_latin1,
  snp_enc_koi8r
} snp_encoding;

typedef enum {
  snp_log_none = 0,
  snp_log_emerg,
  snp_log_alert,
  snp_log_crit,
  snp_log_error,
  snp_log_warning,
  snp_log_notice,
  snp_log_info,
  snp_log_debug,
  snp_log_dump
} snp_log_level;

typedef struct _snp_ctx snp_ctx;

#define SNP_CTX_MSGSIZE                (128)
#define SNP_CTX_FIN                    (0xff)

struct _snp_ctx {
  snp_rc rc;
  int flags;
  snp_encoding encoding;
  unsigned char ntrace;
  unsigned char errlvl;
  unsigned char stat;
  unsigned int seqno;
  unsigned int subno;
  unsigned int seqno2;
  unsigned int errline;
  snp_ctx *prev;
  snp_ctx *next;
  const char *errfile;
  const char *errfunc;
  struct _snp_ctx_impl *impl;
  void *trace[16];
  char errbuf[SNP_CTX_MSGSIZE];
};

#define SNP_CTX_INITIALIZER \
  { snp_success, 0, snp_enc_default, 0, snp_log_notice,\
    SNP_CTX_FIN, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL }

#define SNP_CTX_CLOSED(ctx) ((ctx)->stat == SNP_CTX_FIN)

/**
 * snp_ctx_init:
 * @ctx: 初期化するctx構造体へのポインタを指定します。
 * @flags: 初期化するctxのオプションを指定します。
 * SNP_CTX_NO_DBを指定すると、エラーハンドリングのみが行えるctxを初期化します。
 * SNP_CTX_USE_DBを指定すると、全てのDBAPIが使用できるctxを初期化します。
 * SNP_CTX_USE_QLを指定すると、groonga qlインタプリタを実行可能なctxを初期化します。
 * SNP_CTX_USE_QL|SNP_CTX_BATCH_MODEを指定すると、batchmodeでインタプリタを初期化します。
 * @encoding: 初期化するctxでデフォルトとなるencoding。
 *
 * ctxを初期化します。
 **/

#define SNP_CTX_NO_DB                  (0x00)
#define SNP_CTX_USE_DB                 (0x01)
#define SNP_CTX_USE_QL                 (0x03)
#define SNP_CTX_BATCH_MODE             (0x04)

snp_rc snp_ctx_init(snp_ctx *ctx, int flags, snp_encoding encoding);

/**
 * snp_ctx_fin:
 * @ctx: 終了化するctx構造体へのポインタを指定します。
 *
 * ctxの管理するメモリを解放し、使用を終了します。
 **/
snp_rc snp_ctx_fin(snp_ctx *ctx);

/* obj */

typedef unsigned int snp_obj_flags;

#define SNP_OBJ_TABLE_TYPE_MASK        (0x07L)
#define SNP_OBJ_TABLE_HASH_KEY         (0x00L)
#define SNP_OBJ_TABLE_PAT_KEY          (0x01L)
#define SNP_OBJ_TABLE_NO_KEY           (0x03L)
#define SNP_OBJ_TABLE_ALIAS            (0x04L)

#define SNP_OBJ_KEY_MASK               (0x07L<<3)
#define SNP_OBJ_KEY_UINT               (0x00L<<3)
#define SNP_OBJ_KEY_INT                (0x01L<<3)
#define SNP_OBJ_KEY_FLOAT              (0x02L<<3)

#define SNP_OBJ_TOKEN_MASK             (0x07L<<3)
#define SNP_OBJ_TOKEN_MECAB            (0x00L<<3)
#define SNP_OBJ_TOKEN_NGRAM            (0x01L<<3)
#define SNP_OBJ_TOKEN_DELIMITED        (0x02L<<3)
#define SNP_OBJ_TOKEN_USER_DEFINED     (0x07L<<3)

#define SNP_OBJ_KEY_WITH_SIS           (1L<<6)
#define SNP_OBJ_KEY_NORMALIZE          (1L<<7)
#define SNP_OBJ_KEY_SPLIT_ALPHA        (1L<<8)
#define SNP_OBJ_KEY_SPLIT_DIGIT        (1L<<9)
#define SNP_OBJ_KEY_SPLIT_SYMBOL       (1L<<10)

#define SNP_OBJ_COLUMN_TYPE_MASK       (0x07L)
#define SNP_OBJ_COLUMN_SCALAR          (0x00L)
#define SNP_OBJ_COLUMN_ARRAY           (0x01L)
#define SNP_OBJ_COLUMN_VERSES          (0x02L)
#define SNP_OBJ_COLUMN_POSTINGS        (0x03L)
#define SNP_OBJ_COLUMN_INDEX           (0x04L)

#define SNP_OBJ_COMPRESS_MASK          (0x07L<<4)
#define SNP_OBJ_COMPRESS_NONE          (0x00L<<4)
#define SNP_OBJ_COMPRESS_ZLIB          (0x01L<<4)
#define SNP_OBJ_COMPRESS_LZO           (0x02L<<4)

#define SNP_OBJ_WITH_SECTION           (0L<<7)
#define SNP_OBJ_NO_SECTION             (1L<<7)
#define SNP_OBJ_WITH_SCORE             (0L<<8)
#define SNP_OBJ_NO_SCORE               (1L<<8)
#define SNP_OBJ_WITH_POSITION          (0L<<9)
#define SNP_OBJ_NO_POSITION            (1L<<9)

#define SNP_OBJ_UNIT_MASK              (0x0fL<<8)
#define SNP_OBJ_UNIT_DOCUMENT_NONE     (0x00L<<8)
#define SNP_OBJ_UNIT_DOCUMENT_SECTION  (0x01L<<8)
#define SNP_OBJ_UNIT_DOCUMENT_POSITION (0x02L<<8)
#define SNP_OBJ_UNIT_SECTION_NONE      (0x03L<<8)
#define SNP_OBJ_UNIT_SECTION_POSITION  (0x04L<<8)
#define SNP_OBJ_UNIT_POSITION_NONE     (0x05L<<8)
#define SNP_OBJ_UNIT_USERDEF_DOCUMENT  (0x06L<<8)
#define SNP_OBJ_UNIT_USERDEF_SECTION   (0x07L<<8)
#define SNP_OBJ_UNIT_USERDEF_POSITION  (0x08L<<8)

#define SNP_OBJ_NO_SUBREC              (0L<<13)
#define SNP_OBJ_WITH_SUBREC            (1L<<13)

#define SNP_OBJ_COLUMN_INDEX_SCALAR    (1L<<14)

#define SNP_OBJ_KEY_VAR_SIZE           (1L<<14)

#define SNP_OBJ_DO_DEEP_COPY           (0L<<14)
#define SNP_OBJ_DO_SHALLOW_COPY        (1L<<14)

#define SNP_OBJ_TEMPORARY              (0L<<15)
#define SNP_OBJ_PERSISTENT             (1L<<15)

/* obj types */

#define SNP_VOID                       (0x00)
#define SNP_BULK                       (0x01)
#define SNP_VECTOR                     (0x02)
#define SNP_VERSES                     (0x03)
#define SNP_QUERY                      (0x08)
#define SNP_ACCESSOR                   (0x09)
#define SNP_SNIP                       (0x0a)
#define SNP_PATSNIP                    (0x0b)
#define SNP_CURSOR_TABLE_HASH_KEY      (0x10)
#define SNP_CURSOR_TABLE_PAT_KEY       (0x11)
#define SNP_CURSOR_TABLE_NO_KEY        (0x13)
#define SNP_CURSOR_COLUMN_INDEX        (0x18)
#define SNP_TYPE                       (0x20)
#define SNP_PROC                       (0x21)
#define SNP_TABLE_HASH_KEY             (0x30)
#define SNP_TABLE_PAT_KEY              (0x31)
#define SNP_TABLE_NO_KEY               (0x33)
#define SNP_DB                         (0x37)
#define SNP_COLUMN_FIX_SIZE            (0x40)
#define SNP_COLUMN_VAR_SIZE            (0x41)
#define SNP_COLUMN_INDEX               (0x48)

typedef struct _snp_verse snp_verse;
typedef struct _snp_obj snp_obj;
typedef struct _snp_obj_header snp_obj_header;

struct _snp_verse {
  char *str;
  unsigned int str_len;
  unsigned int weight;
  snp_id domain;
};

struct _snp_obj_header {
  unsigned char type;
  unsigned char impl_flags;
  snp_obj_flags flags;
  snp_id domain;
};

struct _snp_obj {
  snp_obj_header header;
  union {
    struct {
      char *head;
      char *curr;
      char *tail;
    } b;
    struct {
      snp_obj *src;
      snp_verse *verses;
      int n_verses;
    } v;
  } u;
};

#define SNP_OBJ_INIT(obj,obj_type,obj_flags) {\
  (obj)->header.type = (obj_type);\
  (obj)->header.flags = (obj_flags);\
  (obj)->header.impl_flags = 0;\
  (obj)->header.domain = SNP_ID_NIL;\
  (obj)->u.b.head = NULL;\
  (obj)->u.b.curr = NULL;\
  (obj)->u.b.tail = NULL;\
}

#define SNP_OBJ_FIN(ctx,obj) (snp_obj_close((ctx), (obj)))

/**
 * snp_db_create:
 * @path: 作成するdbを格納するファイルパス。NULLならtemporary dbとなる。
 * @encoding: 作成するdbでデフォルトとなるencoding。
 *
 * 新たなdbを作成する。
 **/
snp_obj *snp_db_create(snp_ctx *ctx, const char *path, snp_encoding encoding);

/**
 * snp_db_open:
 * @path: 開こうとするdbを格納するファイルパス。
 *
 * 既存のdbを開く。
 **/
snp_obj *snp_db_open(snp_ctx *ctx, const char *path);

/**
 * snp_ctx_use:
 * @db: ctxが使用するdbを指定します。
 *
 * ctxが操作対象とするdbを指定します。NULLを指定した場合は、
 * dbを操作しない状態(init直後の状態)になります。
 **/
snp_rc snp_ctx_use(snp_ctx *ctx, snp_obj *db);

/**
 * snp_ctx_db:
 *
 * ctxが現在操作対象としているdbを返します。
 * dbを使用していない場合はNULLを返します。
 **/
snp_obj *snp_ctx_db(snp_ctx *ctx);

/**
 * snp_ctx_lookup:
 * @name: 検索しようとするオブジェクトの名前。
 * @name_size: @nameのbyte長。
 *
 * ctxが使用するdbからnameに対応するオブジェクトを検索して返す。
 * nameに一致するオブジェクトが存在しなければNULLを返す。
 **/
snp_obj *snp_ctx_lookup(snp_ctx *ctx, const char *name, unsigned name_size);

/**
 * snp_ctx_get:
 * @id: 検索しようとするオブジェクトのid。
 *
 * ctx、またはctxが使用するdbからidに対応するオブジェクトを検索して返す。
 * idに一致するオブジェクトが存在しなければNULLを返す。
 **/
snp_obj *snp_ctx_get(snp_ctx *ctx, snp_id id);

/**
 * snp_type_create:
 * @name: 作成するtypeの名前。
 * @flags: SNP_OBJ_KEY_VAR_SIZE, SNP_OBJ_KEY_FLOAT, SNP_OBJ_KEY_INT, SNP_OBJ_KEY_UINT
 *        のいずれかを指定
 * @size: SNP_OBJ_KEY_VAR_SIZEの場合は最大長、
 *        それ以外の場合は長さを指定(単位:byte)
 *
 * nameに対応する新たなtype(型)をdbに定義する。
 * (todo: 複合keyを定義するための構造)
 **/
snp_obj *snp_type_create(snp_ctx *ctx, const char *name, unsigned name_size,
                         snp_obj_flags flags, unsigned int size);

/**
 * snp_proc_create:
 * @name: 作成するprocの名前。
 * @type: procの種類。
 * @init: 初期化関数のポインタ
 * @next: 実処理関数のポインタ
 * @fin: 終了関数のポインタ
 *
 * nameに対応する新たなproc(手続き)をctxが使用するdbに定義する。
 **/

typedef struct _snp_proc_ctx snp_proc_ctx;

typedef union {
  int int_value;
  snp_id id;
  void *ptr;
} snp_proc_data;

typedef snp_rc snp_proc_func(snp_ctx *ctx, snp_proc_ctx *pctx,
                             int argc, snp_proc_data *argv);

typedef snp_rc snp_proc_init_func(snp_ctx *ctx, const char *path);

typedef enum {
  SNP_PROC_HOOK,
  SNP_PROC_RECALC,
  SNP_PROC_SCORE,
  SNP_PROC_COMPARE,
  SNP_PROC_GROUP
} snp_proc_type;

snp_obj *snp_proc_create(snp_ctx *ctx,
                         const char *name, unsigned name_size,
                         const char *path, snp_proc_type type,
                         snp_proc_func *init, snp_proc_func *next, snp_proc_func *fin);

/*-------------------------------------------------------------
 * table操作のための関数
 */

#define SNP_TABLE_MAX_KEY_SIZE  4096

/**
 * snp_table_create:
 * @name: 作成するtableの名前。NULLなら無名tableとなる。
 * @path: 作成するtableのファイルパス。
 *        flagsにSNP_OBJ_PERSISTENTが指定されている場合のみ有効。
 *        NULLなら自動的にファイルパスが付与される。
 * @flags: SNP_OBJ_PERSISTENTを指定すると永続tableとなる。
 *         SNP_OBJ_TABLE_PAT_KEY,SNP_OBJ_TABLE_HASH_KEY,SNP_OBJ_TABLE_NO_KEY
 *         のいずれかを指定する。
 *         SNP_OBJ_KEY_NORMALIZEを指定すると正規化された文字列がkeyとなる。
 *         SNP_OBJ_KEY_WITH_SISを指定するとkey文字列の全suffixが自動的に登録される。
 *         SNP_OBJ_TOKEN_MECAB,SNP_OBJ_TOKEN_NGRAM,SNP_OBJ_TOKEN_DELIMITEDは、
 *         作成するtableを語彙表として用いる場合のtokenizeの方法を指定する。
 *         SNP_OBJ_TOKEN_NGRAMを指定した場合に限り、
 *         SNP_OBJ_KEY_SPLIT_ALPHA,SNP_OBJ_KEY_SPLIT_DIGIT,SNP_OBJ_KEY_SPLIT_SYMBOL
 *         を指定して、文字列をN-GRAMに区切る際の方針を指定できる。
 * @key_type: keyの型を指定する。SNP_OBJ_TABLE_NO_KEYが指定された場合は無効。
 *            既存のtypeあるいはtableを指定できる。
 *            key_typeにtable Aを指定してtable Bを作成した場合、Bは必ずAのサブセットとなる。
 * @value_size: keyに対応する値を格納する領域のサイズ(byte長)。tableはcolumnとは別に、
 *              keyに対応する値を格納する領域を一つだけ持つことができる。
 * @encoding: 作成するtableでデフォルトとなるencoding。
 *
 * nameに対応する新たなtableをctxが使用するdbに定義する。
 **/
snp_obj *snp_table_create(snp_ctx *ctx,
                          const char *name, unsigned name_size,
                          const char *path, snp_obj_flags flags,
                          snp_obj *key_type, unsigned value_size, snp_encoding encoding);
/**
 * snp_table_open:
 * @name: 開こうとするtableの名前。NULLなら無名tableとなる。
 * @path: 開こうとするtableのファイルパス。
 *
 * ctxが使用するdbの中でnameに対応付けて既存のtableを開く。
 **/
snp_obj *snp_table_open(snp_ctx *ctx,
                        const char *name, unsigned name_size, const char *path);

typedef unsigned char snp_search_flags;

#define  SNP_SEARCH_EXACT        0
#define  SNP_SEARCH_LCP          1
#define  SNP_SEARCH_SUFFIX       2
#define  SNP_SEARCH_PREFIX       3
#define  SNP_SEARCH_PARTIAL      4
#define  SNP_SEARCH_NEAR         5
#define  SNP_SEARCH_NEAR2        6
#define  SNP_SEARCH_SIMILAR      7
#define  SNP_SEARCH_TERM_EXTRACT 8

#define  SNP_TABLE_ADD      (1L<<6)
#define  SNP_TABLE_ADDED    (1L<<7)

/**
 * snp_table_lookup:
 * @table: 対象table
 * @key: 検索key
 * @flags: SNP_SEARCH_EXACTが指定された場合はkeyに完全一致するrecordを検索する。
 *         SNP_SEARCH_LCPが指定された場合はlongest common prefix searchを行う。
 *         該当するkeyが存在せず、かつSNP_TABLE_ADDが指定された場合は、
 *         tableに該当レコードを追加する。(追加しない場合はSNP_ID_NILを返す)
 *         SNP_TABLE_ADDが指定され、かつ実際にレコードが追加された場合は、
 *         flagsのSNP_TABLE_ADDED bitが立てられる。
 *         flagsにNULLが指定された場合は、SNP_SEARCH_EXACTのみが指定されたものと見なされる。
 *
 * tableからkeyに対応するrecordを検索し、対応するIDを返す。
 **/
snp_id snp_table_lookup(snp_ctx *ctx, snp_obj *table,
                        const void *key, unsigned key_size,
                        snp_search_flags *flags);

/**
 * snp_table_add:
 * @table: 対象table
 *
 * 新しいレコードを追加し、そのIDを返す。
 * SNP_OBJ_TABLE_NO_KEYが指定されたtableでのみ有効。
 **/
snp_id snp_table_add(snp_ctx *ctx, snp_obj *table);

/**
 * snp_table_get_key:
 * @table: 対象table
 * @id: 対象レコードのID
 * @keybuf: keyを格納するバッファ(呼出側で準備する)
 * @buf_size: keybufのサイズ(byte長)
 *
 * tableのIDに対応するレコードのkeyを取得する。対応するレコードが存在する場合はkey長を返す。
 * 見つからない場合は0を返す。
 * 対応するキーの検索に成功し、またbuf_sizeの長さがkey長以上であった場合は、
 * keybufに該当するkeyをコピーする。
 *
 **/
int snp_table_get_key(snp_ctx *ctx, snp_obj *table,
                      snp_id id, void *keybuf, int buf_size);

/**
 * snp_table_delete:
 * @table: 対象table
 * @key: 検索key
 * @key_size: 検索keyのサイズ
 *
 * tableのkeyに対応するレコードを削除する。
 * 対応するレコードが存在しない場合はsnp_invalid_argumentを返す。
 **/
snp_rc snp_table_delete(snp_ctx *ctx, snp_obj *table,
                        const void *key, unsigned key_size);

/**
 * snp_table_delete_by_id:
 * @table: 対象table
 * @id: レコードID
 *
 * tableのkeyに対応するレコードを削除する。
 * 対応するレコードが存在しない場合はsnp_invalid_argumentを返す。
 **/
snp_rc snp_table_delete_by_id(snp_ctx *ctx, snp_obj *table, snp_id id);

/**
 * snp_table_truncate:
 * @table: 対象table
 *
 * tableの全レコードを一括して削除する。
 **/
snp_rc snp_table_truncate(snp_ctx *ctx, snp_obj *table);

typedef snp_obj snp_table_cursor;

#define SNP_CURSOR_DESCENDING 0
#define SNP_CURSOR_ASCENDING  1
#define SNP_CURSOR_GE 0
#define SNP_CURSOR_GT 2
#define SNP_CURSOR_LE 0
#define SNP_CURSOR_LT 4

/**
 * snp_table_cursor_open:
 * @table: 対象table
 * @min: keyの下限 (NULLは下限なしと見なす)
 * @max: keyの上限 (NULLは上限なしと見なす)
 * @flags: SNP_CURSOR_ASCENDINGを指定すると昇順にkeyを取り出す。(指定しなければ降順)
 *         SNP_CURSOR_GTを指定するとminに一致したkeyをcursorの範囲に含まない。
 *         SNP_CURSOR_LTを指定するとmaxに一致したkeyをcursorの範囲に含まない。
 *
 * tableに登録されているレコードを順番に取り出すためのカーソルを生成して返す。
 * SNP_OBJ_TABLE_PAT_KEYを指定したtableではkey順に、
 * SNP_OBJ_TABLE_HASH_KEYを指定したtableではid順にレコードを取り出します。
 **/
snp_table_cursor *snp_table_cursor_open(snp_ctx *ctx, snp_obj *table,
                                        const void *min, unsigned min_size,
                                        const void *max, unsigned max_size,
                                        int flags);

/**
 * snp_table_cursor_close:
 * @tc: 対象cursor
 *
 * snp_table_cursor_openで生成したcursorを解放する。
 **/
snp_rc snp_table_cursor_close(snp_ctx *ctx, snp_table_cursor *tc);

/**
 * snp_table_cursor_next:
 * @tc: 対象cursor
 *
 * cursorのカレントレコードを一件進めてそのIDを返す。
 * cursorの対象範囲の末尾に達するとSNP_ID_NILを返す。
 **/
snp_id snp_table_cursor_next(snp_ctx *ctx, snp_table_cursor *tc);

/**
 * snp_table_cursor_get_key:
 * @tc: 対象cursor
 * @key: カレントレコードのkeyへのポインタがセットされる。
 * cursorのカレントレコードのkeyを@keyにセットし、その長さを返す。
 **/
int snp_table_cursor_get_key(snp_ctx *ctx, snp_table_cursor *tc, void **key);

/**
 * snp_table_cursor_get_value:
 * @tc: 対象cursor
 * @value: カレントレコードのvalueへのポインタがセットされる。
 * cursorのカレントレコードのvalueを@valueにセットし、その長さを返す。
 **/
int snp_table_cursor_get_value(snp_ctx *ctx, snp_table_cursor *tc, void **value);

/**
 * snp_table_cursor_set_value:
 * @tc: 対象cursor
 * @value: 新しいvalueの値。
 * @flags: snp_obj_set_valueのflagsと同様の値を指定できる。
 *
 * cursorのカレントレコードのvalueを引数の内容に置き換える。
 * cursorのカレントレコードが存在しない場合はsnp_invalid_argumentを返す。
 **/
snp_rc snp_table_cursor_set_value(snp_ctx *ctx, snp_table_cursor *tc,
                                  void *value, int flags);

/**
 * snp_table_cursor_delete:
 * @tc: 対象cursor
 *
 * cursorのカレントレコードを削除する。
 * cursorのカレントレコードが存在しない場合はsnp_invalid_argumentを返す。
 **/
snp_rc snp_table_cursor_delete(snp_ctx *ctx, snp_table_cursor *tc);

#define SNP_TABLE_EACH(ctx,table,head,tail,id,key,key_size,value,block) { \
  (ctx)->errlvl = SNP_OK;\
  (ctx)->rc = snp_success;\
  if ((ctx)->seqno & 1) {\
    (ctx)->subno++;\
  } else {\
    (ctx)->seqno++;\
  }\
  if (table) {\
    switch ((table)->header.type) {\
    case SNP_TABLE_PAT_KEY :\
      SNP_PAT_EACH((snp_pat *)(table), (id), (key), (key_size), (value), block);\
      break;\
    case SNP_TABLE_HASH_KEY :\
      SNP_HASH_EACH((snp_hash *)(table), (id), (key), (key_size), (value), block);\
      break;\
    case SNP_TABLE_NO_KEY :\
      SNP_ARRAY_EACH((snp_array *)(table), (head), (tail), (id), (value), block);\
      break;\
    }\
  }\
  if (ctx->subno) {\
    ctx->subno--;\
  } else {\
    ctx->seqno++;\
  }\
}

/**
 * snp_table_sort:
 * @table: 対象table
 * @limit: resに格納するレコードの上限
 * @result: 結果を格納するtable
 * @keys: ソートキー配列へのポインタ
 * @n_keys: ソートキー配列のサイズ
 *
 * table内のレコードをソートし、上位limit個の要素をresultに格納する。
 * keysには、tableのcolumn,accessor,procのいずれかが指定できる。
 **/

typedef struct _snp_table_sort_key snp_table_sort_key;
typedef unsigned char snp_table_sort_flags;

#define SNP_TABLE_SORT_DESC        0
#define SNP_TABLE_SORT_ASC         (1L<<0)

struct _snp_table_sort_key {
  snp_obj *key;
  snp_table_sort_flags flags;
  int offset;
};

int snp_table_sort(snp_ctx *ctx, snp_obj *table, int limit,
                   snp_obj *result, snp_table_sort_key *keys, int n_keys);

/**
 * snp_table_group:
 * @table: 対象table
 * @keys: group化キー構造体の配列へのポインタ
 * @n_keys: group化キー構造体の配列のサイズ
 * @results: group化の結果を格納する構造体の配列へのポインタ
 * @n_results:group化の結果を格納する構造体の配列のサイズ
 *
 * tableのレコードを特定の条件でグループ化する
 **/

typedef struct _snp_table_group_result snp_table_group_result;
typedef unsigned int snp_table_group_flags;

#define SNP_TABLE_GROUP_CALC_COUNT       (1L<<3)
#define SNP_TABLE_GROUP_CALC_MAX         (1L<<4)
#define SNP_TABLE_GROUP_CALC_MIN         (1L<<5)
#define SNP_TABLE_GROUP_CALC_SUM         (1L<<6)
#define SNP_TABLE_GROUP_CALC_AVG         (1L<<7)

typedef enum {
  snp_sel_or = 0,
  snp_sel_and,
  snp_sel_but,
  snp_sel_adjust
} snp_sel_operator;

struct _snp_table_group_result {
  snp_obj *table;
  unsigned char key_begin;
  unsigned char key_end;
  int limit;
  snp_table_group_flags flags;
  snp_sel_operator op;
};

snp_rc snp_table_group(snp_ctx *ctx, snp_obj *table,
                       snp_table_sort_key *keys, int n_keys,
                       snp_table_group_result *results, int n_results);

/**
 * snp_table_setoperation:
 * @table1: 対象table1
 * @table2: 対象table2
 * @res: 結果を格納するtable
 *
 * table1とtable2をopの指定に従って集合演算した結果をresに格納する。
 * resにtable1あるいはtable2そのものを指定した場合を除けば、table1, table2は破壊されない。
 **/
snp_rc snp_table_setoperation(snp_ctx *ctx, snp_obj *table1, snp_obj *table2,
                              snp_obj *res, snp_sel_operator op);

/**
 * snp_table_difference:
 * @table1: 対象table1
 * @table2: 対象table2
 * @res1: 結果を格納するtable
 * @res2: 結果を格納するtable
 *
 * table1とtable2から重複するレコードを取り除いた結果をそれぞれres1, res2に格納する。
 **/
snp_rc snp_table_difference(snp_ctx *ctx, snp_obj *table1, snp_obj *table2,
                            snp_obj *res1, snp_obj *res2);

/**
 * snp_table_column:
 * @table: 対象table
 * @name: カラム名
 *
 * nameに対応するtableのカラムを返す。対応するカラムが存在しなければNULLを返す。
 **/
snp_obj *snp_table_column(snp_ctx *ctx, snp_obj *table,
                          const char *name, unsigned name_size);


/**
 * snp_table_size:
 * @table: 対象table
 *
 * tableに登録されているレコードの件数を返す。
 **/
unsigned int snp_table_size(snp_ctx *ctx, snp_obj *table);

/*-------------------------------------------------------------
 * column操作のための関数
 */

/**
 * snp_column_create:
 * @table: 対象table
 * @name: カラム名
 * @path: カラムを格納するファイルパス。
 *        flagsにSNP_OBJ_PERSISTENTが指定されている場合のみ有効。
 *        NULLなら自動的にファイルパスが付与される。
 * @flags: SNP_OBJ_PERSISTENTを指定すると永続columnとなる。
 *         SNP_OBJ_COLUMN_INDEXを指定すると転置インデックスとなる。
 * @type: カラム値の型。定義済みのtypeあるいはtableを指定できる。
 *
 * tableに新たなカラムを定義する。nameは省略できない。
 * 一つのtableに同一のnameのcolumnを複数定義することはできない。
 **/
snp_obj *snp_column_create(snp_ctx *ctx, snp_obj *table,
                           const char *name, unsigned name_size,
                           const char *path, snp_obj_flags flags, snp_obj *type);

/**
 * snp_column_open:
 * @table: 対象table
 * @name: カラム名
 * @path: カラムを格納するファイルパス。
 * @type: カラム値の型。
 *
 * 既存の永続的なcolumnを、tableのnameに対応するcolumnとして開く
 **/
snp_obj *snp_column_open(snp_ctx *ctx, snp_obj *table,
                         const char *name, unsigned name_size,
                         const char *path, snp_obj *type);

/**
 * snp_column_index_update
 * @column: 対象column
 * @id: 対象レコードのID
 * @section: 対象レコードのセクション番号
 * @oldvalue: 更新前の値
 * @newvalue: 更新後の値
 *
 * oldvalue, newvalueの値から得られるキーに対応するcolumnの値の中の、
 * id, sectionに対応するエントリを更新する。
 * columnはSNP_OBJ_COLUMN_INDEX型のカラムでなければならない。
 **/
snp_rc snp_column_index_update(snp_ctx *ctx, snp_obj *column,
                               snp_id id, unsigned int section,
                               snp_obj *oldvalue, snp_obj *newvalue);

/**
 * snp_column_table:
 * @column: 対象column
 *
 * columnが属するtableを返す。
 **/
snp_obj *snp_column_table(snp_ctx *ctx, snp_obj *column);

/*-------------------------------------------------------------
 * db, table, columnの全てまたは幾つかで共通に使用できる関数
 */

#define SNP_PROC_MAX_ARGS 256

typedef enum {
  SNP_INFO_ENCODING = 0,
  SNP_INFO_SOURCE,
  SNP_INFO_ELEMENT_SIZE,
  SNP_INFO_CURR_MAX,
  SNP_INFO_MAX_ELEMENT_SIZE,
  SNP_INFO_SEG_SIZE,
  SNP_INFO_CHUNK_SIZE,
  SNP_INFO_INITIAL_N_SEGMENTS,
  SNP_INFO_MAX_SECTION,
  SNP_INFO_HOOK_LOCAL_DATA,
  SNP_INFO_ELEMENT_A,
  SNP_INFO_ELEMENT_CHUNK,
  SNP_INFO_ELEMENT_CHUNK_SIZE,
  SNP_INFO_ELEMENT_BUFFER_FREE,
  SNP_INFO_ELEMENT_NTERMS,
  SNP_INFO_ELEMENT_NTERMS_VOID,
  SNP_INFO_ELEMENT_SIZE_IN_CHUNK,
  SNP_INFO_ELEMENT_POS_IN_CHUNK,
  SNP_INFO_ELEMENT_SIZE_IN_BUFFER,
  SNP_INFO_ELEMENT_POS_IN_BUFFER,
  SNP_INFO_ELEMENT_ESTIMATE_SIZE,
  SNP_INFO_NGRAM_UNIT_SIZE,
  SNP_INFO_VERSION,
  SNP_INFO_CONFIGURE_OPTIONS,
  SNP_INFO_CONFIG_PATH,
  SNP_INFO_PARTIAL_MATCH_THRESHOLD
} snp_info_type;

/**
 * snp_obj_get_info:
 * @obj: 対象obj
 * @type: 取得する情報の種類
 * @valuebuf: 値を格納するバッファ(呼出側で準備)
 *
 * objのtypeに対応する情報をvaluebufに格納する。
 **/
snp_obj *snp_obj_get_info(snp_ctx *ctx, snp_obj *obj, snp_info_type type, snp_obj *valuebuf);

/**
 * snp_obj_set_info:
 * @obj: 対象obj
 * @type: 設定する情報の種類
 * @value: 設定しようとする値
 *
 * objのtypeに対応する情報をvalueの内容に更新する。
 **/
snp_rc snp_obj_set_info(snp_ctx *ctx, snp_obj *obj, snp_info_type type, snp_obj *value);

/**
 * snp_obj_get_element_info:
 * @obj: 対象obj
 * @id: 対象ID
 * @type: 取得する情報の種類
 * @value: 値を格納するバッファ(呼出側で準備)
 *
 * objのidに対応するレコードの、typeに対応する情報をvaluebufに格納する。
 * 呼出側ではtypeに応じて十分なサイズのバッファを確保しなければならない。
 **/
snp_obj *snp_obj_get_element_info(snp_ctx *ctx, snp_obj *obj, snp_id id,
                                  snp_info_type type, snp_obj *value);

/**
 * snp_obj_set_element_info:
 * @obj: 対象object
 * @id: 対象ID
 * @type: 設定する情報の種類
 * @value: 設定しようとする値
 *
 * objのidに対応するレコードのtypeに対応する情報をvalueの内容に更新する。
 **/
snp_rc snp_obj_set_element_info(snp_ctx *ctx, snp_obj *obj, snp_id id,
                                snp_info_type type, snp_obj *value);


/**
 * snp_obj_get_value:
 * @obj: 対象object
 * @id: 対象レコードのID
 * @value: 値を格納するバッファ(呼出側で準備する)
 *
 * tableのIDに対応するレコードのvalueを取得する。
 * 対応するレコードが存在する場合はvalue長を返す。見つからない場合は0を返す。
 **/
snp_obj *snp_obj_get_value(snp_ctx *ctx, snp_obj *obj, snp_id id, snp_obj *value);

/**
 * snp_obj_set_value:
 * @obj: 対象object
 * @id: 対象レコードのID
 * @value: 格納する値
 * @flags: 以下の値を指定できる
 *  SNP_OBJ_SET: レコードの値をvalueと置き換える。
 *  SNP_OBJ_INCR: レコードの値にvalueを加算する。
 *  SNP_OBJ_DECR: レコードの値にvalueを減算する。
 *  SNP_OBJ_APPEND: レコードの値の末尾にvalueを追加する。
 *  SNP_OBJ_PREPEND: レコードの値の先頭にvalueを追加する。
 *  SNP_OBJ_GET: 新しいレコードの値をvalueにセットする。
 *  SNP_OBJ_COMPARE: レコードの値とvalueが等しいか調べる。
 *  SNP_OBJ_LOCK: 当該レコードをロックする。SNP_OBJ_COMPAREと共に指定された場合は、
 *                レコードの値とvalueが等しい場合に限ってロックする。
 *  SNP_OBJ_UNLOCK: 当該レコードのロックを解除する。
 *
 * objのIDに対応するレコードの値を更新する。
 * 対応するレコードが存在しない場合はsnp_invalid_argumentを返す。
 **/

#define SNP_OBJ_SET_MASK   (0x07L)
#define SNP_OBJ_SET        (0x01L)
#define SNP_OBJ_INCR       (0x02L)
#define SNP_OBJ_DECR       (0x03L)
#define SNP_OBJ_APPEND     (0x04L)
#define SNP_OBJ_PREPEND    (0x05L)
#define SNP_OBJ_GET        (1L<<4)
#define SNP_OBJ_COMPARE    (1L<<5)
#define SNP_OBJ_LOCK       (1L<<6)
#define SNP_OBJ_UNLOCK     (1L<<7)

snp_rc snp_obj_set_value(snp_ctx *ctx, snp_obj *obj, snp_id id, snp_obj *value, int flags);

/**
 * snp_obj_remove:
 * @path: objectに該当するファイルパス
 *
 * pathに該当するオブジェクトのファイル一式を削除する。
 **/
snp_rc snp_obj_remove(snp_ctx *ctx, const char *path);

/**
 * snp_obj_rename:
 * @old_path: 旧ファイルパス
 * @new_path: 新ファイルパス
 *
 * old_pathに該当するオブジェクトのファイル名をnew_pathに変更する。
 **/
snp_rc snp_obj_rename(snp_ctx *ctx, const char *old_path, const char *new_path);

/**
 * snp_obj_close:
 * @obj: 対象object
 *
 * objをメモリから解放する。objに属するobjectも再帰的にメモリから解放される。
 **/
snp_rc snp_obj_close(snp_ctx *ctx, snp_obj *obj);

/**
 * snp_obj_path:
 * @obj: 対象object
 *
 * objに対応するファイルパスを返す。一時objectならNULLを返す。
 **/
const char *snp_obj_path(snp_ctx *ctx, snp_obj *obj);

/**
 * snp_obj_name:
 * @obj: 対象object
 * @namebuf: nameを格納するバッファ(呼出側で準備する)
 * @buf_size: namebufのサイズ(byte長)
 *
 * objの名前を返す。無名objectなら0を返す。
 * 名前付きのobjectであり、buf_sizeの長さがname長以上であった場合は、
 * namebufに該当するnameをコピーする。
 **/
int snp_obj_name(snp_ctx *ctx, snp_obj *obj, char *namebuf, int buf_size);

/**
 * snp_obj_expire:
 * @obj: 対象object
 *
 * objの占有するメモリのうち、可能な領域をthresholdを指標として解放する。
 **/
int snp_obj_expire(snp_ctx *ctx, snp_obj *obj, int threshold);

/**
 * snp_obj_check:
 * @obj: 対象object
 *
 * objに対応するファイルの整合性を検査する。
 **/
int snp_obj_check(snp_ctx *ctx, snp_obj *obj);

/**
 * snp_obj_lock:
 * @obj: 対象object
 *
 * objをlockする。timeout(秒)経過してもlockを取得できない場合はsnp_other_errorを返す。
 **/
snp_rc snp_obj_lock(snp_ctx *ctx, snp_obj *obj, snp_id id, int timeout);

/**
 * snp_obj_unlock:
 * @obj: 対象object
 *
 * objをunlockする。
 **/
snp_rc snp_obj_unlock(snp_ctx *ctx, snp_obj *obj, snp_id id);

/**
 * snp_obj_clear_lock:
 * @obj: 対象object
 *
 * 強制的にロックをクリアする。
 **/
snp_rc snp_obj_clear_lock(snp_ctx *ctx, snp_obj *obj);

/**
 * snp_obj_is_locked;
 * @obj: 対象object
 *
 * objが現在lockされていれば0以外の値を返す。
 **/
unsigned int snp_obj_is_locked(snp_ctx *ctx, snp_obj *obj);

/**
 * snp_obj_db:
 * @obj: 対象object
 *
 * objの属するdbを返す。
 **/
snp_obj *snp_obj_db(snp_ctx *ctx, snp_obj *obj);

/**
 * snp_obj_search:
 * @obj: 検索対象のobject
 * @query: 検索クエリ
 * @res: 検索結果を格納するテーブル
 * @op: snp_sel_or, snp_sel_and, snp_sel_but, snp_sel_adjustのいずれかを指定する
 * @optargs: 詳細検索条件
 *
 * objを対象としてqueryにマッチするレコードを検索し、
 * opの指定に従ってresにレコードを追加あるいは削除する。
 **/
typedef struct _snp_search_optarg snp_search_optarg;

struct _snp_search_optarg {
  snp_search_flags flags;
  int similarity_threshold;
  int max_interval;
  int *weight_vector;
  int vector_size;
  snp_obj *proc;
  int max_size;
};

snp_rc snp_obj_search(snp_ctx *ctx, snp_obj *obj, snp_obj *query,
                      snp_obj *res, snp_sel_operator op, snp_search_optarg *optarg);

snp_rc snp_verses_add(snp_ctx *ctx, snp_obj *verses,
                      const char *str, unsigned int str_len,
                      unsigned int weight, snp_id lang);

/*-------------------------------------------------------------
 * hook操作のための関数
 */

int snp_proc_call_next(snp_ctx *ctx, snp_obj *exec_info, snp_obj *in, snp_obj *out);
void *snp_proc_get_ctx_local_data(snp_ctx *ctx, snp_obj *exec_info);
void *snp_proc_get_hook_local_data(snp_ctx *ctx, snp_obj *exec_info);

typedef enum {
  SNP_HOOK_SET = 0,
  SNP_HOOK_GET,
  SNP_HOOK_INSERT,
  SNP_HOOK_DELETE,
  SNP_HOOK_SELECT
} snp_hook_entry;

/**
 * snp_obj_add_hook:
 * @obj: 対象object
 * @entry: SNP_HOOK_GETは、objectの参照時に呼び出されるhookを定義する。
          SNP_HOOK_SETは、objectの更新時に呼び出されるhookを定義する。
          SNP_HOOK_SELECTは、検索処理の実行中に適時呼び出され、
          処理の実行状況を調べたり、実行の中断を指示することができる。
 * @offset: hookの実行順位。offsetに対応するhookの直前に新たなhookを挿入する。
            0を指定した場合は先頭に挿入される。-1を指定した場合は末尾に挿入される。
            objectに複数のhookが定義されている場合は順位の順に呼び出される。
 * @proc: 手続き
 * @data: hook固有情報
 *
 * objに対してhookを追加する。
 **/
snp_rc snp_obj_add_hook(snp_ctx *ctx, snp_obj *obj, snp_hook_entry entry,
                        int offset, snp_obj *proc, snp_obj *data);

/**
 * snp_obj_get_hook:
 * @obj: 対象object
 * @entry: hookタイプ
 * @offset: 実行順位
 * @data: hook固有情報格納バッファ
 *
 * objに定義されているhookの手続き(proc)を返す。hook固有情報が定義されている場合は、
 * その内容をdataにコピーして返す。
 **/
snp_obj *snp_obj_get_hook(snp_ctx *ctx, snp_obj *obj, snp_hook_entry entry,
                          int offset, snp_obj *data);

/**
 * snp_obj_delete_hook:
 * @obj: 対象object
 * @entry: hookタイプ
 * @offset: 実行順位
 *
 * objに定義されているhookを削除する。
 **/
snp_rc snp_obj_delete_hook(snp_ctx *ctx, snp_obj *obj, snp_hook_entry entry, int offset);

snp_obj *snp_obj_open(snp_ctx *ctx, unsigned char type, snp_obj_flags flags);

/* query & snippet */

#ifndef SNP_QUERY_AND
#define SNP_QUERY_AND '+'
#endif /* SNP_QUERY_AND */
#ifndef SNP_QUERY_BUT
#define SNP_QUERY_BUT '-'
#endif /* SNP_QUERY_BUT */
#ifndef SNP_QUERY_ADJ_INC
#define SNP_QUERY_ADJ_INC '>'
#endif /* SNP_QUERY_ADJ_POS2 */
#ifndef SNP_QUERY_ADJ_DEC
#define SNP_QUERY_ADJ_DEC '<'
#endif /* SNP_QUERY_ADJ_POS1 */
#ifndef SNP_QUERY_ADJ_NEG
#define SNP_QUERY_ADJ_NEG '~'
#endif /* SNP_QUERY_ADJ_NEG */
#ifndef SNP_QUERY_PREFIX
#define SNP_QUERY_PREFIX '*'
#endif /* SNP_QUERY_PREFIX */
#ifndef SNP_QUERY_PARENL
#define SNP_QUERY_PARENL '('
#endif /* SNP_QUERY_PARENL */
#ifndef SNP_QUERY_PARENR
#define SNP_QUERY_PARENR ')'
#endif /* SNP_QUERY_PARENR */
#ifndef SNP_QUERY_QUOTEL
#define SNP_QUERY_QUOTEL '"'
#endif /* SNP_QUERY_QUOTEL */
#ifndef SNP_QUERY_QUOTER
#define SNP_QUERY_QUOTER '"'
#endif /* SNP_QUERY_QUOTER */
#ifndef SNP_QUERY_ESCAPE
#define SNP_QUERY_ESCAPE '\\'
#endif /* SNP_QUERY_ESCAPE */

typedef struct _snp_snip snp_snip;
typedef struct _snp_query snp_query;
typedef struct _snp_snip_mapping snp_snip_mapping;

struct _snp_snip_mapping {
  void *dummy;
};

snp_query *snp_query_open(snp_ctx *ctx, const char *str, unsigned int str_len,
                          snp_sel_operator default_op,
                          int max_exprs, snp_encoding encoding);
unsigned int snp_query_rest(snp_ctx *ctx, snp_query *q, const char ** const rest);
snp_rc snp_query_close(snp_ctx *ctx, snp_query *q);

snp_rc snp_query_scan(snp_ctx *ctx, snp_query *q, const char **strs, unsigned int *str_lens,
                      unsigned int nstrs, int flags, int *found, int *score);
snp_snip *snp_query_snip(snp_ctx *ctx, snp_query *query, int flags,
                         unsigned int width, unsigned int max_results,
                         unsigned int n_tags,
                         const char **opentags, unsigned int *opentag_lens,
                         const char **closetags, unsigned int *closetag_lens,
                         snp_snip_mapping *mapping);

#define SNP_SNIP_NORMALIZE                      0x0001
#define SNP_SNIP_COPY_TAG                       0x0002
#define SNP_SNIP_SKIP_LEADING_SPACES            0x0004
#define SNP_QUERY_SCAN_NORMALIZE                SNP_SNIP_NORMALIZE

snp_snip *snp_snip_open(snp_ctx *ctx, snp_encoding encoding, int flags, unsigned int width,
                        unsigned int max_results,
                        const char *defaultopentag, unsigned int defaultopentag_len,
                        const char *defaultclosetag, unsigned int defaultclosetag_len,
                        snp_snip_mapping *mapping);
snp_rc snp_snip_close(snp_ctx *ctx, snp_snip *snip);
snp_rc snp_snip_add_cond(snp_ctx *ctx, snp_snip *snip,
                         const char *keyword, unsigned int keyword_len,
                         const char *opentag, unsigned int opentag_len,
                         const char *closetag, unsigned int closetag_len);
snp_rc snp_snip_exec(snp_ctx *ctx, snp_snip *snip,
                     const char *string, unsigned int string_len,
                     unsigned int *nresults, unsigned int *max_tagged_len);
snp_rc snp_snip_get_result(snp_ctx *ctx, snp_snip *snip, const unsigned int index,
                           char *result, unsigned int *result_len);

/* log */

#define SNP_LOG_TIME      1
#define SNP_LOG_TITLE     2
#define SNP_LOG_MESSAGE   4
#define SNP_LOG_LOCATION  8

typedef struct _snp_logger_info snp_logger_info;

struct _snp_logger_info {
  snp_log_level max_level;
  int flags;
  void (*func)(int, const char *, const char *, const char *, const char *, void *);
  void *func_arg;
};

snp_rc snp_logger_info_set(snp_ctx *ctx, const snp_logger_info *info);

void snp_logger_put(snp_ctx *ctx, snp_log_level level,
                    const char *file, int line, const char *func, char *fmt, ...);

int snp_logger_pass(snp_ctx *ctx, snp_log_level level);

#define SNP_LOG(level,...) \
if (snp_logger_pass(ctx, level)) {\
  snp_logger_put(ctx, (level), __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__); \
}

#ifndef SNP_LOG_DEFAULT_LEVEL
#define SNP_LOG_DEFAULT_LEVEL snp_log_notice
#endif /* SNP_LOG_DEFAULT_LEVEL */

#define snp_log(...) \
if (snp_logger_pass(ctx, SNP_LOG_DEFAULT_LEVEL)) {\
  snp_logger_put(ctx, SNP_LOG_DEFAULT_LEVEL, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__); \
}

/* string & bulk */

snp_rc snp_bulk_init(snp_ctx *ctx, snp_obj *bulk, unsigned int size);
snp_rc snp_bulk_reinit(snp_ctx *ctx, snp_obj *bulk, unsigned int size);
snp_rc snp_bulk_resize(snp_ctx *ctx, snp_obj *bulk, unsigned int newsize);
snp_rc snp_bulk_write(snp_ctx *ctx, snp_obj *bulk,
                      const char *str, unsigned int len);
snp_rc snp_bulk_reserve(snp_ctx *ctx, snp_obj *bulk, unsigned int len);
snp_rc snp_bulk_space(snp_ctx *ctx, snp_obj *bulk, unsigned int len);
snp_rc snp_bulk_itoa(snp_ctx *ctx, snp_obj *bulk, int i);
snp_rc snp_bulk_lltoa(snp_ctx *ctx, snp_obj *bulk, long long int i);
snp_rc snp_bulk_ftoa(snp_ctx *ctx, snp_obj *bulk, double d);
snp_rc snp_bulk_itoh(snp_ctx *ctx, snp_obj *bulk, int i, unsigned int len);
snp_rc snp_bulk_itob(snp_ctx *ctx, snp_obj *bulk, snp_id id);
snp_rc snp_bulk_lltob32h(snp_ctx *ctx, snp_obj *bulk, long long int i);
snp_rc snp_bulk_fin(snp_ctx *ctx, snp_obj *bulk);
snp_rc snp_bulk_benc(snp_ctx *ctx, snp_obj *bulk, unsigned int v);
snp_rc snp_bulk_esc(snp_ctx *ctx, snp_obj *bulk, const char *s,
                    unsigned int len, snp_encoding encoding);
snp_rc snp_bulk_urlenc(snp_ctx *ctx, snp_obj *buf,
                       const char *str, unsigned int len);

#define SNP_BULK_PUTS(ctx,bulk,str) (snp_bulk_write((ctx), (bulk), (str), strlen(str)))
#define SNP_BULK_PUTC(ctx,bulk,c) { char _c = (c); snp_bulk_write((ctx), (bulk), &_c, 1); }
#define SNP_BULK_REWIND(bulk) ((bulk)->u.b.curr = (bulk)->u.b.head)
#define SNP_BULK_WSIZE(bulk) ((bulk)->u.b.tail - (bulk)->u.b.head)
#define SNP_BULK_REST(bulk) ((bulk)->u.b.tail - (bulk)->u.b.curr)
#define SNP_BULK_VSIZE(bulk) ((bulk)->u.b.curr - (bulk)->u.b.head)
#define SNP_BULK_EMPTYP(bulk) ((bulk)->u.b.curr == (bulk)->u.b.head)
#define SNP_BULK_HEAD(bulk) ((bulk)->u.b.head)

/* flags for snp_str_normalize */
#define SNP_STR_REMOVEBLANK 1
#define SNP_STR_WITH_CTYPES 2
#define SNP_STR_WITH_CHECKS 4
int snp_str_normalize(snp_ctx *ctx, const char *str, unsigned int str_len,
                      snp_encoding encoding, int flags,
                      char *nstrbuf, int buf_size);
unsigned int snp_str_charlen(snp_ctx *ctx, const char *str, snp_encoding encoding);

/* ql */

#define SNP_QL_MORE  0x01
#define SNP_QL_TAIL  0x02
#define SNP_QL_HEAD  0x04
#define SNP_QL_QUIET 0x08
#define SNP_QL_QUIT  0x10
#define SNP_QL_FIN   SNP_CTX_FIN

snp_rc snp_ql_connect(snp_ctx *ctx, const char *host, int port, int flags);
snp_rc snp_ql_load(snp_ctx *ctx, const char *path);
snp_rc snp_ql_send(snp_ctx *ctx, char *str, unsigned int str_len, int flags);
snp_rc snp_ql_recv(snp_ctx *ctx, char **str, unsigned int *str_len, int *flags);

typedef struct _snp_ql_info snp_ql_info;

struct _snp_ql_info {
  int fd;
  unsigned int com_status;
  unsigned int com_info;
  snp_obj *outbuf;
  unsigned char stat;
};

snp_rc snp_ql_info_get(snp_ctx *ctx, snp_ql_info *info);

#ifdef __cplusplus
}
#endif

#endif /* GROONGA_H */
