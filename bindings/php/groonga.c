
#include "php_groonga.h"

#if HAVE_GROONGA

int le_grn_ctx;
void grn_ctx_dtor(zend_rsrc_list_entry *rsrc TSRMLS_DC)
{
  grn_ctx *ctx = (grn_ctx *)(rsrc->ptr);
  grn_ctx_close(ctx);
}

function_entry groonga_functions[] = {
  PHP_FE(grn_ctx_init        , grn_ctx_init_arg_info)
  PHP_FE(grn_ctx_close       , grn_ctx_close_arg_info)
  PHP_FE(grn_ql_connect      , grn_ql_connect_arg_info)
  PHP_FE(grn_ql_send         , grn_ql_send_arg_info)
  PHP_FE(grn_ql_recv         , grn_ql_recv_arg_info)
  { NULL, NULL, NULL }
};


zend_module_entry groonga_module_entry = {
  STANDARD_MODULE_HEADER,
  "groonga",
  groonga_functions,
  PHP_MINIT(groonga),
  PHP_MSHUTDOWN(groonga),
  PHP_RINIT(groonga),
  PHP_RSHUTDOWN(groonga),
  PHP_MINFO(groonga),
  "0.1",
  STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_GROONGA
ZEND_GET_MODULE(groonga)
#endif


PHP_MINIT_FUNCTION(groonga)
{
  REGISTER_LONG_CONSTANT("GRN_CTX_USE_QL", GRN_CTX_USE_QL, CONST_PERSISTENT | CONST_CS);
  REGISTER_LONG_CONSTANT("GRN_CTX_BATCH_MODE", GRN_CTX_BATCH_MODE, CONST_PERSISTENT | CONST_CS);
  REGISTER_LONG_CONSTANT("GRN_ENC_DEFAULT", GRN_ENC_DEFAULT, CONST_PERSISTENT | CONST_CS);
  REGISTER_LONG_CONSTANT("GRN_ENC_NONE", GRN_ENC_NONE, CONST_PERSISTENT | CONST_CS);
  REGISTER_LONG_CONSTANT("GRN_ENC_EUC_JP", GRN_ENC_EUC_JP, CONST_PERSISTENT | CONST_CS);
  REGISTER_LONG_CONSTANT("GRN_ENC_UTF8", GRN_ENC_UTF8, CONST_PERSISTENT | CONST_CS);
  REGISTER_LONG_CONSTANT("GRN_ENC_SJIS", GRN_ENC_SJIS, CONST_PERSISTENT | CONST_CS);
  REGISTER_LONG_CONSTANT("GRN_ENC_LATIN1", GRN_ENC_LATIN1, CONST_PERSISTENT | CONST_CS);
  REGISTER_LONG_CONSTANT("GRN_ENC_KOI8R", GRN_ENC_KOI8R, CONST_PERSISTENT | CONST_CS);
  REGISTER_LONG_CONSTANT("GRN_QL_MORE", GRN_QL_MORE, CONST_PERSISTENT | CONST_CS);
  REGISTER_LONG_CONSTANT("GRN_QL_TAIL", GRN_QL_TAIL, CONST_PERSISTENT | CONST_CS);
  REGISTER_LONG_CONSTANT("GRN_QL_HEAD", GRN_QL_HEAD, CONST_PERSISTENT | CONST_CS);
  REGISTER_LONG_CONSTANT("GRN_QL_QUIET", GRN_QL_QUIET, CONST_PERSISTENT | CONST_CS);
  REGISTER_LONG_CONSTANT("GRN_QL_QUIT", GRN_QL_QUIT, CONST_PERSISTENT | CONST_CS);
  REGISTER_LONG_CONSTANT("GRN_QL_FIN", GRN_QL_FIN, CONST_PERSISTENT | CONST_CS);
  le_grn_ctx = zend_register_list_destructors_ex(
               grn_ctx_dtor, NULL, "grn_ctx", module_number);

  grn_init();

  return SUCCESS;
}


PHP_MSHUTDOWN_FUNCTION(groonga)
{
  grn_fin();
  return SUCCESS;
}


PHP_RINIT_FUNCTION(groonga)
{
  return SUCCESS;
}


PHP_RSHUTDOWN_FUNCTION(groonga)
{
  return SUCCESS;
}


PHP_MINFO_FUNCTION(groonga)
{
  php_info_print_box_start(0);
  php_printf("<p>Groonga</p>\n");
  php_printf("<p>Version 0.1 (ctx, ql)</p>\n");
  php_printf("<p><b>Authors:</b></p>\n");
  php_printf("<p>yu &lt;yu@irx.jp&gt; (lead)</p>\n");
  php_info_print_box_end();
}


PHP_FUNCTION(grn_ctx_init)
{
  grn_ctx *ctx = (grn_ctx *) malloc(sizeof(grn_ctx));
  long res_id = -1;
  long flags = 0;
  grn_rc rc;


  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|l", &flags) == FAILURE) {
    return;
  }

  if ((rc = grn_ctx_init(ctx, flags)) != GRN_SUCCESS) {
    RETURN_FALSE;
  }

  res_id = ZEND_REGISTER_RESOURCE(return_value, ctx, le_grn_ctx);
  RETURN_RESOURCE(res_id);
}


PHP_FUNCTION(grn_ctx_close)
{
  zval *res = NULL;
  int res_id = -1;

  grn_ctx *ctx;
  grn_rc rc;


  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &res) == FAILURE) {
    return;
  }

  ZEND_FETCH_RESOURCE(ctx, grn_ctx *, &res, res_id, "grn_ctx", le_grn_ctx);

  if ((rc = grn_ctx_close(ctx)) != GRN_SUCCESS) {
    RETURN_FALSE;
  }

  RETURN_TRUE;
}


PHP_FUNCTION(grn_ql_connect)
{
  zval *res = NULL;
  int res_id = -1;

  grn_rc rc;
  grn_ctx *ctx;
  char  *host = "localhost";
  int host_len = 0;
  long port = 10041;
  long flags = 0;


  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs|ll", &res, &host, &host_len, &port, &flags) == FAILURE) {
    return;
  }

  ZEND_FETCH_RESOURCE(ctx, grn_ctx *, &res, res_id, "grn_ctx", le_grn_ctx);

  if ((rc = grn_ql_connect(ctx, host, port, flags)) != GRN_SUCCESS) {
    RETURN_FALSE;
  }

  RETURN_TRUE;
}


PHP_FUNCTION(grn_ql_send)
{
  zval *res = NULL;
  int res_id = -1;

  grn_rc rc;
  grn_ctx *ctx;
  char *query = NULL;
  unsigned int query_len = 0;
  long flags = 0;


  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs|l", &res, &query, &query_len, &flags) == FAILURE) {
    return;
  }

  ZEND_FETCH_RESOURCE(ctx, grn_ctx *, &res, res_id, "grn_ctx", le_grn_ctx);

  if ((rc = grn_ql_send(ctx, query, query_len, flags)) != GRN_SUCCESS) {
    RETURN_FALSE;
  }

  RETURN_TRUE;
}


PHP_FUNCTION(grn_ql_recv)
{
  zval *res = NULL;
  int res_id = -1;
  grn_ctx *ctx;

  char *str;
  int flags;
  unsigned int str_len;
  grn_rc rc;


  array_init(return_value);

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &res) == FAILURE) {
    return;
  }

  ZEND_FETCH_RESOURCE(ctx, grn_ctx *, &res, res_id, "grn_ctx", le_grn_ctx);

  rc = grn_ql_recv(ctx, &str, &str_len, &flags);
  add_next_index_stringl(return_value, str, str_len, 1);
  add_next_index_long(return_value, flags);
}

#endif /* HAVE_GROONGA */
