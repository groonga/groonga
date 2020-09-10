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

This command takes many parameters.

Most parameters are required::

  column_create table
                name
                flags
                type
                [source=null]
		[path=null]

Usage
-----

This section describes about the followings:

  * :ref:`column-create-scalar`
  * :ref:`column-create-vector`
  * :ref:`column-create-vector-weight`
  * :ref:`column-create-reference`
  * :ref:`column-create-index`
  * :ref:`column-create-index-full-text-search`
  * :ref:`column-create-index-multiple-columns`
  * :ref:`column-create-index-small`
  * :ref:`column-create-index-medium`
  * :ref:`column-create-index-large`

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
.. include:: ../../example/reference/commands/column_create/usage_vector_load.log
.. load --table People
.. [
.. {"_key": "alice", "roles": ["adventurer", "younger-sister"]}
.. ]

You can confirm the stored multiple values (``["adventurer",
"younger-sister"]``) by the following :doc:`select` command:

.. groonga-command
.. include:: ../../example/reference/commands/column_create/usage_vector_select.log
.. select --table People

.. _column-create-vector-weight:

Create a weight vector column
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

TODO: See also :ref:`weight-vector-column` and :ref:`select-adjuster`.

.. _column-create-reference:

Create a column that refers a table's record
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Both scalar column and vector column can store reference to record of
an existing table as column value. It's useful to store relationship
between records.

For example, using a column that refers a person record is better for
storing a character into a book record. Because one person may be
appeared in some books.

You must specify table name to be referenced to the ``type`` parameter
to create a column that refers a table's record.

Here is an example to create the ``character`` column to the ``Books``
table. The ``character`` column refers the ``People`` table. It can
store one ``People`` table's record.

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
following :doc:`select` command. It retrieves the ``age`` column and
the ``roles`` column values:

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

If you make a new index, it is invisible until finishing of index build.

If Groonga has an index column for the ``age`` column of the
``People`` table, Groonga can do fast equal search, fast comparison
search and fast range search against ``age`` column values.

You must specify the following parameters to create an index column:

  * The ``flags`` parameter: ``COLUMN_INDEX``

  * The ``type`` parameter: The table name of index target column such
    as ``People``

  * The ``source`` parameter: The index target column name such as
    ``age``

You don't need additional flags to the ``flags`` parameter for equal
search, comparison search and range search index column. You need
additional flags to the ``flags`` parameter for full text search index
column or multiple column index column. See
:ref:`column-create-index-full-text-search` and
:ref:`column-create-index-multiple-columns` for details.

Here is an example to create an index column for the ``age`` column of
the ``People`` table.

First, you need to create a table for range index column. See
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
``Ages.people_age_index`` newly created index column from log. Groonga
reports used index columns in ``info`` log level. You can change log
level dynamically by :doc:`log_level` command.

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

The log says ``Ages.people_age_index`` index column is used for range
search.

.. _column-create-index-full-text-search:

Create an index column for full text search
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

There is a difference between for non full text search (equal search,
comparison search or range search) index column and for full text
search index column. You need to add ``WITH_POSITION`` to the
``flags`` parameter. It means that you need to specify
``COLUMN_INDEX|WITH_POSITION`` to the ``flags`` parameter. It's the
difference.

Here is an example to create a full text search index column for the
key of the ``People`` table.

First, you need to create a table for full text search index
column. See :ref:`table-create-lexicon` for details. This example
creates the ``Terms`` table as :ref:`table-pat-key` with
:ref:`token-bigram` tokenizer and :ref:`normalizer-auto` normalizer:

.. groonga-command
.. include:: ../../example/reference/commands/column_create/usage_full_text_search_index_create_table.log
.. table_create \
..   --name Terms \
..   --flags TABLE_PAT_KEY \
..   --key_type ShortText \
..   --default_tokenizer TokenBigram \
..   --normalizer NormalizerAuto

Now, you can create a full text search index column for the key of the
``People`` table. ``COLUMN_INDEX|WITH_POSITION`` in the ``flags``
parameter, ``People`` in the ``type`` parameter and ``_key`` in the
``source`` parameter are important:

.. groonga-command
.. include:: ../../example/reference/commands/column_create/usage_full_text_search_index_create_column.log
.. column_create \
..   --table Terms \
..   --name people_key_index \
..   --flags COLUMN_INDEX|WITH_POSITION \
..   --type People \
..   --source _key

You can confirm that ``--match_columns _key`` and ``--query Alice``
are evaluated by the ``Terms.people_key_index`` newly created full
text search index column from log. Groonga reports used index columns
in ``info`` log level. You can change log level dynamically by
:doc:`log_level` command.

.. groonga-command
.. log: true
.. include:: ../../example/reference/commands/column_create/usage_full_text_search_index_select.log
.. log_level --level info
.. select \
..   --table People \
..   --match_columns _key \
..   --query Alice
.. log_level --level notice
.. log: false

You can confirm that the ``Terms.people_key_index`` is used from the
following log::

  [object][search][index][key][exact] <Terms.people_key_index>

The log says ``Terms.people_key_index`` index column is used for full
text search. (To be precise, the index column is used for exact term
search by inverted index.)

.. _column-create-index-multiple-columns:

Create a multiple columns index column
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You can create an index column for multiple columns. It means that you
can do fast search for multiple columns with one index
column. Multiple columns index column has better space efficiency than
single column index column only when multiple columns have many same
tokens. Multiple columns index column may be slower than single column
index column. Because multiple columns index column will be a bigger
index column.

You can't use multiples columns in different tables as index target
columns in the same multiple columns index column. You must specify
columns in the same tables as index target columns to one multiple
columns index column. For example, you can't create a multiple columns
index for ``People._key`` and ``Books._key`` because they are columns
of different tables. You can create a multiple columns index column
for ``People._key`` and ``People.roles`` because they are columns of
the same table.

There is a difference between for single column index column and for
multiple columns index column. You need to add ``WITH_SECTION`` to the
``flags`` parameter. It means that you need to specify
``COLUMN_INDEX|WITH_SECTION`` to the ``flags`` parameter. It's the
difference.

If you want to create a multiple columns index column for full text
search, you need to specify
``COLUMN_INDEX|WITH_POSITION|WITH_SECTION`` to the ``flags``
parameter. See :ref:`column-create-index-full-text-search` for full
text search index column details.

Here is an example to create a multiple columns index column for the
key of the ``People`` table and the ``roles`` column of the ``People``
table.

There is no difference between index table for single column index
column and multiple columns index column.

In this example, ``Names`` table is created for equal search and
prefix search. It uses ``TABLE_PAT_KEY`` because ``TABLE_PAT_KEY``
supports prefix search. See :doc:`../tables` for details.

.. groonga-command
.. include:: ../../example/reference/commands/column_create/usage_multiple_columns_index_create_table.log
.. table_create \
..   --name Names \
..   --flags TABLE_PAT_KEY \
..   --key_type ShortText \
..   --normalizer NormalizerAuto

You can create a multiple columns index column for the key of the
``People`` table and ``roles`` column of the ``People``
table. ``COLUMN_INDEX|WITH_SECTION`` in the ``flags`` parameter,
``People`` in the ``type`` parameter and ``_key,roles`` in the
``source`` parameter are important:

.. groonga-command
.. include:: ../../example/reference/commands/column_create/usage_multiple_columns_index_create_column.log
.. column_create \
..   --table Names \
..   --name people_key_roles_index \
..   --flags COLUMN_INDEX|WITH_SECTION \
..   --type People \
..   --source _key,roles

You can confirm that ``--filter 'roles @^ "Younger"`` is evaluated by
the ``Names.people_key_roles_index`` newly created multiple columns
index column from log. Groonga reports used index columns in ``info``
log level. You can change log level dynamically by :doc:`log_level`
command.

.. groonga-command
.. log: true
.. include:: ../../example/reference/commands/column_create/usage_multiple_columns_index_select.log
.. log_level --level info
.. select \
..   --table People \
..   --filter 'roles @^ "Younger"'
.. log_level --level notice
.. log: false

You can confirm that the ``Names.people_key_roles_index`` is used from
the following log::

  [table][select][index][prefix] <Names.people_key_roles_index>

The log says ``Names.people_key_roles_index`` index column is used for
prefix search.

.. _column-create-index-small:

Create a small index column
^^^^^^^^^^^^^^^^^^^^^^^^^^^

If you know index target data are small, you can reduce memory usage
for the index column. Memory usage is ``1/256`` of the default index
column.

How many data are small? It depends on data. Small index column can't
handle 1 billion records at least. If index target is only one
scalar column with no text family type (``ShortText``, ``Text`` or
``LongText``), the maximum handleable records are depends of the
number of kinds of index target data. If index target column has
``1``, ``1``, ``2`` and ``3``, the number of kinds of them are ``3``
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

You need to add ``INDEX_SMALL`` to the ``flags`` parameter such as
``COLUMN_INDEX|INDEX_SMALL`` to create a small index column.

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

How many data are medium? It depends on data.

If index target is only one scalar column, a medium index column
can handle all records.

A medium index column may not handle all records at the following
cases:

  * Index target is one text family (``ShortText``, ``Text`` or
    ``LongText``) scalar column
  * Index target is one vector column
  * Index targets are multiple columns
  * Index table has tokenizer

You need to add ``INDEX_MEDIUM`` to the ``flags`` parameter such as
``COLUMN_INDEX|INDEX_MEDIUM`` to create a medium index column.

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

.. _column-create-index-large:

Create a large index column
^^^^^^^^^^^^^^^^^^^^^^^^^^^

If you know index target data are large, you need to use large index
column. It uses increases memory usage for the index column but it can
accept more data. Memory usage is 2 times larger than the default
index column.

How many data are large? It depends on data.

If index target is only one scalar column, it's not large data.

Large data must have many records (normally at least 10 millions
records) and at least one of the following features:

  * Index targets are multiple columns
  * Index table has tokenizer

You need to add ``INDEX_LARGE`` to the ``flags`` parameter such as
``COLUMN_INDEX|INDEX_LARGE`` to create a large index column.

You can use a large index column for an index column of the ``_key``
of the ``People`` table and the ``role`` column of the ``People``
table.

Here is an example to create a large index column:

.. groonga-command
.. include:: ../../example/reference/commands/column_create/usage_large_index_create_column.log
.. column_create \
..   --table Terms \
..   --name people_roles_large_index \
..   --flags COLUMN_INDEX|WITH_POSITION|WITH_SECTION|INDEX_LARGE \
..   --type People \
..   --source roles

Parameters
----------

This section describes all parameters.

Required parameters
^^^^^^^^^^^^^^^^^^^

There are some required parameters.

.. _column-create-table:

``table``
"""""""""

Specifies an existing table name for the new column.

.. _column-create-name:

``name``
""""""""

Specifies the column name to be created.

The column name must be unique in the same table.

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


.. _column-create-flags:

``flags``
"""""""""

Specifies the column type and column customize options.

Here are available flags:

.. list-table::
   :header-rows: 1

   * - Flag
     - Description

   * - ``COLUMN_SCALAR``
     - Scalar column. It can store one value. See also
       :doc:`/reference/columns/scalar` or
       :ref:`column-create-scalar`.

   * - ``COLUMN_VECTOR``
     - Vector column. It can store multiple values. See also
       :doc:`/reference/columns/vector` or
       :ref:`column-create-vector`.

   * - ``COLUMN_INDEX``
     - Index column. It stores data for fast search. See also
       :doc:`/reference/columns/index` or
       :ref:`column-create-index`.

   * - ``COMPRESS_ZLIB``
     - It enables column value compression by zlib. You need Groonga
       that enables zlib support.

       Compression by zlib is higher space efficiency than compression
       by LZ4. But compression by zlib is slower than compression by
       LZ4.

       This flag is available only for ``COLUMN_SCALAR`` and
       ``COLUMN_VECTOR``.

   * - ``COMPRESS_LZ4``
     - It enables column value compression by LZ4. You need Groonga
       that enables LZ4 support.

       Compression by LZ4 is faster than compression by zlib. But
       compression by LZ4 is lower space efficiency than compression
       by zlib.

       This flag is available only for ``COLUMN_SCALAR`` and
       ``COLUMN_VECTOR``.

   * - ``COMPRESS_ZSTD``
     - It enables column value compression by Zstandard. You need
       Groonga that enables Zstandard support.

       Compression by Zstandard is faster than compression by zlib and
       the same space efficiency as zlib.

       This flag is available only for ``COLUMN_SCALAR`` and
       ``COLUMN_VECTOR``.

   * - ``WITH_SECTION``
     - It enables section support to index column.

       If section support is enabled, you can support multiple
       documents in the same index column.

       You must specify this flag to create a multiple columns index
       column. See also :ref:`column-create-index-multiple-columns` for
       details.

       Section support requires additional spaces. If you don't need
       section support, you should not enable section support.

       This flag is available only for ``COLUMN_INDEX``.

   * - ``WITH_WEIGHT``
     - It enables weight support to vector column or index column.

       If weight support is enabled for vector column, you can add
       weight for each element. If weight support is enabled for index
       column, you can add weight for each posting. They are useful to
       compute suitable search score.

       You must specify this flag to use :ref:`select-adjuster`. See
       also :ref:`column-create-vector-weight` for details.

       Weight support requires additional spaces. If you don't need
       weight support, you should not enable weight support.

       This flag is available only for ``COLUMN_VECTOR`` or
       ``COLUMN_INDEX``.

   * - ``WITH_POSITION``
     - It enables position support to index column. It means that the
       index column is full inverted index. (Index column is
       implemented as inverted index.)

       If position support is enabled, you can add position in the
       document for each posting. It's required for phrase search. It
       means that index column for full text search must enable
       position support because most full text search uses phrase
       search.

       You must specify this flag to create a full text search index
       column. See also :ref:`column-create-index-full-text-search` for
       details.

       Position support requires additional spaces. If you don't need
       position support, you should not enable position support.

       This flag is available only for ``COLUMN_INDEX``.

   * - ``INDEX_SMALL``
     - .. versionadded:: 6.0.8

       It requires to create a small index column.

       If index target data are small, small index column is enough.
       Small index column uses fewer memory than a normal index column
       or a medium index column. See also
       :ref:`column-create-index-small` for knowing what are "small
       data" and how to use this flag.

       This flag is available only for ``COLUMN_INDEX``.

   * - ``INDEX_MEDIUM``
     - .. versionadded:: 6.0.8

       It requires to create a medium index column.

       If index target data are medium, medium index column is enough.
       Medium index column uses fewer memory than a normal index
       column. See also :ref:`column-create-index-medium` for knowing
       what are "medium data" and how to use this flag.

       This flag is available only for ``COLUMN_INDEX``.

   * - ``INDEX_LARGE``
     - .. versionadded:: 9.0.2

       It requires to create a large index column.

       If index target data are large, you need to use large index
       column. Large index column uses more memory than a normal index
       column but accepts more data than a normal index column. See
       also :ref:`column-create-index-large` for knowing what are
       "large data" and how to use this flag.

       This flag is available only for ``COLUMN_INDEX``.

You must specify one of ``COLUMN_${TYPE}`` flags. You can't specify
two or more ``COLUMN_${TYPE}`` flags. For example,
``COLUMN_SCALAR|COLUMN_VECTOR`` is invalid.

You can combine flags with ``|`` (vertical bar) such as
``COLUMN_INDEX|WITH_SECTION|WITH_POSITION``.

.. _column-create-type:

``type``
""""""""

Specifies type of the column value.

If the column is scalar column or vector column, here are available
types:

  * Builtin types described in :doc:`/reference/types`
  * Tables defined by users

If the column is index column, here are available types:

  * Tables defined by users

See also the followings:

  * :ref:`column-create-scalar`
  * :ref:`column-create-vector`
  * :ref:`column-create-reference`
  * :ref:`column-create-index`

Optional parameters
^^^^^^^^^^^^^^^^^^^

There is an optional parameter.

.. _column-create-source:

``source``
""""""""""

Specifies index target columns. You can specify one or more columns to
the ``source`` parameter.

This parameter is only available for index column.

You can only specify columns of the table specified as
:ref:`column-create-type`. You can also use the ``_key`` pseudo column
for specifying the table key as index target.

If you specify multiple columns to the ``source`` parameter, separate
columns with ``,`` (comma) such as ``_key,roles``.

``path``
""""""""

.. versionadded:: 10.0.7

Specifies a path for storing a column.

This option is useful if you want to store a column that
you often use to fast storage (e.g. SSD) and store it that you don't often
use to slow storage (e.g. HDD).

You can use a relative path or an absolute path in this option.
If you specify a relative path, it is resolved from the current directory for the ``groonga`` process.

The default value is none.

Return value
------------

``column_create`` returns ``true`` as body on success such as::

  [HEADER, true]

If ``column_create`` fails, ``column_create`` returns ``false`` as
body::

  [HEADER, false]

Error details are in ``HEADER``.

See :doc:`/reference/command/output_format` for ``HEADER``.

See also
--------

  * :doc:`/reference/column`
  * :doc:`/reference/commands/table_create`
  * :doc:`/reference/command/output_format`
