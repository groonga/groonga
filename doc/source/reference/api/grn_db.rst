.. -*- rst -*-

``grn_db``
==========

Summary
-------

TODO...

Example
-------

TODO...

Reference
---------

TODO...

.. c:type:: grn_db

   TODO...

.. c:type:: grn_db_create_optarg

   It is used for specifying options for :c:func:`grn_db_create`.

.. c:member:: char **grn_db_create_optarg.builtin_type_names

   組み込み型の名前となるnul終端文字列の配列を指定する。

.. c:member:: int grn_db_create_optarg.n_builtin_type_names

   n_builtin_type_namesには、optarg.builtin_type_namesで指定する文字列の数を
   指定する。配列のoffsetはenum型grn_builtin_typeの値に対応する。

.. c:function:: grn_obj *grn_db_create(grn_ctx *ctx, const char *path, grn_db_create_optarg *optarg)

   新たなdbを作成します。

   :param ctx: 初期化済みの :c:type:`grn_ctx` を指定します。
   :param path: 作成するdbを格納するファイルパスを指定します。NULLならtemporary dbとなります。NULL以外のパスを指定した場合はpersistent dbとなります。
   :param optarg:
      Currently, it is not used. It is just ignored.

      作成するdbの組み込み型の名前を変更する時に指定します。

      optarg.builtin_type_namesには、組み込み型の名前となるnull終端文字列の配列を指定します。optarg.n_builtin_type_namesには、optarg.builtin_type_namesで指定する文字列の数を指定します。配列のoffsetはenum型grn_builtin_typeの値に対応します。

.. c:function:: grn_obj *grn_db_open(grn_ctx *ctx, const char *path)

   既存のdbを開きます。

   :param path: 開こうとするdbを格納するファイルパスを指定します。

.. c:function:: void grn_db_touch(grn_ctx *ctx, grn_obj *db)

   dbの内容の最終更新時刻を現在時刻にします。

   最終更新時刻はキャッシュが有効かどうかの判断などに利用されます。

   :param db: 内容が変更されたdbを指定します。

.. c:function:: grn_obj *grn_obj_db(grn_ctx *ctx, grn_obj *obj)

   objの属するdbを返します。

   :param obj: 対象objectを指定します。

.. c:function:: grn_rc grn_db_recover(grn_ctx *ctx, grn_obj *db)

   .. note::

      This is an experimental API.

   .. note::

      This is a dangerous API. You must not use this API when other
      thread or process opens the target database. If you use this API
      against shared database, the database may be broken.

   .. versionadded:: 4.0.9

   Checks the passed database and recovers it if it is broken and it
   can be recovered.

   This API uses lock existence for checking whether the database is
   broken or not.

   Here are recoverable cases:

     * Index column is broken. The index column must have source column.

   Here are unrecoverable cases:

     * Object name management feature is broken.
     * Table is broken.
     * Data column is broken.

   Object name management feature is used for managing table name,
   column name and so on. If the feature is broken, the database can't
   be recovered. Please re-create the database from backup.

   Table and data column can be recovered by removing an existence
   lock and re-add data.

   :param db: The database to be recovered.
   :return: ``GRN_SUCCESS`` on success, not ``GRN_SUCCESS`` on error.

.. c:function:: grn_rc grn_db_unmap(grn_ctx *ctx, grn_obj *db)

   .. note::

      This is an experimental API.

   .. note::

      This is a thread unsafe API. You can't touch the database while
      this API is running.

   .. versionadded:: 5.0.7

   Unmaps all opened tables and columns in the passed
   database. Resources used by these opened tables and columns are
   freed.

   Normally, this API isn't useless. Because resources used by opened
   tables and columns are managed by OS automatically.

   :param db: The database to be recovered.
   :return: ``GRN_SUCCESS`` on success, not ``GRN_SUCCESS`` on error.
