.. -*- rst -*-

.. highlightlang:: none

.. groonga-command
.. database: commands_column_create

``column_create``
=================

Summary
-------

``column_create`` creates a new column in a table.

You need to create one or more columns to store multiple data in one
record.

Groonga provides an index as a column. It's different from other
systems. An index is just an index in other systems. Implementing an
index as a column provides flexibility. For example, you can add
metadata to each token.

See :doc:`/reference/column` for column details.

Syntax
------

This command takes many parameters::

  column_create table
                name
                flags
                type
                [source=null]

Usage
-----

This section describes about the followings:

  * :ref:`column-create-scalar`
  * :ref:`column-create-vector`
  * :ref:`column-create-reference`
  * :ref:`column-create-index`
  * :ref:`column-create-index-full-text-search`
  * :ref:`column-create-index-multiple-columns`
  * :ref:`column-create-index-small`
  * :ref:`column-create-index-medium`

Here is the ``People`` table definition. The ``People`` table is used
in examples:

.. groonga-command
.. include:: ../../example/reference/commands/column_create/usage_table.log
.. table_create \
..   --name People \
..   --flags TABLE_HASH_KEY \
..   --key_type ShortText

.. _column-create-scalar:

Create a scalar column
^^^^^^^^^^^^^^^^^^^^^^

Groonga provides scalar column to store one value. For example, scalar
column should be used for storing age into a person record. Because a
person record must have only one age.

If you want to store multiple values into a record, scalar column
isn't suitable. Use :ref:`column-create-vector` instead.

You must specify ``COLUMN_SCALAR`` to the ``flags`` parameter to
create a scalar column.

Here is an example to create the ``age`` column to the ``People``
table. ``age`` column is a scalar column. It can store one ``UInt8``
(``0-255``) value as its value:

.. groonga-command
.. include:: ../../example/reference/commands/column_create/usage_scalar_create.log
.. column_create \
..   --table People \
..   --name age \
..   --flags COLUMN_SCALAR \
..   --type UInt8

You can store one value (``7``) by the following :doc:`load` command:

.. groonga-command
.. include:: ../../example/reference/commands/column_create/usage_scalar_load.log
.. load --table People
.. [
.. {"_key": "alice", "age": 7}
.. ]

You can confirm the stored one value (``7``) by the following
:doc:`select` command:

.. groonga-command
.. include:: ../../example/reference/commands/column_create/usage_scalar_select.log
.. select --table People

.. _column-create-vector:

Create a vector column
^^^^^^^^^^^^^^^^^^^^^^

Groonga provides vector column to store multiple values. For example,
vector column may be used for storing roles into a person
record. Because a person record may have multiple roles.

If you want to store only one value into a record, vector column isn't
suitable. Use :ref:`column-create-scalar` instead.

You must specify ``COLUMN_VECTOR`` to the ``flags`` parameter to
create a vector column.

Here is an example to create the ``roles`` column to the ``People``
table. ``roles`` column is a vector column. It can store zero or more
``ShortText`` values as its value:

.. groonga-command
.. include:: ../../example/reference/commands/column_create/usage_vector_create.log
.. column_create \
..   --table People \
..   --name roles \
..   --flags COLUMN_VECTOR \
..   --type ShortText

You can store multiple values (``["adventurer", "younger-sister"]``)
by the following :doc:`load` command:

.. groonga-command
.. include:: ../../example/reference/commands/column_create/usage_scalar_load.log
.. load --table People
.. [
.. {"_key": "alice", "roles": ["adventurer", "younger-sister"]}
.. ]

You can confirm the stored multiple values (``["adventurer",
"younger-sister"]``) by the following :doc:`select` command:

.. groonga-command
.. include:: ../../example/reference/commands/column_create/usage_vector_select.log
.. select --table People

.. _column-create-reference:

Create a column that refers a table's record
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Both scalar column and vector column can store reference to record of
an existing table as column value. It's useful to store relationship
between records.

For example, using a column that refers is better for storing a
character into a book record. Because one person may be appeared in
some books.

You must specify table name to ``type`` parameter to create a column
that refers a table's record.

Here is an example to create the ``character`` column to the ``Books``
table. ``character`` column refers ``People`` table. It can store
one ``People`` table's record.

Here is the ``Books`` table definition:

.. groonga-command
.. include:: ../../example/reference/commands/column_create/usage_reference_create_table.log
.. table_create \
..   --name Books \
..   --flags TABLE_HASH_KEY \
..   --key_type ShortText

Here is the ``character`` column definition in the ``Books``
table. ``--type People`` is important:

.. groonga-command
.. include:: ../../example/reference/commands/column_create/usage_reference_create_column.log
.. column_create \
..   --table Books \
..   --name character \
..   --flags COLUMN_SCALAR \
..   --type People

You can store one reference (``"alice"``) by the following :doc:`load`
command. You can use key value (``People._key`` value) for referring a
record:

.. groonga-command
.. include:: ../../example/reference/commands/column_create/usage_reference_load.log
.. load --table Books
.. [
.. {"_key": "Alice's Adventure in Wonderland", "character": "alice"}
.. ]

You can confirm the stored reference (``"alice"`` record) by the
following :doc:`select` command. It retrieves ``age`` column and
``roles`` column values:

.. groonga-command
.. include:: ../../example/reference/commands/column_create/usage_reference_select.log
.. select \
..  --table Books \
..  --output_columns _key,character._key,character.age,character.roles

.. _column-create-index:

Create an index column
^^^^^^^^^^^^^^^^^^^^^^

Groonga provides index column for fast search. It doesn't store your
data. It stores data for fast search.

You don't need to update index column by yourself. Index column is
updated automatically when you store data into a data column (scalar
column or vector column) that is marked as index target column. You
can set multiple columns as index target columns to one index column.

If Groonga has an index for the ``age`` column of the ``People``
table, Groonga can do fast equal search, fast comparison search and
fast range search against ``age`` column values.

You must specify the following parameters to create an index column:

  * The ``flags`` parameter: ``COLUMN_INDEX``

  * The ``type`` parameter: The table name of source column such as
    ``People``

  * The ``source`` parameter: The source column name such as ``age``

You don't need additional flags to the ``flags`` parameter for equal
search, comparison search and range search index. You need additional
flags to the ``flags`` parameter for full text search index or
multiple column index. See :ref:`column-create-index-full-text-search`
and :ref:`column-create-index-multiple-columns` for details.

Here is an example to create an index for the ``age`` column of the
``People`` table.

First, you need to create a range index table. See
:ref:`table-create-range-index-table` for details. This example
creates the ``Ages`` table as :ref:`table-pat-key`:

.. groonga-command
.. include:: ../../example/reference/commands/column_create/usage_index_create_table.log
.. table_create \
..   --name Ages \
..   --flags TABLE_PAT_KEY \
..   --key_type UInt8

Now, you can create an index column for the ``age`` column of the
``People`` table. ``COLUMN_INDEX`` in the ``flags`` parameter,
``People`` in the ``type`` parameter and ``age`` in the ``source``
parameter are important:

.. groonga-command
.. include:: ../../example/reference/commands/column_create/usage_index_create_column.log
.. column_create \
..   --table Ages \
..   --name people_age_index \
..   --flags COLUMN_INDEX \
..   --type People \
..   --source age

You can confirm that ``age > 5`` is evaluated by the
``Ages.people_age_index`` newly created index from log. Groonga
reports used indexes in ``info`` log level. You can change log level
dynamically by :doc:`log_level` command.

.. groonga-command
.. log: true
.. include:: ../../example/reference/commands/column_create/usage_index_select.log
.. log_level --level info
.. select \
..   --table People \
..   --filter 'age > 5'
.. log_level --level notice
.. log: false

You can confirm that the ``Ages.people_age_index`` is used from the
following log::

  [table][select][index][range] <Ages.people_age_index>

The log says ``Ages.people_age_index`` index is used for range search.

.. _column-create-index-full-text-search:

Create an index column for full text search
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

There is a difference between for non full text search (equal search,
comparison search or range search) index and for full text search
index. You need to add ``WITH_POSITION`` to the ``flags``
parameter. It means that you need to specify
``COLUMN_INDEX|WITH_POSITION`` to the ``flags`` parameter. It's the
difference.

Here is an example to create a full text search index for the
``roles`` column of the ``People`` table.

First, you need to create a table for full text search index. See
:ref:`table-create-lexicon` for details. This example creates the
``Terms`` table as :ref:`table-pat-key` with :ref:`token-bigram`
tokenizer and :ref:`normalizer-auto` normalizer:

.. groonga-command
.. include:: ../../example/reference/commands/column_create/usage_full_text_search_index_create_table.log
.. table_create \
..   --name Terms \
..   --flags TABLE_PAT_KEY \
..   --key_type ShortText \
..   --default_tokenizer TokenBigram \
..   --normalizer NormalizerAuto

Now, you can create a full text search index column for the ``roles``
column of the ``People`` table. ``COLUMN_INDEX|WITH_POSITION`` in the
``flags`` parameter, ``People`` in the ``type`` parameter and
``roles`` in the ``source`` parameter are important:

.. groonga-command
.. include:: ../../example/reference/commands/column_create/usage_full_text_search_index_create_column.log
.. column_create \
..   --table Terms \
..   --name people_roles_index \
..   --flags COLUMN_INDEX|WITH_POSITION \
..   --type People \
..   --source roles

You can confirm that ``--match_columns roles`` and ``--query Sister``
are evaluated by the ``Terms.people_roles_index`` newly created full
text search index from log. Groonga reports used indexes in ``info``
log level. You can change log level dynamically by :doc:`log_level`
command.

.. groonga-command
.. log: true
.. include:: ../../example/reference/commands/column_create/usage_full_text_search_index_select.log
.. log_level --level info
.. select \
..   --table People \
..   --match_columns roles \
..   --query Sister
.. log_level --level notice
.. log: false

You can confirm that the ``Terms.people_roles_index`` is used from the
following log::

  [object][search][index][key][exact] <Terms.people_roles_index>

The log says ``Terms.people_roles_index`` index is used for full text
search. (To be precise, the index is used for exact term search by
inverted index.)

.. _column-create-index-multiple-columns:

Create a multiple columns index column
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You can create an index column for multiple columns. It means that you
can do fast search for multiple columns with one index
column. Multiple columns index has better space efficiency than single
column index only when multiple columns have many same
tokens. Multiple columns index may be slower than single column
index. Because multiple columns index will be a bigger index.

You can't use multiples columns in different tables as source columns
in the same multiple columns index. You must specify columns in the
same tables as source columns to one multiple columns index. For
example, you can't create a multiple columns index for ``People._key``
and ``Books._key`` because they are columns of different tables. You
can create a multiple columns index for ``People._key`` and
``People.roles`` because they are columns of the same table.

There is a difference between for single column index and for multiple
columns index. You need to add ``WITH_SECTION`` to the ``flags``
parameter. It means that you need to specify
``COLUMN_INDEX|WITH_SECTION`` to the ``flags`` parameter. It's the
difference.

If you want to create a multiple columns index for full text search,
you need to specify ``COLUMN_INDEX|WITH_POSITION|WITH_SECTION`` to
the ``flags`` parameter. See :ref:`column-create-full-text-search` for
full text search index column details.

Here is an example to create a multiple columns full text search index
for the key of the ``People`` table and the ``roles`` column of the
``People`` table.

There is no difference between index table for single column index and
multiple columns index. In this example, the ``Terms`` table created
at :ref:`column-create-full-text-search` is used.

You can create a multiple columns full text search index column for
the key of the ``People`` table and ``roles`` column of the ``People``
table. ``COLUMN_INDEX|WITH_POSITION|WITH_SECTION`` in the ``flags``
parameter, ``People`` in the ``type`` parameter and ``_key,roles`` in
the ``source`` parameter are important:

.. groonga-command
.. include:: ../../example/reference/commands/column_create/usage_multiple_columns_index_create_column.log
.. column_create \
..   --table Terms \
..   --name people_key_roles_index \
..   --flags COLUMN_INDEX|WITH_POSITION|WITH_SECTION \
..   --type People \
..   --source _key,roles

You can confirm that ``--match_columns _key`` and ``--query
Alice`` are evaluated by the ``Terms.people_key_roles_index`` newly
created multiple columns full text search index from log. Groonga
reports used indexes in ``info`` log level. You can change log level
dynamically by :doc:`log_level` command.

.. groonga-command
.. log: true
.. include:: ../../example/reference/commands/column_create/usage_multiple_columns_index_select.log
.. log_level --level info
.. select \
..   --table People \
..   --match_columns _key,roles \
..   --query Alice
.. log_level --level notice
.. log: false

You can confirm that the ``Terms.people_roles_index`` is used from the
following log::

  [object][search][index][key][exact] <Terms.people_key_roles_index>

The log says ``Terms.people_roles_index`` index is used for full text
search. (To be precise, the index is used for exact term search by
inverted index.)

.. _column-create-index-small:

Create a small index column
^^^^^^^^^^^^^^^^^^^^^^^^^^^

If you know index target data are small, you can reduce memory usage
for the index column. Memory usage is ``1/256`` of the default index
column.

How many data are small? It's depend on data. Small index column can't
handle 10 billions records at least. If index target is only one
scalar column with no text family type (``ShortText``, ``Text`` or
``LongText``), the maximum handleable records are depends of the
number of kinds of index target data. If index target column has
``1``, ``1``, ``22`` and ``3``, the number of kinds of them are ``3``
(``1`` and ``2`` and ``3``). The following table shows the
relationship between the number of kinds of index target data and the
number of handleable records:

.. list-table:: The number of kinds of index target data and the number of handleable records in a small index column
   :header-rows: 1

   * - The number of kinds of index target data
     - The number of hanleable records
   * - 1
     - 16779234
   * - 2
     - 4648070
   * - 4
     - 7238996
   * - 8
     - 8308622
   * - 16
     - 11068624
   * - 32
     - 12670817
   * - 64
     - 18524211
   * - 128
     - 38095511
   * - 256
     - 51265384

You need to add ``INDEX_SMALL`` to the ``flags`` parameter such s
``COLUMN_INDEX|INDEX_SMALL``.

If the ``People`` table has only 1 million records, you can use a
small index column for the ``age`` column:

.. groonga-command
.. include:: ../../example/reference/commands/column_create/usage_small_index_create_column.log
.. column_create \
..   --table Ages \
..   --name people_age_small_index \
..   --flags COLUMN_INDEX|INDEX_SMALL \
..   --type People \
..   --source age

.. _column-create-index-medium:

Create a medium index column
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

If you know index target data are medium, you can reduce memory usage
for the index column. Memory usage is ``5/24`` of the default index
column.

How many data are medium? It's depend on data.

If index target is only one scalar column, a medium index column
can handle all records.

A medium index column may not handle all records at the following
cases:

  * Index target is one text family (``ShortText``, ``Text`` or
    ``LongText``) scalar column
  * Index target is one vector column
  * Index targets are multiple columns
  * Index table has tokenizer

You need to add ``INDEX_MEDIUM`` to the ``flags`` parameter such s
``COLUMN_INDEX|INDEX_MEDIUM``.

You can use a medium index column for an index column of the ``age``
column of the ``People`` table safely. Because it's one scalar column
with ``UInt8`` type.

Here is an example to create a medium index column:

.. groonga-command
.. include:: ../../example/reference/commands/column_create/usage_medium_index_create_column.log
.. column_create \
..   --table Ages \
..   --name people_age_medium_index \
..   --flags COLUMN_INDEX|INDEX_MEDIUM \
..   --type People \
..   --source age

Parameters
----------

This section describes all parameters.

TODO

``table``

  カラムを追加するテーブルの名前を指定します。

``name``

  作成するカラムの名前を指定します。カラム名は、テーブルの中で一意でなければなりません。

  ピリオド('.'), コロン(':')を含む名前のカラムは作成できません。また、アンダースコア('_')で始まる名前は予約済みであり、使用できません。

``flags``

  カラムの属性を表す以下の数値か、パイプ('|')で組み合わせたシンボル名を指定します。

  0, ``COLUMN_SCALAR``
    単一の値が格納できるカラムを作成します。
  1, ``COLUMN_VECTOR``
    複数の値の配列を格納できるカラムを作成します。
  2, ``COLUMN_INDEX``
    インデックス型のカラムを作成します。

  There are two flags to compress the value of column, but you can't specify these flags for now because there are memory leaks issue `GitHub#6 <https://github.com/groonga/groonga/issues/6>`_ when refers the value of column. This issue occurs both of them (zlib and lzo).

  16, ``COMPRESS_ZLIB``
    Compress the value of column by using zlib. This flag is enabled when you build Groonga with ``--with-zlib``.
  32, ``COMPRESS_LZO``
    Compress the value of column by using lzo. This flag is enabled when you build Groonga with ``--with-lzo``.

  インデックス型のカラムについては、flagsの値に以下の値を加えることによって、追加の属
  性を指定することができます。

  128, ``WITH_SECTION``
    段落情報を格納するインデックスを作成します。

  256, ``WITH_WEIGHT``
    ウェイト情報を格納するインデックスを作成します。

  512, ``WITH_POSITION``
    位置情報を格納するインデックス(完全転置インデックス)を作成します。

``type``

  値の型を指定します。Groongaの組込型か、同一データベースに定義済みのユーザ定義型、定義済みのテーブルを指定することができます。

``source``

  インデックス型のカラムを作成した場合は、インデックス対象となるカラムをsource引数に指定します。

Return value
------------

::

 [HEADER, SUCCEEDED]

``HEADER``

  See :doc:`/reference/command/output_format` about ``HEADER``.

``SUCCEEDED``

  If command is succeeded, it returns true on success, false otherwise.
