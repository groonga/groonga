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

#ifndef GRN_API
#if defined(_WIN32) || defined(_WIN64)
#define GRN_API __declspec(dllimport)
#else
#define GRN_API
#endif /* defined(_WIN32) || defined(_WIN64) */
#endif /* GRN_API */

typedef unsigned grn_id;

#define GRN_ID_NIL                     (0x00)
#define GRN_ID_MAX                     (0x3fffffff)

typedef enum {
  GRN_SUCCESS = 0,
  GRN_END_OF_DATA = 1,
  GRN_UNKNOWN_ERROR = -1,
  GRN_OPERATION_NOT_PERMITTED = -2,
  GRN_NO_SUCH_FILE_OR_DIRECTORY = -3,
  GRN_NO_SUCH_PROCESS = -4,
  GRN_INTERRUPTED_FUNCTION_CALL = -5,
  GRN_INPUT_OUTPUT_ERROR = -6,
  GRN_NO_SUCH_DEVICE_OR_ADDRESS = -7,
  GRN_ARG_LIST_TOO_LONG = -8,
  GRN_EXEC_FORMAT_ERROR = -9,
  GRN_BAD_FILE_DESCRIPTOR = -10,
  GRN_NO_CHILD_PROCESSES = -11,
  GRN_RESOURCE_TEMPORARILY_UNAVAILABLE = -12,
  GRN_NOT_ENOUGH_SPACE = -13,
  GRN_PERMISSION_DENIED = -14,
  GRN_BAD_ADDRESS = -15,
  GRN_RESOURCE_BUSY = -16,
  GRN_FILE_EXISTS = -17,
  GRN_IMPROPER_LINK = -18,
  GRN_NO_SUCH_DEVICE = -19,
  GRN_NOT_A_DIRECTORY = -20,
  GRN_IS_A_DIRECTORY = -21,
  GRN_INVALID_ARGUMENT = -22,
  GRN_TOO_MANY_OPEN_FILES_IN_SYSTEM = -23,
  GRN_TOO_MANY_OPEN_FILES = -24,
  GRN_INAPPROPRIATE_I_O_CONTROL_OPERATION = -25,
  GRN_FILE_TOO_LARGE = -26,
  GRN_NO_SPACE_LEFT_ON_DEVICE = -27,
  GRN_INVALID_SEEK = -28,
  GRN_READ_ONLY_FILE_SYSTEM = -29,
  GRN_TOO_MANY_LINKS = -30,
  GRN_BROKEN_PIPE = -31,
  GRN_DOMAIN_ERROR = -32,
  GRN_RESULT_TOO_LARGE = -33,
  GRN_RESOURCE_DEADLOCK_AVOIDED = -34,
  GRN_NO_MEMORY_AVAILABLE = -35,
  GRN_FILENAME_TOO_LONG = -36,
  GRN_NO_LOCKS_AVAILABLE = -37,
  GRN_FUNCTION_NOT_IMPLEMENTED = -38,
  GRN_DIRECTORY_NOT_EMPTY = -39,
  GRN_ILLEGAL_BYTE_SEQUENCE = -40,
  GRN_SOCKET_NOT_INITIALIZED = -41,
  GRN_OPERATION_WOULD_BLOCK = -42,
  GRN_ADDRESS_IS_NOT_AVAILABLE = -43,
  GRN_NETWORK_IS_DOWN = -44,
  GRN_NO_BUFFER = -45,
  GRN_SOCKET_IS_ALREADY_CONNECTED = -46,
  GRN_SOCKET_IS_NOT_CONNECTED = -47,
  GRN_SOCKET_IS_ALREADY_SHUTDOWNED = -48,
  GRN_OPERATION_TIMEOUT = -49,
  GRN_CONNECTION_REFUSED = -50,
  GRN_RANGE_ERROR = -51,
  GRN_TOKENIZER_ERROR = -52,
  GRN_FILE_CORRUPT = -53,
  GRN_INVALID_FORMAT = -54,
  GRN_OBJECT_CORRUPT = -55,
  GRN_TOO_MANY_SYMBOLIC_LINKS = -56,
  GRN_NOT_SOCKET = -57,
  GRN_OPERATION_NOT_SUPPORTED = -58,
  GRN_ADDRESS_IS_IN_USE = -59,
  GRN_ZLIB_ERROR = -60,
  GRN_LZO_ERROR = -61,
} grn_rc;

GRN_API grn_rc grn_init(void);
GRN_API grn_rc grn_fin(void);

typedef enum {
  GRN_ENC_DEFAULT = 0,
  GRN_ENC_NONE,
  GRN_ENC_EUC_JP,
  GRN_ENC_UTF8,
  GRN_ENC_SJIS,
  GRN_ENC_LATIN1,
  GRN_ENC_KOI8R
} grn_encoding;

typedef enum {
  GRN_LOG_NONE = 0,
  GRN_LOG_EMERG,
  GRN_LOG_ALERT,
  GRN_LOG_CRIT,
  GRN_LOG_ERROR,
  GRN_LOG_WARNING,
  GRN_LOG_NOTICE,
  GRN_LOG_INFO,
  GRN_LOG_DEBUG,
  GRN_LOG_DUMP
} grn_log_level;

typedef struct _grn_ctx grn_ctx;

#define GRN_CTX_MSGSIZE                (0x80)
#define GRN_CTX_FIN                    (0xff)

struct _grn_ctx {
  grn_rc rc;
  int flags;
  grn_encoding encoding;
  unsigned char ntrace;
  unsigned char errlvl;
  unsigned char stat;
  unsigned int seqno;
  unsigned int subno;
  unsigned int seqno2;
  unsigned int errline;
  grn_ctx *prev;
  grn_ctx *next;
  const char *errfile;
  const char *errfunc;
  struct _grn_ctx_impl *impl;
  void *trace[16];
  char errbuf[GRN_CTX_MSGSIZE];
};

/**
 * grn_ctx_init:
 * @ctx: 初期化するctx構造体へのポインタを指定します。
 * @flags: 初期化するctxのオプションを指定します。
 * GRN_CTX_USE_QLを指定すると、groonga qlインタプリタを実行可能なctxを初期化します。
 * GRN_CTX_USE_QL|GRN_CTX_BATCH_MODEを指定すると、batchmodeでインタプリタを初期化します。
 *
 * ctxを初期化します。
 **/

#define GRN_CTX_USE_QL                 (0x03)
#define GRN_CTX_BATCH_MODE             (0x04)

GRN_API grn_rc grn_ctx_init(grn_ctx *ctx, int flags);

/**
 * grn_ctx_fin:
 * @ctx: 解放するctx構造体へのポインタを指定します。
 *
 * ctxの管理するメモリを解放し、使用を終了します。
 **/
GRN_API grn_rc grn_ctx_fin(grn_ctx *ctx);

/**
 * grn_get_default_encoding:
 *
 * デフォルトのencodingを返します。
 **/
GRN_API grn_encoding grn_get_default_encoding(void);

/**
 * grn_get_default_encoding:
 * @encoding: 変更後のデフォルトのencodingを指定します。
 *
 * デフォルトのencodingを変更します。
 **/
GRN_API grn_rc grn_set_default_encoding(grn_encoding encoding);

#define GRN_CTX_GET_ENCODING(ctx) ((ctx)->encoding)
#define GRN_CTX_SET_ENCODING(ctx,enc) \
  ((ctx)->encoding = (enc == GRN_ENC_DEFAULT) ? grn_get_default_encoding() : enc)

/* obj */

typedef unsigned short int grn_obj_flags;

#define GRN_OBJ_TABLE_TYPE_MASK        (0x07)
#define GRN_OBJ_TABLE_HASH_KEY         (0x00)
#define GRN_OBJ_TABLE_PAT_KEY          (0x01)
#define GRN_OBJ_TABLE_NO_KEY           (0x03)
#define GRN_OBJ_TABLE_ALIAS            (0x04)

#define GRN_OBJ_KEY_MASK               (0x07<<3)
#define GRN_OBJ_KEY_UINT               (0x00<<3)
#define GRN_OBJ_KEY_INT                (0x01<<3)
#define GRN_OBJ_KEY_FLOAT              (0x02<<3)

#define GRN_OBJ_KEY_WITH_SIS           (0x01<<6)
#define GRN_OBJ_KEY_NORMALIZE          (0x01<<7)

#define GRN_OBJ_COLUMN_TYPE_MASK       (0x07)
#define GRN_OBJ_COLUMN_SCALAR          (0x00)
#define GRN_OBJ_COLUMN_VECTOR          (0x01)
#define GRN_OBJ_COLUMN_INDEX           (0x02)

#define GRN_OBJ_COMPRESS_MASK          (0x07<<4)
#define GRN_OBJ_COMPRESS_NONE          (0x00<<4)
#define GRN_OBJ_COMPRESS_ZLIB          (0x01<<4)
#define GRN_OBJ_COMPRESS_LZO           (0x02<<4)

#define GRN_OBJ_WITH_SECTION           (0x01<<7)
#define GRN_OBJ_WITH_WEIGHT            (0x01<<8)
#define GRN_OBJ_WITH_POSITION          (0x01<<9)
#define GRN_OBJ_WITH_BUFFER            (0x01<<10)

#define GRN_OBJ_UNIT_MASK              (0x0f<<8)
#define GRN_OBJ_UNIT_DOCUMENT_NONE     (0x00<<8)
#define GRN_OBJ_UNIT_DOCUMENT_SECTION  (0x01<<8)
#define GRN_OBJ_UNIT_DOCUMENT_POSITION (0x02<<8)
#define GRN_OBJ_UNIT_SECTION_NONE      (0x03<<8)
#define GRN_OBJ_UNIT_SECTION_POSITION  (0x04<<8)
#define GRN_OBJ_UNIT_POSITION_NONE     (0x05<<8)
#define GRN_OBJ_UNIT_USERDEF_DOCUMENT  (0x06<<8)
#define GRN_OBJ_UNIT_USERDEF_SECTION   (0x07<<8)
#define GRN_OBJ_UNIT_USERDEF_POSITION  (0x08<<8)

#define GRN_OBJ_NO_SUBREC              (0x00<<13)
#define GRN_OBJ_WITH_SUBREC            (0x01<<13)

#define GRN_OBJ_KEY_VAR_SIZE           (0x01<<14)

#define GRN_OBJ_TEMPORARY              (0x00<<15)
#define GRN_OBJ_PERSISTENT             (0x01<<15)

/* obj types */

#define GRN_VOID                       (0x00)
#define GRN_ATOM                       (0x01)
#define GRN_BULK                       (0x02)
#define GRN_PTR                        (0x03)
#define GRN_UVECTOR                    (0x04) /* vector of grn_id */
#define GRN_PVECTOR                    (0x05) /* vector of grn_obj* */
#define GRN_VECTOR                     (0x06) /* vector of arbitrary data */
#define GRN_MSG                        (0x07)
#define GRN_QUERY                      (0x08)
#define GRN_ACCESSOR                   (0x09)
#define GRN_SNIP                       (0x0a)
#define GRN_PATSNIP                    (0x0b)
#define GRN_CURSOR_TABLE_HASH_KEY      (0x10)
#define GRN_CURSOR_TABLE_PAT_KEY       (0x11)
#define GRN_CURSOR_TABLE_NO_KEY        (0x13)
#define GRN_CURSOR_COLUMN_INDEX        (0x18)
#define GRN_TYPE                       (0x20)
#define GRN_PROC                       (0x21)
#define GRN_EXPR                       (0x22)
#define GRN_TABLE_HASH_KEY             (0x30)
#define GRN_TABLE_PAT_KEY              (0x31)
#define GRN_TABLE_NO_KEY               (0x33)
#define GRN_DB                         (0x37)
#define GRN_COLUMN_FIX_SIZE            (0x40)
#define GRN_COLUMN_VAR_SIZE            (0x41)
#define GRN_COLUMN_INDEX               (0x48)

typedef struct _grn_section grn_section;
typedef struct _grn_obj grn_obj;
typedef struct _grn_obj_header grn_obj_header;

struct _grn_section {
  unsigned int offset;
  unsigned int length;
  unsigned int weight;
  grn_id domain;
};

struct _grn_obj_header {
  unsigned char type;
  unsigned char impl_flags;
  grn_obj_flags flags;
  grn_id domain;
};

struct _grn_obj {
  grn_obj_header header;
  union {
    struct {
      char *head;
      char *curr;
      char *tail;
    } b;
    struct {
      grn_obj *body;
      grn_section *sections;
      int n_sections;
    } v;
  } u;
};

#define GRN_OBJ_REFER                  (0x01<<0)
#define GRN_OBJ_OUTPLACE               (0x01<<1)

#define GRN_OBJ_INIT(obj,obj_type,obj_flags,obj_domain) do { \
  (obj)->header.type = (obj_type);\
  (obj)->header.impl_flags = (obj_flags);\
  (obj)->header.flags = 0;\
  (obj)->header.domain = (obj_domain);\
  (obj)->u.b.head = NULL;\
  (obj)->u.b.curr = NULL;\
  (obj)->u.b.tail = NULL;\
} while (0)

#define GRN_OBJ_FIN(ctx,obj) (grn_obj_close((ctx), (obj)))

/**
 * grn_db_create:
 * @path: 作成するdbを格納するファイルパス。NULLならtemporary dbとなる。
 * @optarg: 作成するdbの組み込み型の名前を変更する時に指定する。
 * optarg.builtin_type_namesには組み込み型の名前となるnul終端文字列の配列を指定する。
 * optarg.n_builtin_type_namesには、optarg.builtin_type_namesで指定する文字列の数を
 * 指定する。配列のoffsetはenum型grn_builtin_typeの値に対応する。
 *
 * 新たなdbを作成する。
 **/

typedef struct _grn_db_create_optarg grn_db_create_optarg;

struct _grn_db_create_optarg {
  char **builtin_type_names;
  int n_builtin_type_names;
};

GRN_API grn_obj *grn_db_create(grn_ctx *ctx, const char *path, grn_db_create_optarg *optarg);

#define GRN_DB_OPEN_OR_CREATE(ctx,path,optarg,db) \
  (((db) = grn_db_open((ctx), (path))) || (db = grn_db_create((ctx), (path), (optarg))))

/**
 * grn_db_open:
 * @path: 開こうとするdbを格納するファイルパス。
 *
 * 既存のdbを開く。
 **/
GRN_API grn_obj *grn_db_open(grn_ctx *ctx, const char *path);

/**
 * grn_ctx_use:
 * @db: ctxが使用するdbを指定します。
 *
 * ctxが操作対象とするdbを指定します。NULLを指定した場合は、
 * dbを操作しない状態(init直後の状態)になります。
 **/
GRN_API grn_rc grn_ctx_use(grn_ctx *ctx, grn_obj *db);

/**
 * grn_ctx_db:
 *
 * ctxが現在操作対象としているdbを返します。
 * dbを使用していない場合はNULLを返します。
 **/
GRN_API grn_obj *grn_ctx_db(grn_ctx *ctx);

/**
 * grn_ctx_get:
 * @name: 検索しようとするオブジェクトの名前。
 * @name_size: @nameのbyte長。
 *
 * ctxが使用するdbからnameに対応するオブジェクトを検索して返す。
 * nameに一致するオブジェクトが存在しなければNULLを返す。
 **/
GRN_API grn_obj *grn_ctx_get(grn_ctx *ctx, const char *name, unsigned name_size);

/**
 * grn_ctx_at:
 * @id: 検索しようとするオブジェクトのid。
 *
 * ctx、またはctxが使用するdbからidに対応するオブジェクトを検索して返す。
 * idに一致するオブジェクトが存在しなければNULLを返す。
 **/

typedef enum {
  GRN_DB_VOID = 0,
  GRN_DB_INT32,
  GRN_DB_UINT32,
  GRN_DB_INT64,
  GRN_DB_UINT64,
  GRN_DB_FLOAT,
  GRN_DB_TIME,
  GRN_DB_SHORTTEXT,
  GRN_DB_TEXT,
  GRN_DB_LONGTEXT,
  GRN_DB_DELIMIT,
  GRN_DB_UNIGRAM,
  GRN_DB_BIGRAM,
  GRN_DB_TRIGRAM,
  GRN_DB_MECAB,
} grn_builtin_type;

GRN_API grn_obj *grn_ctx_at(grn_ctx *ctx, grn_id id);

/**
 * grn_type_create:
 * @name: 作成するtypeの名前。
 * @flags: GRN_OBJ_KEY_VAR_SIZE, GRN_OBJ_KEY_FLOAT, GRN_OBJ_KEY_INT, GRN_OBJ_KEY_UINT
 *        のいずれかを指定
 * @size: GRN_OBJ_KEY_VAR_SIZEの場合は最大長、
 *        それ以外の場合は長さを指定(単位:byte)
 *
 * nameに対応する新たなtype(型)をdbに定義する。
 **/

GRN_API grn_obj *grn_type_create(grn_ctx *ctx, const char *name, unsigned name_size,
                                 grn_obj_flags flags, unsigned int size);

GRN_API grn_rc grn_db_load(grn_ctx *ctx, const char *path);

/**
 * grn_proc_create:
 * @name: 作成するprocの名前。
 * @type: procの種類。
 * @init: 初期化関数のポインタ
 * @next: 実処理関数のポインタ
 * @fin: 終了関数のポインタ
 *
 * nameに対応する新たなproc(手続き)をctxが使用するdbに定義する。
 **/

typedef union {
  int int_value;
  grn_id id;
  void *ptr;
} grn_proc_data;

typedef grn_rc grn_proc_func(grn_ctx *ctx, grn_obj *obj, grn_proc_data *user_data);

typedef grn_rc grn_proc_init_func(grn_ctx *ctx, const char *path);

GRN_API grn_obj *grn_proc_create(grn_ctx *ctx,
                                 const char *name, unsigned name_size, const char *path,
                                 grn_proc_func *init, grn_proc_func *next, grn_proc_func *fin,
                                 unsigned nargs, unsigned nresults, grn_obj *result_types);

/*-------------------------------------------------------------
 * table操作のための関数
 */

#define GRN_TABLE_MAX_KEY_SIZE         (0x1000)

/**
 * grn_table_create:
 * @name: 作成するtableの名前。NULLなら無名tableとなる。
 * @path: 作成するtableのファイルパス。
 *        flagsにGRN_OBJ_PERSISTENTが指定されている場合のみ有効。
 *        NULLなら自動的にファイルパスが付与される。
 * @flags: GRN_OBJ_PERSISTENTを指定すると永続tableとなる。
 *         GRN_OBJ_TABLE_PAT_KEY,GRN_OBJ_TABLE_HASH_KEY,GRN_OBJ_TABLE_NO_KEY
 *         のいずれかを指定する。
 *         GRN_OBJ_KEY_NORMALIZEを指定すると正規化された文字列がkeyとなる。
 *         GRN_OBJ_KEY_WITH_SISを指定するとkey文字列の全suffixが自動的に登録される。
 * @key_type: keyの型を指定する。GRN_OBJ_TABLE_NO_KEYが指定された場合は無効。
 *            既存のtypeあるいはtableを指定できる。
 *            key_typeにtable Aを指定してtable Bを作成した場合、Bは必ずAのサブセットとなる。
 * @value_size: keyに対応する値を格納する領域のサイズ(byte長)。tableはcolumnとは別に、
 *              keyに対応する値を格納する領域を一つだけ持つことができる。
 *
 * nameに対応する新たなtableをctxが使用するdbに定義する。
 **/
GRN_API grn_obj *grn_table_create(grn_ctx *ctx,
                                  const char *name, unsigned name_size,
                                  const char *path, grn_obj_flags flags,
                                  grn_obj *key_type, unsigned value_size);
/**
 * grn_table_open:
 * @name: 開こうとするtableの名前。NULLなら無名tableとなる。
 * @path: 開こうとするtableのファイルパス。
 *
 * ctxが使用するdbの中でnameに対応付けて既存のtableを開く。
 **/
GRN_API grn_obj *grn_table_open(grn_ctx *ctx,
                                const char *name, unsigned name_size, const char *path);

#define GRN_TABLE_OPEN_OR_CREATE(ctx,name,name_size,path,flags,key_type,value_size,table) \
  (((table) = grn_ctx_get((ctx), (name), (name_size))) ||\
   ((table) = grn_table_create((ctx), (name), (name_size), (path), (flags), (key_type), (value_size))))

typedef unsigned char grn_search_flags;

#define GRN_SEARCH_EXACT               (0x00)
#define GRN_SEARCH_LCP                 (0x01)
#define GRN_SEARCH_SUFFIX              (0x02)
#define GRN_SEARCH_PREFIX              (0x03)
#define GRN_SEARCH_PARTIAL             (0x04)
#define GRN_SEARCH_NEAR                (0x05)
#define GRN_SEARCH_NEAR2               (0x06)
#define GRN_SEARCH_SIMILAR             (0x07)
#define GRN_SEARCH_TERM_EXTRACT        (0x08)
#define GRN_TABLE_ADD                  (0x01<<6)
#define GRN_TABLE_ADDED                (0x01<<7)

/**
 * grn_table_lookup:
 * @table: 対象table
 * @key: 検索key
 * @flags: GRN_SEARCH_EXACTが指定された場合はkeyに完全一致するrecordを検索する。
 *         GRN_SEARCH_LCPが指定された場合はlongest common prefix searchを行う。
 *         該当するkeyが存在せず、かつGRN_TABLE_ADDが指定された場合は、
 *         tableに該当レコードを追加する。(追加しない場合はGRN_ID_NILを返す)
 *         GRN_TABLE_ADDが指定され、かつ実際にレコードが追加された場合は、
 *         flagsのGRN_TABLE_ADDED bitが立てられる。
 *         flagsにNULLが指定された場合は、GRN_SEARCH_EXACTのみが指定されたものと見なされる。
 *
 * tableからkeyに対応するrecordを検索し、対応するIDを返す。
 *
 * deprecated: 近々廃止されます。
 **/
GRN_API grn_id grn_table_lookup(grn_ctx *ctx, grn_obj *table,
                                const void *key, unsigned key_size,
                                grn_search_flags *flags);

/**
 * grn_table_add:
 * @table: 対象table
 * @key: 検索key
 * @added: NULL以外の値が指定された場合、
 * 新たにrecordが追加された時には1が、既存recordだった時には0がセットされる。
 *
 * keyに対応する新しいrecordをtableに追加し、そのIDを返す。
 * keyに対応するrecordがすでにtableに存在するならば、そのrecordのIDを返す。
 * GRN_OBJ_TABLE_NO_KEYが指定されたtableでは、key, key_size は無視される。
 **/
GRN_API grn_id grn_table_add(grn_ctx *ctx, grn_obj *table,
                             const void *key, unsigned key_size, int *added);

/**
 * grn_table_get:
 * @table: 対象table
 * @key: 検索key
 *
 * tableからkeyに対応するrecordを検索し、対応するIDを返す。
 **/
GRN_API grn_id grn_table_get(grn_ctx *ctx, grn_obj *table,
                             const void *key, unsigned key_size);

/**
 * grn_table_lcp_search:
 * @table: 対象table
 * @key: 検索key
 *
 * tableがGRN_TABLE_PAT_KEYを指定して作ったtableなら、
 * longest common prefix searchを行い、対応するIDを返す。
 **/
GRN_API grn_id grn_table_lcp_search(grn_ctx *ctx, grn_obj *table,
                                    const void *key, unsigned key_size);

/**
 * grn_table_get_key:
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
GRN_API int grn_table_get_key(grn_ctx *ctx, grn_obj *table,
                              grn_id id, void *keybuf, int buf_size);

/**
 * grn_table_delete:
 * @table: 対象table
 * @key: 検索key
 * @key_size: 検索keyのサイズ
 *
 * tableのkeyに対応するレコードを削除する。
 * 対応するレコードが存在しない場合はGRN_INVALID_ARGUMENTを返す。
 **/
GRN_API grn_rc grn_table_delete(grn_ctx *ctx, grn_obj *table,
                                const void *key, unsigned key_size);

/**
 * grn_table_delete_by_id:
 * @table: 対象table
 * @id: レコードID
 *
 * tableのkeyに対応するレコードを削除する。
 * 対応するレコードが存在しない場合はGRN_INVALID_ARGUMENTを返す。
 **/
GRN_API grn_rc grn_table_delete_by_id(grn_ctx *ctx, grn_obj *table, grn_id id);

/**
 * grn_table_truncate:
 * @table: 対象table
 *
 * tableの全レコードを一括して削除する。
 **/
GRN_API grn_rc grn_table_truncate(grn_ctx *ctx, grn_obj *table);

typedef grn_obj grn_table_cursor;

#define GRN_CURSOR_ASCENDING           (0x00<<0)
#define GRN_CURSOR_DESCENDING          (0x01<<0)
#define GRN_CURSOR_GE                  (0x00<<1)
#define GRN_CURSOR_GT                  (0x01<<1)
#define GRN_CURSOR_LE                  (0x00<<2)
#define GRN_CURSOR_LT                  (0x01<<2)

/**
 * grn_table_cursor_open:
 * @table: 対象table
 * @min: keyの下限 (NULLは下限なしと見なす)
 * @min_size: @minのsize
 * @max: keyの上限 (NULLは上限なしと見なす)
 * @max_size: @maxのsize
 * @flags: GRN_CURSOR_ASCENDINGを指定すると昇順にkeyを取り出す。(指定しなければ降順)
 *         GRN_CURSOR_GTを指定するとminに一致したkeyをcursorの範囲に含まない。
 *         GRN_CURSOR_LTを指定するとmaxに一致したkeyをcursorの範囲に含まない。
 *
 * tableに登録されているレコードを順番に取り出すためのカーソルを生成して返す。
 * GRN_OBJ_TABLE_PAT_KEYを指定したtableではkey順に、
 * GRN_OBJ_TABLE_HASH_KEYを指定したtableではid順にレコードを取り出します。
 **/
GRN_API grn_table_cursor *grn_table_cursor_open(grn_ctx *ctx, grn_obj *table,
                                                const void *min, unsigned min_size,
                                                const void *max, unsigned max_size,
                                                int flags);

/**
 * grn_table_cursor_close:
 * @tc: 対象cursor
 *
 * grn_table_cursor_openで生成したcursorを解放する。
 **/
GRN_API grn_rc grn_table_cursor_close(grn_ctx *ctx, grn_table_cursor *tc);

/**
 * grn_table_cursor_next:
 * @tc: 対象cursor
 *
 * cursorのカレントレコードを一件進めてそのIDを返す。
 * cursorの対象範囲の末尾に達するとGRN_ID_NILを返す。
 **/
GRN_API grn_id grn_table_cursor_next(grn_ctx *ctx, grn_table_cursor *tc);

/**
 * grn_table_cursor_get_key:
 * @tc: 対象cursor
 * @key: カレントレコードのkeyへのポインタがセットされる。
 * cursorのカレントレコードのkeyを@keyにセットし、その長さを返す。
 **/
GRN_API int grn_table_cursor_get_key(grn_ctx *ctx, grn_table_cursor *tc, void **key);

/**
 * grn_table_cursor_get_value:
 * @tc: 対象cursor
 * @value: カレントレコードのvalueへのポインタがセットされる。
 * cursorのカレントレコードのvalueを@valueにセットし、その長さを返す。
 **/
GRN_API int grn_table_cursor_get_value(grn_ctx *ctx, grn_table_cursor *tc, void **value);

/**
 * grn_table_cursor_set_value:
 * @tc: 対象cursor
 * @value: 新しいvalueの値。
 * @flags: grn_obj_set_valueのflagsと同様の値を指定できる。
 *
 * cursorのカレントレコードのvalueを引数の内容に置き換える。
 * cursorのカレントレコードが存在しない場合はGRN_INVALID_ARGUMENTを返す。
 **/
GRN_API grn_rc grn_table_cursor_set_value(grn_ctx *ctx, grn_table_cursor *tc,
                                  void *value, int flags);

/**
 * grn_table_cursor_delete:
 * @tc: 対象cursor
 *
 * cursorのカレントレコードを削除する。
 * cursorのカレントレコードが存在しない場合はGRN_INVALID_ARGUMENTを返す。
 **/
GRN_API grn_rc grn_table_cursor_delete(grn_ctx *ctx, grn_table_cursor *tc);

/**
 * grn_table_cursor_table:
 * @tc: 対象cursor
 *
 * cursorが属するtableを返す。
 **/
GRN_API grn_obj *grn_table_cursor_table(grn_ctx *ctx, grn_table_cursor *tc);

#define GRN_TABLE_EACH(ctx,table,head,tail,id,key,key_size,value,block) do {\
  (ctx)->errlvl = GRN_OK;\
  (ctx)->rc = GRN_SUCCESS;\
  if ((ctx)->seqno & 1) {\
    (ctx)->subno++;\
  } else {\
    (ctx)->seqno++;\
  }\
  if (table) {\
    switch ((table)->header.type) {\
    case GRN_TABLE_PAT_KEY :\
      GRN_PAT_EACH((grn_pat *)(table), (id), (key), (key_size), (value), block);\
      break;\
    case GRN_TABLE_HASH_KEY :\
      GRN_HASH_EACH((grn_hash *)(table), (id), (key), (key_size), (value), block);\
      break;\
    case GRN_TABLE_NO_KEY :\
      GRN_ARRAY_EACH((grn_array *)(table), (head), (tail), (id), (value), block);\
      break;\
    }\
  }\
  if (ctx->subno) {\
    ctx->subno--;\
  } else {\
    ctx->seqno++;\
  }\
} while (0)

/**
 * grn_table_sort:
 * @table: 対象table
 * @limit: resに格納するレコードの上限
 * @result: 結果を格納するtable
 * @keys: ソートキー配列へのポインタ
 * @n_keys: ソートキー配列のサイズ
 *
 * table内のレコードをソートし、上位limit個の要素をresultに格納する。
 * keysには、tableのcolumn,accessor,procのいずれかが指定できる。
 **/

typedef struct _grn_table_sort_key grn_table_sort_key;
typedef unsigned char grn_table_sort_flags;

#define GRN_TABLE_SORT_ASC             (0x00<<0)
#define GRN_TABLE_SORT_DESC            (0x01<<0)

struct _grn_table_sort_key {
  grn_obj *key;
  grn_table_sort_flags flags;
  int offset;
};

GRN_API int grn_table_sort(grn_ctx *ctx, grn_obj *table, int limit,
                           grn_obj *result, grn_table_sort_key *keys, int n_keys);

/**
 * grn_table_group:
 * @table: 対象table
 * @keys: group化キー構造体の配列へのポインタ
 * @n_keys: group化キー構造体の配列のサイズ
 * @results: group化の結果を格納する構造体の配列へのポインタ
 * @n_results:group化の結果を格納する構造体の配列のサイズ
 *
 * tableのレコードを特定の条件でグループ化する
 **/

typedef struct _grn_table_group_result grn_table_group_result;
typedef unsigned int grn_table_group_flags;

#define GRN_TABLE_GROUP_CALC_COUNT     (0x01<<3)
#define GRN_TABLE_GROUP_CALC_MAX       (0x01<<4)
#define GRN_TABLE_GROUP_CALC_MIN       (0x01<<5)
#define GRN_TABLE_GROUP_CALC_SUM       (0x01<<6)
#define GRN_TABLE_GROUP_CALC_AVG       (0x01<<7)

typedef enum {
  GRN_SEL_OR = 0,
  GRN_SEL_AND,
  GRN_SEL_BUT,
  GRN_SEL_ADJUST
} grn_sel_operator;

struct _grn_table_group_result {
  grn_obj *table;
  unsigned char key_begin;
  unsigned char key_end;
  int limit;
  grn_table_group_flags flags;
  grn_sel_operator op;
};

GRN_API grn_rc grn_table_group(grn_ctx *ctx, grn_obj *table,
                               grn_table_sort_key *keys, int n_keys,
                               grn_table_group_result *results, int n_results);

/**
 * grn_table_setoperation:
 * @table1: 対象table1
 * @table2: 対象table2
 * @res: 結果を格納するtable
 * @op: 実行する演算の種類
 *
 * table1とtable2をopの指定に従って集合演算した結果をresに格納する。
 * resにtable1あるいはtable2そのものを指定した場合を除けば、table1, table2は破壊されない。
 **/
GRN_API grn_rc grn_table_setoperation(grn_ctx *ctx, grn_obj *table1, grn_obj *table2,
                                      grn_obj *res, grn_sel_operator op);

/**
 * grn_table_difference:
 * @table1: 対象table1
 * @table2: 対象table2
 * @res1: 結果を格納するtable
 * @res2: 結果を格納するtable
 *
 * table1とtable2から重複するレコードを取り除いた結果をそれぞれres1, res2に格納する。
 **/
GRN_API grn_rc grn_table_difference(grn_ctx *ctx, grn_obj *table1, grn_obj *table2,
                                    grn_obj *res1, grn_obj *res2);

/**
 * grn_table_columns:
 * @table: 対象table
 * @name: 取得したいカラム名のprefix
 * @name_size: @nameの長さ
 * @res: 結果を格納するGRN_TABLE_HASH_KEYのtable
 *
 * @nameから始まるtableのカラムIDを@resに格納する。
 * @name_sizeが0の場合はすべてのカラムIDを格納する。格納した
 * カラムIDの数を返す。
 **/
GRN_API int grn_table_columns(grn_ctx *ctx, grn_obj *table,
                              const char *name, unsigned name_size,
                              grn_obj *res);

/**
 * grn_obj_column:
 * @table: 対象table
 * @name: カラム名
 *
 * nameに対応するtableのカラムを返す。対応するカラムが存在しなければNULLを返す。
 **/
GRN_API grn_obj *grn_obj_column(grn_ctx *ctx, grn_obj *table,
                                const char *name, unsigned name_size);


/**
 * grn_table_size:
 * @table: 対象table
 *
 * tableに登録されているレコードの件数を返す。
 **/
GRN_API unsigned int grn_table_size(grn_ctx *ctx, grn_obj *table);

/*-------------------------------------------------------------
 * column操作のための関数
 */

/**
 * grn_column_create:
 * @table: 対象table
 * @name: カラム名
 * @name_size: @nameのsize(byte)
 * @path: カラムを格納するファイルパス。
 *        flagsにGRN_OBJ_PERSISTENTが指定されている場合のみ有効。
 *        NULLなら自動的にファイルパスが付与される。
 * @flags: GRN_OBJ_PERSISTENTを指定すると永続columnとなる。
 *         GRN_OBJ_COLUMN_INDEXを指定すると転置インデックスとなる。
 *         GRN_OBJ_COLUMN_SCALARを指定するとスカラ値(単独の値)を格納する。
 *         GRN_OBJ_COLUMN_VECTORを指定すると値の配列を格納する。
 *         GRN_OBJ_COMPRESS_ZLIBを指定すると値をzlib圧縮して格納する。
 *         GRN_OBJ_COMPRESS_LZOを指定すると値をlzo圧縮して格納する。
 *         GRN_OBJ_COLUMN_INDEXと共にGRN_OBJ_WITH_SECTIONを指定すると、
 *         転置索引にsection(段落情報)を合わせて格納する。
 *         GRN_OBJ_COLUMN_INDEXと共にGRN_OBJ_WITH_WEIGHTを指定すると、
 *         転置索引にweight情報を合わせて格納する。
 *         GRN_OBJ_COLUMN_INDEXと共にGRN_OBJ_WITH_POSITIONを指定すると、
 *         転置索引に出現位置情報を合わせて格納する。
 * @type: カラム値の型。定義済みのtypeあるいはtableを指定できる。
 *
 * tableに新たなカラムを定義する。nameは省略できない。
 * 一つのtableに同一のnameのcolumnを複数定義することはできない。
 **/
GRN_API grn_obj *grn_column_create(grn_ctx *ctx, grn_obj *table,
                                   const char *name, unsigned name_size,
                                   const char *path, grn_obj_flags flags, grn_obj *type);

#define GRN_COLUMN_OPEN_OR_CREATE(ctx,table,name,name_size,path,flags,type,column) \
  (((column) = grn_obj_column((ctx), (table), (name), (name_size))) ||\
   ((column) = grn_column_create((ctx), (table), (name), (name_size), (path), (flags), (type))))

/**
 * grn_column_open:
 * @table: 対象table
 * @name: カラム名
 * @path: カラムを格納するファイルパス。
 * @type: カラム値の型。
 *
 * 既存の永続的なcolumnを、tableのnameに対応するcolumnとして開く
 **/
GRN_API grn_obj *grn_column_open(grn_ctx *ctx, grn_obj *table,
                                 const char *name, unsigned name_size,
                                 const char *path, grn_obj *type);

/**
 * grn_column_index_update
 * @column: 対象column
 * @id: 対象レコードのID
 * @section: 対象レコードのセクション番号
 * @oldvalue: 更新前の値
 * @newvalue: 更新後の値
 *
 * oldvalue, newvalueの値から得られるキーに対応するcolumnの値の中の、
 * id, sectionに対応するエントリを更新する。
 * columnはGRN_OBJ_COLUMN_INDEX型のカラムでなければならない。
 **/
GRN_API grn_rc grn_column_index_update(grn_ctx *ctx, grn_obj *column,
                                       grn_id id, unsigned int section,
                                       grn_obj *oldvalue, grn_obj *newvalue);

/**
 * grn_column_table:
 * @column: 対象column
 *
 * columnが属するtableを返す。
 **/
GRN_API grn_obj *grn_column_table(grn_ctx *ctx, grn_obj *column);

/*-------------------------------------------------------------
 * db, table, columnの全てまたは幾つかで共通に使用できる関数
 */

#define GRN_PROC_MAX_ARGS              (0x100)

typedef enum {
  GRN_INFO_ENCODING = 0,
  GRN_INFO_SOURCE,
  GRN_INFO_DEFAULT_TOKENIZER,
  GRN_INFO_ELEMENT_SIZE,
  GRN_INFO_CURR_MAX,
  GRN_INFO_MAX_ELEMENT_SIZE,
  GRN_INFO_SEG_SIZE,
  GRN_INFO_CHUNK_SIZE,
  GRN_INFO_MAX_SECTION,
  GRN_INFO_HOOK_LOCAL_DATA,
  GRN_INFO_ELEMENT_A,
  GRN_INFO_ELEMENT_CHUNK,
  GRN_INFO_ELEMENT_CHUNK_SIZE,
  GRN_INFO_ELEMENT_BUFFER_FREE,
  GRN_INFO_ELEMENT_NTERMS,
  GRN_INFO_ELEMENT_NTERMS_VOID,
  GRN_INFO_ELEMENT_SIZE_IN_CHUNK,
  GRN_INFO_ELEMENT_POS_IN_CHUNK,
  GRN_INFO_ELEMENT_SIZE_IN_BUFFER,
  GRN_INFO_ELEMENT_POS_IN_BUFFER,
  GRN_INFO_ELEMENT_ESTIMATE_SIZE,
  GRN_INFO_NGRAM_UNIT_SIZE,
  GRN_INFO_VERSION,
  GRN_INFO_CONFIGURE_OPTIONS,
  GRN_INFO_CONFIG_PATH,
  GRN_INFO_PARTIAL_MATCH_THRESHOLD
} grn_info_type;

/**
 * grn_obj_get_info:
 * @obj: 対象obj
 * @type: 取得する情報の種類
 * @valuebuf: 値を格納するバッファ(呼出側で準備)
 *
 * objのtypeに対応する情報をvaluebufに格納する。
 **/
GRN_API grn_obj *grn_obj_get_info(grn_ctx *ctx, grn_obj *obj, grn_info_type type, grn_obj *valuebuf);

/**
 * grn_obj_set_info:
 * @obj: 対象obj
 * @type: 設定する情報の種類
 * @value: 設定しようとする値
 *
 * objのtypeに対応する情報をvalueの内容に更新する。
 **/
GRN_API grn_rc grn_obj_set_info(grn_ctx *ctx, grn_obj *obj, grn_info_type type, grn_obj *value);

/**
 * grn_obj_get_element_info:
 * @obj: 対象obj
 * @id: 対象ID
 * @type: 取得する情報の種類
 * @value: 値を格納するバッファ(呼出側で準備)
 *
 * objのidに対応するレコードの、typeに対応する情報をvaluebufに格納する。
 * 呼出側ではtypeに応じて十分なサイズのバッファを確保しなければならない。
 **/
GRN_API grn_obj *grn_obj_get_element_info(grn_ctx *ctx, grn_obj *obj, grn_id id,
                                          grn_info_type type, grn_obj *value);

/**
 * grn_obj_set_element_info:
 * @obj: 対象object
 * @id: 対象ID
 * @type: 設定する情報の種類
 * @value: 設定しようとする値
 *
 * objのidに対応するレコードのtypeに対応する情報をvalueの内容に更新する。
 **/
GRN_API grn_rc grn_obj_set_element_info(grn_ctx *ctx, grn_obj *obj, grn_id id,
                                        grn_info_type type, grn_obj *value);


/**
 * grn_obj_get_value:
 * @obj: 対象object
 * @id: 対象レコードのID
 * @value: 値を格納するバッファ(呼出側で準備する)
 *
 * tableのIDに対応するレコードのvalueを取得する。
 * valueを戻り値として返す。
 **/
GRN_API grn_obj *grn_obj_get_value(grn_ctx *ctx, grn_obj *obj, grn_id id, grn_obj *value);

/**
 * grn_obj_set_value:
 * @obj: 対象object
 * @id: 対象レコードのID
 * @value: 格納する値
 * @flags: 以下の値を指定できる
 *  GRN_OBJ_SET: レコードの値をvalueと置き換える。
 *  GRN_OBJ_INCR: レコードの値にvalueを加算する。
 *  GRN_OBJ_DECR: レコードの値にvalueを減算する。
 *  GRN_OBJ_APPEND: レコードの値の末尾にvalueを追加する。
 *  GRN_OBJ_PREPEND: レコードの値の先頭にvalueを追加する。
 *  GRN_OBJ_GET: 新しいレコードの値をvalueにセットする。
 *  GRN_OBJ_COMPARE: レコードの値とvalueが等しいか調べる。
 *  GRN_OBJ_LOCK: 当該レコードをロックする。GRN_OBJ_COMPAREと共に指定された場合は、
 *                レコードの値とvalueが等しい場合に限ってロックする。
 *  GRN_OBJ_UNLOCK: 当該レコードのロックを解除する。
 *
 * objのIDに対応するレコードの値を更新する。
 * 対応するレコードが存在しない場合はGRN_INVALID_ARGUMENTを返す。
 **/

#define GRN_OBJ_SET_MASK               (0x07)
#define GRN_OBJ_SET                    (0x01)
#define GRN_OBJ_INCR                   (0x02)
#define GRN_OBJ_DECR                   (0x03)
#define GRN_OBJ_APPEND                 (0x04)
#define GRN_OBJ_PREPEND                (0x05)
#define GRN_OBJ_GET                    (0x01<<4)
#define GRN_OBJ_COMPARE                (0x01<<5)
#define GRN_OBJ_LOCK                   (0x01<<6)
#define GRN_OBJ_UNLOCK                 (0x01<<7)

GRN_API grn_rc grn_obj_set_value(grn_ctx *ctx, grn_obj *obj, grn_id id, grn_obj *value, int flags);

/**
 * grn_obj_remove:
 * @obj: 対象object
 *
 * objをメモリから解放し、それが永続オブジェクトであった場合は、
 * 該当するファイル一式を削除する。
 **/
GRN_API grn_rc grn_obj_remove(grn_ctx *ctx, grn_obj *obj);

/**
 * grn_obj_rename:
 * @old_path: 旧ファイルパス
 * @new_path: 新ファイルパス
 *
 * old_pathに該当するオブジェクトのファイル名をnew_pathに変更する。
 **/
GRN_API grn_rc grn_obj_rename(grn_ctx *ctx, const char *old_path, const char *new_path);

/**
 * grn_obj_close:
 * @obj: 対象object
 *
 * objをメモリから解放する。objに属するobjectも再帰的にメモリから解放される。
 **/
GRN_API grn_rc grn_obj_close(grn_ctx *ctx, grn_obj *obj);

/**
 * grn_obj_path:
 * @obj: 対象object
 *
 * objに対応するファイルパスを返す。一時objectならNULLを返す。
 **/
GRN_API const char *grn_obj_path(grn_ctx *ctx, grn_obj *obj);

/**
 * grn_obj_name:
 * @obj: 対象object
 * @namebuf: 名前を格納するバッファ(呼出側で準備する)
 * @buf_size: namebufのサイズ(byte長)
 *
 * objの名前の長さを返す。無名objectなら0を返す。
 * 名前付きのobjectであり、buf_sizeの長さが名前の長以上であった場合は、
 * namebufに該当する名前をコピーする。
 **/
GRN_API int grn_obj_name(grn_ctx *ctx, grn_obj *obj, char *namebuf, int buf_size);

/**
 * grn_obj_name:
 * @obj: 対象object
 * @namebuf: 名前を格納するバッファ(呼出側で準備する)
 * @buf_size: namebufのサイズ(byte長)
 *
 * カラムobjの名前の長さを返す。
 * buf_sizeの長さが名前の長以上であった場合は、
 * namebufに該当する名前をコピーする。
 **/
GRN_API int grn_column_name(grn_ctx *ctx, grn_obj *obj, char *namebuf, int buf_size);

/**
 * grn_obj_get_range:
 * @obj: 対象object
 *
 * @objのとる値の範囲を表わしているオブジェクトのIDを返す。
 * 例えば、grn_builtin_typeにあるGRN_DB_INTなどを返す。
 **/
GRN_API grn_id grn_obj_get_range(grn_ctx *ctx, grn_obj *obj);

#define GRN_OBJ_GET_DOMAIN(obj) ((obj)->header.domain)

/**
 * grn_obj_expire:
 * @obj: 対象object
 *
 * objの占有するメモリのうち、可能な領域をthresholdを指標として解放する。
 **/
GRN_API int grn_obj_expire(grn_ctx *ctx, grn_obj *obj, int threshold);

/**
 * grn_obj_check:
 * @obj: 対象object
 *
 * objに対応するファイルの整合性を検査する。
 **/
GRN_API int grn_obj_check(grn_ctx *ctx, grn_obj *obj);

/**
 * grn_obj_lock:
 * @obj: 対象object
 *
 * objをlockする。timeout(秒)経過してもlockを取得できない場合は
 * GRN_RESOURCE_DEADLOCK_AVOIDEDを返す。
 **/
GRN_API grn_rc grn_obj_lock(grn_ctx *ctx, grn_obj *obj, grn_id id, int timeout);

/**
 * grn_obj_unlock:
 * @obj: 対象object
 *
 * objをunlockする。
 **/
GRN_API grn_rc grn_obj_unlock(grn_ctx *ctx, grn_obj *obj, grn_id id);

/**
 * grn_obj_clear_lock:
 * @obj: 対象object
 *
 * 強制的にロックをクリアする。
 **/
GRN_API grn_rc grn_obj_clear_lock(grn_ctx *ctx, grn_obj *obj);

/**
 * grn_obj_is_locked;
 * @obj: 対象object
 *
 * objが現在lockされていれば0以外の値を返す。
 **/
GRN_API unsigned int grn_obj_is_locked(grn_ctx *ctx, grn_obj *obj);

/**
 * grn_obj_db:
 * @obj: 対象object
 *
 * objの属するdbを返す。
 **/
GRN_API grn_obj *grn_obj_db(grn_ctx *ctx, grn_obj *obj);

/**
 * grn_obj_id:
 * @obj: 対象object
 *
 * objのidを返す。
 **/
GRN_API grn_id grn_obj_id(grn_ctx *ctx, grn_obj *obj);


/**
 * grn_obj_search:
 * @obj: 検索対象のobject
 * @query: 検索クエリ
 * @res: 検索結果を格納するテーブル
 * @op: GRN_SEL_OR, GRN_SEL_AND, GRN_SEL_BUT, GRN_SEL_ADJUSTのいずれかを指定する
 * @optarg: 詳細検索条件
 *
 * objを対象としてqueryにマッチするレコードを検索し、
 * opの指定に従ってresにレコードを追加あるいは削除する。
 **/
typedef struct _grn_search_optarg grn_search_optarg;

struct _grn_search_optarg {
  grn_search_flags flags;
  int similarity_threshold;
  int max_interval;
  int *weight_vector;
  int vector_size;
  grn_obj *proc;
  int max_size;
};

GRN_API grn_rc grn_obj_search(grn_ctx *ctx, grn_obj *obj, grn_obj *query,
                              grn_obj *res, grn_sel_operator op, grn_search_optarg *optarg);

/*-------------------------------------------------------------
 * grn_vector
*/

GRN_API unsigned int grn_vector_size(grn_ctx *ctx, grn_obj *vector);

GRN_API grn_rc grn_vector_add_element(grn_ctx *ctx, grn_obj *vector,
                                      const char *str, unsigned int str_len,
                                      unsigned int weight, grn_id domain);

GRN_API unsigned int grn_vector_get_element(grn_ctx *ctx, grn_obj *vector,
                                            unsigned int offset, const char **str,
                                            unsigned int *weight, grn_id *domain);

/*-------------------------------------------------------------
 * hook操作のための関数
 */

GRN_API int grn_proc_call_next(grn_ctx *ctx, grn_obj *exec_info, grn_obj *in, grn_obj *out);
GRN_API void *grn_proc_get_ctx_local_data(grn_ctx *ctx, grn_obj *exec_info);
GRN_API void *grn_proc_get_hook_local_data(grn_ctx *ctx, grn_obj *exec_info);

typedef enum {
  GRN_HOOK_SET = 0,
  GRN_HOOK_GET,
  GRN_HOOK_INSERT,
  GRN_HOOK_DELETE,
  GRN_HOOK_SELECT
} grn_hook_entry;

/**
 * grn_obj_add_hook:
 * @obj: 対象object
 * @entry: GRN_HOOK_GETは、objectの参照時に呼び出されるhookを定義する。
          GRN_HOOK_SETは、objectの更新時に呼び出されるhookを定義する。
          GRN_HOOK_SELECTは、検索処理の実行中に適時呼び出され、
          処理の実行状況を調べたり、実行の中断を指示することができる。
 * @offset: hookの実行順位。offsetに対応するhookの直前に新たなhookを挿入する。
            0を指定した場合は先頭に挿入される。-1を指定した場合は末尾に挿入される。
            objectに複数のhookが定義されている場合は順位の順に呼び出される。
 * @proc: 手続き
 * @data: hook固有情報
 *
 * objに対してhookを追加する。
 **/
GRN_API grn_rc grn_obj_add_hook(grn_ctx *ctx, grn_obj *obj, grn_hook_entry entry,
                                int offset, grn_obj *proc, grn_obj *data);

/**
 * grn_obj_get_nhooks:
 * @obj: 対象object
 * @entry: hookタイプ
 *
 * objに定義されているhookの数を返す。
 **/
GRN_API int grn_obj_get_nhooks(grn_ctx *ctx, grn_obj *obj, grn_hook_entry entry);

/**
 * grn_obj_get_hook:
 * @obj: 対象object
 * @entry: hookタイプ
 * @offset: 実行順位
 * @data: hook固有情報格納バッファ
 *
 * objに定義されているhookの手続き(proc)を返す。hook固有情報が定義されている場合は、
 * その内容をdataにコピーして返す。
 **/
GRN_API grn_obj *grn_obj_get_hook(grn_ctx *ctx, grn_obj *obj, grn_hook_entry entry,
                                  int offset, grn_obj *data);

/**
 * grn_obj_delete_hook:
 * @obj: 対象object
 * @entry: hookタイプ
 * @offset: 実行順位
 *
 * objに定義されているhookを削除する。
 **/
GRN_API grn_rc grn_obj_delete_hook(grn_ctx *ctx, grn_obj *obj, grn_hook_entry entry, int offset);

GRN_API grn_obj *grn_obj_open(grn_ctx *ctx, unsigned char type, grn_obj_flags flags, grn_id domain);

/* query & snippet */

#ifndef GRN_QUERY_AND
#define GRN_QUERY_AND '+'
#endif /* GRN_QUERY_AND */
#ifndef GRN_QUERY_BUT
#define GRN_QUERY_BUT '-'
#endif /* GRN_QUERY_BUT */
#ifndef GRN_QUERY_ADJ_INC
#define GRN_QUERY_ADJ_INC '>'
#endif /* GRN_QUERY_ADJ_POS2 */
#ifndef GRN_QUERY_ADJ_DEC
#define GRN_QUERY_ADJ_DEC '<'
#endif /* GRN_QUERY_ADJ_POS1 */
#ifndef GRN_QUERY_ADJ_NEG
#define GRN_QUERY_ADJ_NEG '~'
#endif /* GRN_QUERY_ADJ_NEG */
#ifndef GRN_QUERY_PREFIX
#define GRN_QUERY_PREFIX '*'
#endif /* GRN_QUERY_PREFIX */
#ifndef GRN_QUERY_PARENL
#define GRN_QUERY_PARENL '('
#endif /* GRN_QUERY_PARENL */
#ifndef GRN_QUERY_PARENR
#define GRN_QUERY_PARENR ')'
#endif /* GRN_QUERY_PARENR */
#ifndef GRN_QUERY_QUOTEL
#define GRN_QUERY_QUOTEL '"'
#endif /* GRN_QUERY_QUOTEL */
#ifndef GRN_QUERY_QUOTER
#define GRN_QUERY_QUOTER '"'
#endif /* GRN_QUERY_QUOTER */
#ifndef GRN_QUERY_ESCAPE
#define GRN_QUERY_ESCAPE '\\'
#endif /* GRN_QUERY_ESCAPE */

typedef struct _grn_snip grn_snip;
typedef struct _grn_query grn_query;
typedef struct _grn_snip_mapping grn_snip_mapping;

struct _grn_snip_mapping {
  void *dummy;
};

GRN_API grn_query *grn_query_open(grn_ctx *ctx, const char *str, unsigned int str_len,
                                  grn_sel_operator default_op, int max_exprs);
GRN_API unsigned int grn_query_rest(grn_ctx *ctx, grn_query *q, const char ** const rest);
GRN_API grn_rc grn_query_close(grn_ctx *ctx, grn_query *q);

GRN_API grn_rc grn_query_scan(grn_ctx *ctx, grn_query *q, const char **strs, unsigned int *str_lens,
                              unsigned int nstrs, int flags, int *found, int *score);
GRN_API grn_snip *grn_query_snip(grn_ctx *ctx, grn_query *query, int flags,
                                 unsigned int width, unsigned int max_results,
                                 unsigned int n_tags,
                                 const char **opentags, unsigned int *opentag_lens,
                                 const char **closetags, unsigned int *closetag_lens,
                                 grn_snip_mapping *mapping);

#define GRN_SNIP_NORMALIZE             (0x01<<0)
#define GRN_SNIP_COPY_TAG              (0x01<<1)
#define GRN_SNIP_SKIP_LEADING_SPACES   (0x01<<2)
#define GRN_QUERY_SCAN_NORMALIZE       GRN_SNIP_NORMALIZE

GRN_API grn_snip *grn_snip_open(grn_ctx *ctx, int flags, unsigned int width,
                                unsigned int max_results,
                                const char *defaultopentag, unsigned int defaultopentag_len,
                                const char *defaultclosetag, unsigned int defaultclosetag_len,
                                grn_snip_mapping *mapping);
GRN_API grn_rc grn_snip_close(grn_ctx *ctx, grn_snip *snip);
GRN_API grn_rc grn_snip_add_cond(grn_ctx *ctx, grn_snip *snip,
                                 const char *keyword, unsigned int keyword_len,
                                 const char *opentag, unsigned int opentag_len,
                                 const char *closetag, unsigned int closetag_len);
GRN_API grn_rc grn_snip_exec(grn_ctx *ctx, grn_snip *snip,
                             const char *string, unsigned int string_len,
                             unsigned int *nresults, unsigned int *max_tagged_len);
GRN_API grn_rc grn_snip_get_result(grn_ctx *ctx, grn_snip *snip, const unsigned int index,
                                   char *result, unsigned int *result_len);

/* log */

#define GRN_LOG_TIME                   (0x01<<0)
#define GRN_LOG_TITLE                  (0x01<<1)
#define GRN_LOG_MESSAGE                (0x01<<2)
#define GRN_LOG_LOCATION               (0x01<<3)

typedef struct _grn_logger_info grn_logger_info;

struct _grn_logger_info {
  grn_log_level max_level;
  int flags;
  void (*func)(int, const char *, const char *, const char *, const char *, void *);
  void *func_arg;
};

GRN_API grn_rc grn_logger_info_set(grn_ctx *ctx, const grn_logger_info *info);

GRN_API void grn_logger_put(grn_ctx *ctx, grn_log_level level,
                            const char *file, int line, const char *func, const char *fmt, ...);

GRN_API int grn_logger_pass(grn_ctx *ctx, grn_log_level level);

#ifndef GRN_LOG_DEFAULT_LEVEL
#define GRN_LOG_DEFAULT_LEVEL GRN_LOG_NOTICE
#endif /* GRN_LOG_DEFAULT_LEVEL */

#define GRN_LOG(ctx,level,...) do {\
  if (grn_logger_pass(ctx, level)) {\
    grn_logger_put(ctx, (level), __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__); \
  }\
} while (0)

/* grn_bulk */

#define GRN_BULK_BUFSIZE (sizeof(grn_obj) - sizeof(grn_obj_header))
#define GRN_BULK_OUTP(bulk) ((bulk)->header.impl_flags & GRN_OBJ_OUTPLACE)
#define GRN_BULK_REWIND(bulk) do {\
  if (GRN_BULK_OUTP(bulk)) {\
    (bulk)->u.b.curr = (bulk)->u.b.head;\
  } else {\
    (bulk)->header.flags = 0;\
  }\
} while (0)
#define GRN_BULK_WSIZE(bulk) \
  (GRN_BULK_OUTP(bulk)\
   ? ((bulk)->u.b.tail - (bulk)->u.b.head)\
   : GRN_BULK_BUFSIZE)
#define GRN_BULK_REST(bulk) \
  (GRN_BULK_OUTP(bulk)\
   ? ((bulk)->u.b.tail - (bulk)->u.b.curr)\
   : GRN_BULK_BUFSIZE - (bulk)->header.flags)
#define GRN_BULK_VSIZE(bulk) \
  (GRN_BULK_OUTP(bulk)\
   ? ((bulk)->u.b.curr - (bulk)->u.b.head)\
   : (bulk)->header.flags)
#define GRN_BULK_EMPTYP(bulk) \
  (GRN_BULK_OUTP(bulk)\
   ? ((bulk)->u.b.curr == (bulk)->u.b.head)\
   : !((bulk)->header.flags))
#define GRN_BULK_HEAD(bulk) \
  (GRN_BULK_OUTP(bulk)\
   ? ((bulk)->u.b.head)\
   : (char *)&((bulk)->u.b.head))
#define GRN_BULK_CURR(bulk) \
  (GRN_BULK_OUTP(bulk)\
   ? ((bulk)->u.b.curr)\
   : (char *)&((bulk)->u.b.head) + (bulk)->header.flags)
#define GRN_BULK_TAIL(bulk) \
  (GRN_BULK_OUTP(bulk)\
   ? ((bulk)->u.b.tail)\
   : (char *)&((bulk)[1]))

GRN_API grn_rc grn_bulk_reinit(grn_ctx *ctx, grn_obj *bulk, unsigned int size);
GRN_API grn_rc grn_bulk_resize(grn_ctx *ctx, grn_obj *bulk, unsigned int newsize);
GRN_API grn_rc grn_bulk_write(grn_ctx *ctx, grn_obj *bulk,
                              const char *str, unsigned int len);
GRN_API grn_rc grn_bulk_write_from(grn_ctx *ctx, grn_obj *bulk,
                                   const char *str, unsigned int from, unsigned int len);
GRN_API grn_rc grn_bulk_reserve(grn_ctx *ctx, grn_obj *bulk, unsigned int len);
GRN_API grn_rc grn_bulk_space(grn_ctx *ctx, grn_obj *bulk, unsigned int len);
GRN_API grn_rc grn_bulk_truncate(grn_ctx *ctx, grn_obj *bulk, unsigned int len);
GRN_API grn_rc grn_bulk_fin(grn_ctx *ctx, grn_obj *bulk);

/* grn_text */

GRN_API grn_rc grn_text_itoa(grn_ctx *ctx, grn_obj *bulk, int i);
GRN_API grn_rc grn_text_lltoa(grn_ctx *ctx, grn_obj *bulk, long long int i);
GRN_API grn_rc grn_text_ftoa(grn_ctx *ctx, grn_obj *bulk, double d);
GRN_API grn_rc grn_text_itoh(grn_ctx *ctx, grn_obj *bulk, int i, unsigned int len);
GRN_API grn_rc grn_text_itob(grn_ctx *ctx, grn_obj *bulk, grn_id id);
GRN_API grn_rc grn_text_lltob32h(grn_ctx *ctx, grn_obj *bulk, long long int i);
GRN_API grn_rc grn_text_benc(grn_ctx *ctx, grn_obj *bulk, unsigned int v);
GRN_API grn_rc grn_text_esc(grn_ctx *ctx, grn_obj *bulk, const char *s, unsigned int len);
GRN_API grn_rc grn_text_urlenc(grn_ctx *ctx, grn_obj *buf,
                               const char *str, unsigned int len);

typedef struct _grn_obj_format grn_obj_format;

struct _grn_obj_format {
  const void *min;
  unsigned min_size;
  const void *max;
  unsigned max_size;
  int flags;
  int limit;
  int ncolumns;
  grn_obj **columns;
};

#define GRN_OBJ_FORMAT_ASARRAY         (0x01<<3)

GRN_API grn_rc grn_text_otoj(grn_ctx *ctx, grn_obj *bulk, grn_obj *obj,
                             grn_obj_format *format);

/* various values exchanged via grn_obj */

#define GRN_OBJ_DO_SHALLOW_COPY        (GRN_OBJ_REFER|GRN_OBJ_OUTPLACE)
#define GRN_OBJ_VECTOR                 (0x01<<7)

#define GRN_VALUE_FIX_SIZE_INIT(obj,flags,domain)\
  GRN_OBJ_INIT((obj), ((flags) & GRN_OBJ_VECTOR) ? GRN_UVECTOR : GRN_ATOM,\
               ((flags) & GRN_OBJ_DO_SHALLOW_COPY), (domain))
#define GRN_VALUE_VAR_SIZE_INIT(obj,flags,domain)\
  GRN_OBJ_INIT((obj), ((flags) & GRN_OBJ_VECTOR) ? GRN_VECTOR : GRN_BULK,\
               ((flags) & GRN_OBJ_DO_SHALLOW_COPY), (domain))

#define GRN_TEXT_INIT(obj,flags) \
  GRN_VALUE_VAR_SIZE_INIT(obj, flags, GRN_DB_TEXT)
#define GRN_SHORTTEXT_INIT(obj,flags) \
  GRN_VALUE_VAR_SIZE_INIT(obj, flags, GRN_DB_SHORTTEXT)
#define GRN_LONGTEXT_INIT(obj) \
  GRN_VALUE_VAR_SIZE_INIT(obj, flags, GRN_DB_LONGTEXT)
#define GRN_TEXT_SET_REF(obj,str,len) do {\
  (obj)->u.b.head = (char *)(str);\
  (obj)->u.b.curr = (char *)(str) + (len);\
} while (0)
#define GRN_TEXT_SET(ctx,obj,str,len) do {\
  if ((obj)->header.impl_flags & GRN_OBJ_REFER) {\
    GRN_TEXT_SET_REF((obj), (str), (len));\
  } else {\
    grn_bulk_write_from((ctx), (obj), (const char *)(str), 0, (unsigned int)(len));\
  }\
} while (0)
#define GRN_TEXT_PUT(ctx,obj,str,len) \
  grn_bulk_write((ctx), (obj), (const char *)(str), (unsigned int)(len))
#define GRN_TEXT_PUTC(ctx,obj,c) do {\
  char _c = (c); grn_bulk_write((ctx), (obj), &_c, 1);\
} while (0)

#define GRN_TEXT_PUTS(ctx,obj,str) GRN_TEXT_PUT((ctx), (obj), (str), strlen(str))
#define GRN_TEXT_SETS(ctx,obj,str) GRN_TEXT_SET((ctx), (obj), (str), strlen(str))
#define GRN_TEXT_VALUE(obj) GRN_BULK_HEAD(obj)
#define GRN_TEXT_LEN(obj) GRN_BULK_VSIZE(obj)

#define GRN_INT32_INIT(obj,flags) \
  GRN_VALUE_FIX_SIZE_INIT(obj, flags, GRN_DB_INT32)
#define GRN_UINT32_INIT(obj,flags) \
  GRN_VALUE_FIX_SIZE_INIT(obj, flags, GRN_DB_UINT32)
#define GRN_INT64_INIT(obj,flags) \
  GRN_VALUE_FIX_SIZE_INIT(obj, flags, GRN_DB_INT64)
#define GRN_UINT64_INIT(obj,flags) \
  GRN_VALUE_FIX_SIZE_INIT(obj, flags, GRN_DB_UINT64)
#define GRN_FLOAT_INIT(obj,flags) \
  GRN_VALUE_FIX_SIZE_INIT(obj, flags, GRN_DB_FLOAT)
#define GRN_TIME_INIT(obj,flags) \
  GRN_VALUE_FIX_SIZE_INIT(obj, flags, GRN_DB_TIME)
#define GRN_RECORD_INIT GRN_VALUE_FIX_SIZE_INIT
#define GRN_PTR_INIT(obj,flags,domain)\
  GRN_OBJ_INIT((obj), ((flags) & GRN_OBJ_VECTOR) ? GRN_PVECTOR : GRN_PTR,\
               ((flags) & GRN_OBJ_DO_SHALLOW_COPY), (domain))

#define GRN_INT32_SET(ctx,obj,val) do {\
  int _val = (int)(val);\
  grn_bulk_write_from((ctx), (obj), (char *)&_val, 0, sizeof(int));\
} while (0)
#define GRN_UINT32_SET(ctx,obj,val) do {\
  unsigned int _val = (unsigned int)(val);\
  grn_bulk_write_from((ctx), (obj), (char *)&_val, 0, sizeof(unsigned int));\
} while (0)
#define GRN_INT64_SET(ctx,obj,val) do {\
  long long int _val = (long long int)(val);\
  grn_bulk_write_from((ctx), (obj), (char *)&_val, 0, sizeof(long long int));\
} while (0)
#define GRN_UINT64_SET(ctx,obj,val) do {\
  long long unsigned int _val = (long long unsigned int)(val);\
  grn_bulk_write_from((ctx), (obj), (char *)&_val, 0, sizeof(long long unsigned int));\
} while (0)
#define GRN_FLOAT_SET(ctx,obj,val) do {\
  double _val = (double)(val);\
  grn_bulk_write_from((ctx), (obj), (char *)&_val, 0, sizeof(double));\
} while (0)
#define GRN_TIME_SET GRN_UINT64_SET
#define GRN_RECORD_SET(ctx,obj,val) do {\
  grn_id _val = (grn_id)(val);\
  grn_bulk_write_from((ctx), (obj), (char *)&_val, 0, sizeof(grn_id));\
} while (0)
#define GRN_PTR_SET(ctx,obj,val) \
  grn_bulk_write_from((ctx), (obj), (char *)val, 0, sizeof(grn_obj *))

#define GRN_INT32_VALUE(obj) (*((int *)GRN_BULK_HEAD(obj)))
#define GRN_UINT32_VALUE(obj) (*((unsigned int *)GRN_BULK_HEAD(obj)))
#define GRN_INT64_VALUE(obj) (*((long long int *)GRN_BULK_HEAD(obj)))
#define GRN_UINT64_VALUE(obj) (*((long long unsigned int *)GRN_BULK_HEAD(obj)))
#define GRN_FLOAT_VALUE(obj) (*((double *)GRN_BULK_HEAD(obj)))
#define GRN_TIME_VALUE GRN_UINT64_VALUE
#define GRN_RECORD_VALUE(obj) (*((grn_id *)GRN_BULK_HEAD(obj)))
#define GRN_PTR_VALUE(obj) ((grn_obj *)GRN_BULK_HEAD(obj))

#define GRN_INT32_PUT(ctx,obj,val) do {\
  int _val = (int)(val); grn_bulk_write((ctx), (obj), (char *)&_val, sizeof(int));\
} while (0)
#define GRN_UINT32_PUT(ctx,obj,val) do {\
  unsigned int _val = (unsigned  int)(val); grn_bulk_write((ctx), (obj), (char *)&_val, sizeof(unsigned int));\
} while (0)
#define GRN_INT64_PUT(ctx,obj,val) do {\
  long long int _val = (long long int)(val);\
  grn_bulk_write((ctx), (obj), (char *)&_val, sizeof(long long int));\
} while (0)
#define GRN_UINT64_PUT(ctx,obj,val) do {\
  long long unsigned int _val = (long long unsigned int)(val);\
  grn_bulk_write((ctx), (obj), (char *)&_val, sizeof(long long unsigned int));\
} while (0)
#define GRN_FLOAT_PUT(ctx,obj,val) do {\
  double _val = (double)(val); grn_bulk_write((ctx), (obj), (char *)&_val, sizeof(double));\
} while (0)
#define GRN_TIME_PUT GRN_INT64_PUT
#define GRN_RECORD_PUT(ctx,obj,val) do {\
  grn_id _val = (grn_id)(val); grn_bulk_write((ctx), (obj), (char *)&_val, sizeof(grn_id));\
} while (0)
#define GRN_PTR_PUT(ctx,obj,val) \
  grn_bulk_write((ctx), (obj), (char *)val, sizeof(grn_obj *))

/* grn_str */

typedef struct {
  const char *orig;
  char *norm;
  short *checks;
  unsigned char *ctypes;
  int flags;
  unsigned int orig_blen;
  unsigned int norm_blen;
  unsigned int length;
  grn_encoding encoding;
} grn_str;

#define GRN_STR_REMOVEBLANK            (0x01<<0)
#define GRN_STR_WITH_CTYPES            (0x01<<1)
#define GRN_STR_WITH_CHECKS            (0x01<<2)
#define GRN_STR_NORMALIZE              GRN_OBJ_KEY_NORMALIZE

GRN_API grn_str *grn_str_open(grn_ctx *ctx, const char *str, unsigned int str_len,
                              int flags);
GRN_API grn_rc grn_str_close(grn_ctx *ctx, grn_str *nstr);

GRN_API int grn_charlen(grn_ctx *ctx, const char *str, const char *end);

/* expr */

typedef enum {
  GRN_OP_PUSH = 0,
  GRN_OP_CALL,
  GRN_OP_INTERN,
  GRN_OP_TABLE_CREATE,
  GRN_OP_VAR_SET_VALUE,
  GRN_OP_OBJ_GET_VALUE,
  GRN_OP_OBJ_SET_VALUE,
  GRN_OP_OBJ_SEARCH,
  GRN_OP_TABLE_SCAN,
  GRN_OP_TABLE_SORT,
  GRN_OP_TABLE_GROUP,
  GRN_OP_JSON_PUT,
  GRN_OP_AND,
  GRN_OP_OR,
  GRN_OP_EQUAL,
  GRN_OP_NOT_EQUAL,
  GRN_OP_LESS,
  GRN_OP_GREATER,
  GRN_OP_LESS_EQUAL,
  GRN_OP_GREATER_EQUAL,
  GRN_OP_GEO_DISTANCE1,
  GRN_OP_GEO_DISTANCE2,
  GRN_OP_GEO_DISTANCE3,
  GRN_OP_GEO_DISTANCE4,
  GRN_OP_GEO_WITHINP5,
  GRN_OP_GEO_WITHINP6,
  GRN_OP_GEO_WITHINP8
} grn_op;

GRN_API grn_obj *grn_expr_create(grn_ctx *ctx, const char *name, unsigned name_size);
GRN_API grn_rc grn_expr_close(grn_ctx *ctx, grn_obj *expr);
GRN_API grn_obj *grn_expr_add_var(grn_ctx *ctx, grn_obj *expr,
                                  const char *name, unsigned name_size);
GRN_API grn_obj *grn_expr_get_var(grn_ctx *ctx, grn_obj *expr,
                                  const char *name, unsigned name_size);
GRN_API grn_obj *grn_expr_append_obj(grn_ctx *ctx, grn_obj *expr, grn_obj *obj);
GRN_API grn_obj *grn_expr_append_const(grn_ctx *ctx, grn_obj *expr, grn_obj *obj);
GRN_API grn_rc grn_expr_append_op(grn_ctx *ctx, grn_obj *expr, grn_op op, int nargs);
GRN_API grn_rc grn_expr_compile(grn_ctx *ctx, grn_obj *expr);
GRN_API grn_obj *grn_expr_exec(grn_ctx *ctx, grn_obj *expr);
GRN_API grn_obj *grn_expr_get_value(grn_ctx *ctx, grn_obj *expr, int offset);

GRN_API grn_rc grn_table_scan(grn_ctx *ctx, grn_obj *table, grn_obj *expr,
                              grn_obj *res, grn_sel_operator op);

/* ql */

#define GRN_QL_MORE                    (0x01<<0)
#define GRN_QL_TAIL                    (0x01<<1)
#define GRN_QL_HEAD                    (0x01<<2)
#define GRN_QL_QUIET                   (0x01<<3)
#define GRN_QL_QUIT                    (0x01<<4)
#define GRN_QL_FIN                     GRN_CTX_FIN

GRN_API grn_rc grn_ql_connect(grn_ctx *ctx, const char *host, int port, int flags);
GRN_API grn_rc grn_ql_load(grn_ctx *ctx, const char *path);
GRN_API grn_rc grn_ql_send(grn_ctx *ctx, char *str, unsigned int str_len, int flags);
GRN_API grn_rc grn_ql_recv(grn_ctx *ctx, char **str, unsigned int *str_len, int *flags);

typedef struct _grn_ql_info grn_ql_info;

struct _grn_ql_info {
  int fd;
  unsigned int com_status;
  grn_obj *outbuf;
  unsigned char stat;
};

GRN_API grn_rc grn_ql_info_get(grn_ctx *ctx, grn_ql_info *info);

/* hash */

typedef struct _grn_hash grn_hash;
typedef struct _grn_hash_cursor grn_hash_cursor;

GRN_API grn_hash *grn_hash_create(grn_ctx *ctx, const char *path, unsigned int key_size,
                                  unsigned int value_size, unsigned int flags);

GRN_API grn_hash *grn_hash_open(grn_ctx *ctx, const char *path);

GRN_API grn_rc grn_hash_close(grn_ctx *ctx, grn_hash *hash);

GRN_API grn_id grn_hash_lookup(grn_ctx *ctx, grn_hash *hash, const void *key,
                               unsigned int key_size, void **value, grn_search_flags *flags);

GRN_API grn_id grn_hash_add(grn_ctx *ctx, grn_hash *hash, const void *key,
                            unsigned int key_size, void **value, int *added);
GRN_API grn_id grn_hash_get(grn_ctx *ctx, grn_hash *hash, const void *key,
                            unsigned int key_size, void **value);

GRN_API int grn_hash_get_key(grn_ctx *ctx, grn_hash *hash, grn_id id, void *keybuf, int bufsize);
GRN_API int grn_hash_get_key2(grn_ctx *ctx, grn_hash *hash, grn_id id, grn_obj *bulk);
GRN_API int grn_hash_get_value(grn_ctx *ctx, grn_hash *hash, grn_id id, void *valuebuf);
GRN_API grn_rc grn_hash_set_value(grn_ctx *ctx, grn_hash *hash, grn_id id, void *value,
                                  int flags);

typedef struct _grn_table_delete_optarg grn_table_delete_optarg;

struct _grn_table_delete_optarg {
  int flags;
  int (*func)(grn_ctx *ctx, grn_obj *, grn_id, void *);
  void *func_arg;
};

GRN_API grn_rc grn_hash_delete_by_id(grn_ctx *ctx, grn_hash *hash, grn_id id,
                                     grn_table_delete_optarg *optarg);
GRN_API grn_rc grn_hash_delete(grn_ctx *ctx, grn_hash *hash,
                               const void *key, unsigned int key_size,
                               grn_table_delete_optarg *optarg);

GRN_API grn_hash_cursor *grn_hash_cursor_open(grn_ctx *ctx, grn_hash *hash,
                                              const void *min, unsigned int min_size,
                                              const void *max, unsigned int max_size, int flags);
GRN_API grn_id grn_hash_cursor_next(grn_ctx *ctx, grn_hash_cursor *c);
GRN_API void grn_hash_cursor_close(grn_ctx *ctx, grn_hash_cursor *c);

GRN_API int grn_hash_cursor_get_key(grn_ctx *ctx, grn_hash_cursor *c, void **key);
GRN_API int grn_hash_cursor_get_value(grn_ctx *ctx, grn_hash_cursor *c, void **value);
GRN_API grn_rc grn_hash_cursor_set_value(grn_ctx *ctx, grn_hash_cursor *c,
                                         void *value, int flags);

GRN_API int grn_hash_cursor_get_key_value(grn_ctx *ctx, grn_hash_cursor *c,
                                          void **key, unsigned int *key_size, void **value);

GRN_API grn_rc grn_hash_cursor_delete(grn_ctx *ctx, grn_hash_cursor *c,
                                      grn_table_delete_optarg *optarg);

#define GRN_HASH_EACH(hash,id,key,key_size,value,block) do {\
  grn_hash_cursor *_sc = grn_hash_cursor_open(ctx, hash, NULL, 0, NULL, 0, 0);\
  if (_sc) {\
    grn_id id;\
    while ((id = grn_hash_cursor_next(ctx, _sc))) {\
      grn_hash_cursor_get_key_value(ctx, _sc, (void **)(key),\
                                    (key_size), (void **)(value));\
      block\
    }\
    grn_hash_cursor_close(ctx, _sc);\
  }\
} while (0)

/* pat */

typedef struct _grn_pat grn_pat;
typedef struct _grn_pat_cursor grn_pat_cursor;

GRN_API grn_pat *grn_pat_create(grn_ctx *ctx, const char *path, unsigned int key_size,
                                unsigned int value_size, unsigned int flags);

GRN_API grn_pat *grn_pat_open(grn_ctx *ctx, const char *path);

GRN_API grn_rc grn_pat_close(grn_ctx *ctx, grn_pat *pat);

GRN_API grn_rc grn_pat_remove(grn_ctx *ctx, const char *path);

GRN_API grn_id grn_pat_lookup(grn_ctx *ctx, grn_pat *pat, const void *key, int key_size,
                              void **value, grn_search_flags *flags);
GRN_API grn_id grn_pat_get(grn_ctx *ctx, grn_pat *pat, const void *key,
                           unsigned int key_size, void **value);
GRN_API grn_id grn_pat_add(grn_ctx *ctx, grn_pat *pat, const void *key,
                           unsigned int key_size, void **value, int *added);

GRN_API int grn_pat_get_key(grn_ctx *ctx, grn_pat *pat, grn_id id, void *keybuf, int bufsize);
GRN_API int grn_pat_get_key2(grn_ctx *ctx, grn_pat *pat, grn_id id, grn_obj *bulk);
GRN_API int grn_pat_get_value(grn_ctx *ctx, grn_pat *pat, grn_id id, void *valuebuf);
GRN_API grn_rc grn_pat_set_value(grn_ctx *ctx, grn_pat *pat, grn_id id,
                                 void *value, int flags);

GRN_API grn_rc grn_pat_delete_by_id(grn_ctx *ctx, grn_pat *pat, grn_id id,
                                    grn_table_delete_optarg *optarg);
GRN_API grn_rc grn_pat_delete(grn_ctx *ctx, grn_pat *pat, const void *key, unsigned int key_size,
                              grn_table_delete_optarg *optarg);
GRN_API int grn_pat_delete_with_sis(grn_ctx *ctx, grn_pat *pat, grn_id id,
                                    grn_table_delete_optarg *optarg);

typedef struct _grn_pat_scan_hit grn_pat_scan_hit;

struct _grn_pat_scan_hit {
  grn_id id;
  unsigned int offset;
  unsigned int length;
};

GRN_API int grn_pat_scan(grn_ctx *ctx, grn_pat *pat, const char *str, unsigned int str_len,
                         grn_pat_scan_hit *sh, unsigned int sh_size, const char **rest);

GRN_API grn_rc grn_pat_prefix_search(grn_ctx *ctx, grn_pat *pat,
                                     const void *key, unsigned int key_size, grn_hash *h);
GRN_API grn_rc grn_pat_suffix_search(grn_ctx *ctx, grn_pat *pat,
                                     const void *key, unsigned int key_size, grn_hash *h);
GRN_API grn_id grn_pat_lcp_search(grn_ctx *ctx, grn_pat *pat,
                                  const void *key, unsigned int key_size);

GRN_API unsigned int grn_pat_size(grn_ctx *ctx, grn_pat *pat);

GRN_API grn_pat_cursor *grn_pat_cursor_open(grn_ctx *ctx, grn_pat *pat,
                                            const void *min, unsigned int min_size,
                                            const void *max, unsigned int max_size, int flags);
GRN_API grn_id grn_pat_cursor_next(grn_ctx *ctx, grn_pat_cursor *c);
GRN_API void grn_pat_cursor_close(grn_ctx *ctx, grn_pat_cursor *c);

GRN_API int grn_pat_cursor_get_key(grn_ctx *ctx, grn_pat_cursor *c, void **key);
GRN_API int grn_pat_cursor_get_value(grn_ctx *ctx, grn_pat_cursor *c, void **value);

GRN_API int grn_pat_cursor_get_key_value(grn_ctx *ctx, grn_pat_cursor *c,
                                         void **key, unsigned int *key_size, void **value);
GRN_API grn_rc grn_pat_cursor_set_value(grn_ctx *ctx, grn_pat_cursor *c,
                                        void *value, int flags);
GRN_API grn_rc grn_pat_cursor_delete(grn_ctx *ctx, grn_pat_cursor *c,
                                     grn_table_delete_optarg *optarg);

#define GRN_PAT_EACH(pat,id,key,key_size,value,block) do {\
  grn_pat_cursor *_sc = grn_pat_cursor_open(ctx, pat, NULL, 0, NULL, 0, 0);\
  if (_sc) {\
    grn_id id;\
    while ((id = grn_pat_cursor_next(ctx, _sc))) {\
      grn_pat_cursor_get_key_value(ctx, _sc, (void **)(key),\
                                    (key_size), (void **)(value));\
      block\
    }\
    grn_pat_cursor_close(ctx, _sc);\
  }\
} while (0)

#ifdef __cplusplus
}
#endif

#endif /* GROONGA_H */
