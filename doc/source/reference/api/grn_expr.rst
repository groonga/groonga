.. -*- rst -*-

.. highlight:: c

grn_expr
========

`grn_expr` is an :c:type:`grn_obj` that represents an expression. Here
is a list of what expression can do:

  * Expression can apply some operations to a record by
    :c:func:`grn_expr_exec`.
  * Expression can represents search condition. :c:func:`grn_table_select`
    can select records that match against the search condition represented
    by expression.

There are two string representations of expression:

  * :doc:`/reference/grn_expr/query_syntax`
  * :doc:`/reference/grn_expr/script_syntax`

:c:func:`grn_expr_parse` parses string represented expression and
appends the parsed expression to another expression.

Example
-------

TODO...

Reference
---------

.. c:function:: grn_obj *grn_expr_create(grn_ctx *ctx, const char *name, unsigned int name_size)

.. c:function:: grn_rc grn_expr_close(grn_ctx *ctx, grn_obj *expr)

.. c:function:: grn_obj *grn_expr_add_var(grn_ctx *ctx, grn_obj *expr, const char *name, unsigned int name_size)

.. c:function:: grn_obj *grn_expr_get_var_by_offset(grn_ctx *ctx, grn_obj *expr, unsigned int offset)

.. c:function:: grn_obj *grn_expr_append_obj(grn_ctx *ctx, grn_obj *expr, grn_obj *obj, grn_operator op, int nargs)

.. c:function:: grn_obj *grn_expr_append_const(grn_ctx *ctx, grn_obj *expr, grn_obj *obj, grn_operator op, int nargs)

.. c:function:: grn_obj *grn_expr_append_const_str(grn_ctx *ctx, grn_obj *expr, const char *str, unsigned int str_size, grn_operator op, int nargs)

.. c:function:: grn_obj *grn_expr_append_const_int(grn_ctx *ctx, grn_obj *expr, int i, grn_operator op, int nargs)

.. c:function:: grn_rc grn_expr_append_op(grn_ctx *ctx, grn_obj *expr, grn_operator op, int nargs)

.. c:function:: grn_rc grn_expr_get_keywords(grn_ctx *ctx, grn_obj *expr, grn_obj *keywords)

   Extracts keywords from ``expr`` and stores to
   ``keywords``. Keywords in ``keywords`` are owned by ``expr``. Don't
   unlink them. Each keyword is ``GRN_BULK`` and its domain is
   ``GRN_DB_TEXT``.

   ``keywords`` must be ``GRN_PVECTOR``.

   Here is an example code::

      grn_obj keywords;
      GRN_PTR_INIT(&keywords, GRN_OBJ_VECTOR, GRN_ID_NIL);
      grn_expr_get_keywords(ctx, expr, &keywords);
      {
        int i, n_keywords;
        n_keywords = GRN_BULK_VSIZE(&keywords) / sizeof(grn_obj *);
        for (i = 0; i < n_keywords; i++) {
          grn_obj *keyword = GRN_PTR_VALUE_AT(&keywords, i);
          const char *keyword_content;
          int keyword_size;
          keyword_content = GRN_TEXT_VALUE(keyword);
          keyword_size = GRN_TEXT_LEN(keyword);
          /*
            Use keyword_content and keyword_size.
            You don't need to unlink keyword.
            keyword is owned by expr.
          */
        }
      }
      GRN_OBJ_FIN(ctx, &keywords);


   :param ctx: The context that creates the ``expr``.
   :param expr: The expression to be extracted.
   :param keywords: The container to store extracted keywords.
                    It must be ``GRN_PVECTOR``.

                    Each extracted keyword is ``GRN_BULK`` and its
                    domain is ``GRN_DB_TEXT``.

                    Extracted keywords are owned by ``expr``. Don't
                    unlink them.
   :return: ``GRN_SUCCESS`` on success, not ``GRN_SUCCESS`` on error.

.. c:function:: grn_rc grn_expr_syntax_escape(grn_ctx *ctx, const char *string, int string_size, const char *target_characters, char escape_character, grn_obj *escaped_string)

   Escapes ``target_characters`` in ``string`` by ``escape_character``.

   :param ctx: Its encoding must be the same encoding of ``string``.
               It is used for allocating buffer for ``escaped_string``.
   :param string: String expression representation.
   :param string_size: The byte size of ``string``. ``-1`` means ``string``
                       is NULL terminated string.
   :param target_characters: NULL terminated escape target characters.
                             For example, ``"+-><~*()\"\\:"`` is
                             ``target_characters`` for
                             :doc:`/reference/grn_expr/query_syntax`.
   :param escape_character: The character to use escape a character in
                            ``target_characters``. For example, ``\\``
                            (backslash) is ``escaped_character`` for
                            :doc:`/reference/grn_expr/query_syntax`.
   :param escaped_string: The output of escaped ``string``. It should be
                          text typed bulk.
   :return: ``GRN_SUCCESS`` on success, not ``GRN_SUCCESS`` on error.

.. c:function:: grn_rc grn_expr_syntax_escape_query(grn_ctx *ctx, const char *query, int query_size, grn_obj *escaped_query)

   Escapes special characters in
   :doc:`/reference/grn_expr/query_syntax`.

   :param ctx: Its encoding must be the same encoding of ``query``.
               It is used for allocating buffer for ``escaped_query``.
   :param query: String expression representation in
                 :doc:`/reference/grn_expr/query_syntax`.
   :param query_size: The byte size of ``query``. ``-1`` means ``query``
                      is NULL terminated string.
   :param escaped_query: The output of escaped ``query``. It should be
                         text typed bulk.
   :return: ``GRN_SUCCESS`` on success, not ``GRN_SUCCESS`` on error.

.. c:function:: grn_rc grn_expr_compile(grn_ctx *ctx, grn_obj *expr)

.. c:function:: grn_obj *grn_expr_exec(grn_ctx *ctx, grn_obj *expr, int nargs)

.. c:function:: grn_obj *grn_expr_alloc(grn_ctx *ctx, grn_obj *expr, grn_id domain, grn_obj_flags flags)
