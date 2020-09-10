.. -*- rst -*-

.. highlightlang:: none

.. groonga-command
.. database: commands_table_create

``table_create``
================

Summary
-------

``table_create`` creates a new table in the current database. You need
to create one or more tables to store and search data.

See :doc:`/reference/tables` for table details.

Syntax
------

This command takes many parameters.

The required parameter is only ``name`` and otehrs are optional::

  table_create name
               [flags=TABLE_HASH_KEY]
               [key_type=null]
               [value_type=null]
               [default_tokenizer=null]
               [normalizer=null]
               [token_filters=null]
               [path=null]

Usage
-----

This section describes about the followings:

  * :ref:`table-create-data-store`
  * :ref:`table-create-large-data-store`
  * :ref:`table-create-lexicon`
  * :ref:`table-create-tag-index-table`
  * :ref:`table-create-range-index-table`

.. _table-create-data-store:

Create data store table
^^^^^^^^^^^^^^^^^^^^^^^

You can use all table types for data store table. See
:doc:`/reference/tables` for all table types.

Table type is specified as ``TABLE_${TYPE}`` to ``flags`` parameter.

Here is an example to create ``TABLE_NO_KEY`` table:

.. groonga-command
.. include:: ../../example/reference/commands/table_create/data_store_table_no_key.log
.. table_create Logs TABLE_NO_KEY

The ``table_create`` command creates a table that is named ``Logs``
and is ``TABLE_NO_KEY`` type.

If your records aren't searched by key, ``TABLE_NO_KEY`` type table is
suitable. Because ``TABLE_NO_KEY`` doesn't support key but it is fast
and small table. Storing logs into Groonga database is the case.

If your records are searched by key or referenced by one or more
columns, ``TABLE_NO_KEY`` type isn't suitable. Lexicon for fulltext
search is the case.

.. _table-create-large-data-store:

Create large data store table
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

If you want to store many large keys, your table may not be able to
store them. If total key data is larger than 4GiB, you can't store all
key data into your table by default.

You can expand the maximum total key size to 1TiB from 4GiB by
``KEY_LARGE`` flag. ``KEY_LARGE`` flag can be used with only
``TABLE_HASH_KEY``. You can't use ``KEY_LARGE`` flag with
``TABLE_NO_KEY``, ``TABLE_PAT_KEY`` nor ``TABLE_DAT_KEY``.

Here is an example to create a table that can store many large keys:

.. groonga-command
.. include:: ../../example/reference/commands/table_create/large_data_store_table.log
.. table_create Paths TABLE_HASH_KEY|KEY_LARGE ShortText

The ``table_create`` command creates a table that is named ``Paths``
and is ``TABLE_HASH_KEY`` type. The ``Paths`` table can store many
large keys.

.. _table-create-lexicon:

Create lexicon
^^^^^^^^^^^^^^

You can use all table types except ``TABLE_NO_KEY`` for lexicon.
Lexicon needs key support but ``TABLE_NO_KEY`` doesn't support key.

Here is an example to create ``TABLE_PAT_KEY`` table:

.. groonga-command
.. include:: ../../example/reference/commands/table_create/lexicon_pat_key.log
.. table_create Lexicon TABLE_PAT_KEY ShortText --default_tokenizer TokenBigram --normalizer NormalizerAuto

The ``table_create`` command creates the following table:

* The table is named ``Lexicon``.
* The table is ``TABLE_PAT_KEY`` type table.
* The table's key is ``ShortText`` type.
* The table uses ``TokenBigram`` tokenizer to extract tokens from a
  normalized text.
* The table uses ``NormalizerAuto`` normalizer to normalize a text.

``TABLE_PAT_KEY`` is suitable table type for lexicon. Lexicon is used
for fulltext search.

In fulltext search, predictive search may be used for fuzzy
search. Predictive search is supported by ``TABLE_PAT_KEY`` and
``TABLE_DAT_KEY``.

Lexicon has many keys because a fulltext target text has many
tokens. Table that has many keys should consider table size because
large table requires large memory. Requiring large memory causes disk
I/O. It blocks fast search. So table size is important for a table
that has many keys. ``TABLE_PAT_KEY`` is less table size than
``TABLE_DAT_KEY``.

Because of the above reasons, ``TABLE_PAT_KEY`` is suitable table type
for lexicon.

.. _table-create-tag-index-table:

Create tag index table
^^^^^^^^^^^^^^^^^^^^^^

You can use all table types except ``TABLE_NO_KEY`` for tag index
table. Tag index table needs key support but ``TABLE_NO_KEY`` doesn't
support key.

Here is an example to create ``TABLE_HASH_KEY`` table:

.. groonga-command
.. include:: ../../example/reference/commands/table_create/tag_index_table_hash_key.log
.. table_create Tags TABLE_HASH_KEY ShortText

The ``table_create`` command creates a table that is named ``Tags``,
is ``TABLE_HASH_KEY`` type and has ``ShortText`` type key.

``TABLE_HASH_KEY`` or ``TABLE_DAT_KEY`` are suitable table types for
tag index table.

If you need only exact match tag search feature, ``TABLE_HASH_KEY`` is
suitable. It is the common case.

If you also need predictive tag search feature (for example, searching
``"groonga"`` by ``"gr"`` keyword.), ``TABLE_DAT_KEY`` is suitable.
``TABLE_DAT_KEY`` is large table size but it is not important because
the number of tags will not be large.

.. _table-create-range-index-table:

Create range index table
^^^^^^^^^^^^^^^^^^^^^^^^

You can use ``TABLE_PAT_KEY`` and ``TABLE_DAT_KEY`` table types for
range index table. Range index table needs range search support but
``TABLE_NO_KEY`` and ``TABLE_HASH_KEY`` don't support it.

Here is an example to create ``TABLE_DAT_KEY`` table:

.. groonga-command
.. include:: ../../example/reference/commands/table_create/range_index_table_dat_key.log
.. table_create Ages TABLE_DAT_KEY UInt32

The ``table_create`` command creates a table that is named ``Ages``,
is ``TABLE_DAT_KEY`` type and has ``UInt32`` type key.

``TABLE_PAT_KEY`` and ``TABLE_DAT_KEY`` are suitable table types for
range index table.

If you don't have many indexed items, ``TABLE_DAT_KEY`` is
suitable. Index for age is the case in the above example. Index for
age will have only 0-100 items because human doesn't live so long.

If you have many indexed items, ``TABLE_PAT_KEY`` is suitable. Because
``TABLE_PAT_KEY`` is smaller than ``TABLE_DAT_KEY``.

Parameters
----------

This section describes all parameters.

Required parameters
^^^^^^^^^^^^^^^^^^^

There is only one required parameter.

.. _table-create-name:

``name``
""""""""

Specifies a table name to be created. ``name`` must be specified.

Here are available characters:

* ``0`` .. ``9`` (digit)
* ``a`` .. ``z`` (alphabet, lower case)
* ``A`` .. ``Z`` (alphabet, upper case)
* ``#`` (hash)
* ``@`` (at mark)
* ``-`` (hyphen)
* ``_`` (underscore) (NOTE: Underscore can't be used as the first
  character.)

You need to create a name with one or more the above characters. Note
that you can't use ``_`` as the first character such as ``_name``.

Optional parameters
^^^^^^^^^^^^^^^^^^^

There are some optional parameters.

.. _table-create-flags:

``flags``
"""""""""

Specifies a table type and table customize options.

Here are available flags:

.. list-table::
   :header-rows: 1

   * - Flag
     - Description
   * - ``TABLE_NO_KEY``
     - Array table. See also :ref:`table-no-key`.
   * - ``TABLE_HASH_KEY``
     - Hash table. See also :ref:`table-hash-key`.
   * - ``TABLE_PAT_KEY``
     - Patricia trie. See also :ref:`table-pat-key`.
   * - ``TABLE_DAT_KEY``
     - Double array trie. See also :ref:`table-dat-key`.
   * - ``KEY_WITH_SIS``
     - Enable Semi Infinite String. Require ``TABLE_PAT_KEY``.
   * - ``KEY_LARGE``
     - Expand the maximum total key size to 1TiB from 4GiB. Require
       ``TABLE_HASH_KEY``.

.. note::
   Since Groonga 2.1.0 ``KEY_NORMALIZE`` flag is deprecated. Use
   ``normalizer`` option with ``NormalizerAuto`` instead.

You must specify one of ``TABLE_${TYPE}`` flags. You can't specify two
or more ``TABLE_${TYPE}`` flags. For example,
``TABLE_NO_KEY|TABLE_HASH_KEY`` is invalid.

You can combine flags with ``|`` (vertical bar) such as
``TABLE_PAT_KEY|KEY_WITH_SIS``.

See :doc:`/reference/tables` for difference between table types.

The default flags are ``TABLE_HASH_KEY``.

.. _table-create-key-type:

``key_type``
""""""""""""

Specifies key type.

If you specify ``TABLE_HASH_KEY``, ``TABLE_PAT_KEY`` or
``TABLE_DAT_KEY`` as ``flags`` parameter, you need to specify
``key_type`` option.

See :doc:`/reference/types` for all types.

The default value is none.

.. _table-create-value-type:

``value_type``
""""""""""""""

Specifies value type.

You can use value when you specify ``TABLE_NO_KEY``,
``TABLE_HASH_KEY`` or ``TABLE_PAT_KEY`` as ``flags`` parameter. Value
type must be a fixed size type. For example, ``UInt32`` can be used
but ``ShortText`` cannot be used. Use columns instead of value.

The default value is none.

.. _table-create-default-tokenizer:

``default_tokenizer``
"""""""""""""""""""""

Specifies the default tokenizer that is used on searching and data
loading.

You must specify ``default_tokenizer`` for a table that is used for
lexicon of fulltext search index. See :doc:`/reference/tokenizers` for
available tokenizers. You must choose a tokenizer from the list for
fulltext search.

You don't need to specify ``default_tokenizer`` in the following
cases:

  * You don't use the table as a lexicon.

  * You use the table as a lexicon but you don't need fulltext
    search. For example:

      * Index target data isn't text data such as ``Int32`` and ``Time``.
      * You just need exact match search, prefix search and so on.

You can't use ``default_tokenizer`` with ``TABLE_NO_KEY`` flag because
a table that uses ``TABLE_NO_KEY`` flag can't be used as lexicon.

You must specify ``TABLE_HASH_KEY``, ``TABLE_PAT_KEY``,
``TABLE_DAT_KEY`` to :ref:`table-create-flags` when you want to use
the table as a lexicon.

The default value is none.

.. _table-create-normalizer:

``normalizer``
""""""""""""""

Specifies a normalizer that is used to normalize key.

You cannot use ``normalizer`` with ``TABLE_NO_KEY`` because
``TABLE_NO_KEY`` doesn't support key.

See :doc:`/reference/normalizers` for all normalizsers.

The default value is none.

.. _table-create-token-filters:

``token_filters``
"""""""""""""""""

Specifies token filters separated by ``,``.
Token filters are used to process tokens.

You cannot use ``token_filters`` with ``TABLE_NO_KEY`` because
``TABLE_NO_KEY`` doesn't support key.

See :doc:`/reference/token_filters` for all token filters.

The default value is none.

``path``
""""""""

.. versionadded:: 10.0.7

Specifies a path for storing a table.

This option is useful if you want to store a table that you often use to fast
storage (e.g. SSD) and store it that you don't often use to slow storage (e.g. HDD).

You can use a relative path or an absolute path in this option.
If you specify a relative path, it is resolved from the current directory for the ``groonga`` process.

The default value is none.

Return value
------------

``table_create`` returns ``true`` as body on success such as::

  [HEADER, true]

If ``table_create`` fails, error details are in ``HEADER``.

See :doc:`/reference/command/output_format` for ``HEADER``.

See also
--------

* :doc:`/reference/tables`
* :doc:`/reference/commands/column_create`
* :doc:`/reference/tokenizers`
* :doc:`/reference/normalizers`
* :doc:`/reference/command/output_format`
