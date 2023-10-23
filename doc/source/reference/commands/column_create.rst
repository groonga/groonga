.. -*- rst -*-

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
  * :ref:`column-create-missing-mode`
  * :ref:`column-create-invalid-mode`

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

.. _column-create-missing-mode:

Missing mode
^^^^^^^^^^^^

.. versionadded:: 12.0.2

You can control how to process a nonexistent key in the specified new
value of a reference column by a ``MISSING_*`` flag. Here are
available ``MISSING_*`` flags:

* ``MISSING_ADD`` (default)
* ``MISSING_IGNORE``
* ``MISSING_NIL``

You can't specify multiple ``MISSING_*`` flags for a column.

``MISSING_*`` flags are meaningful only for a reference column.

The following table describes the differences between ``MISSING_*``
flags when a nonexistent key is specified to a reference scalar
column:

.. list-table::
   :header-rows: 1

   * - Flag
     - Description
     - An example given value
     - An example set value

   * - ``MISSING_ADD``
     - The given nonexistent key is added to the referred table
       automatically and the ID of the newly added record is set.

       This is the default.
     - ``"nonexistent"``
     - The record ID of the newly added record whose key is
       ``"nonexistent"``.
   * - ``MISSING_IGNORE``
     - The given nonexistent key is ignored and ``0`` is set.

       There is no difference between ``MISSING_IGNORE`` and
       ``MISSING_NIL`` for a reference scalar column.
     - ``"nonexistent"``
     - ``0``
   * - ``MISSING_NIL``
     - The given nonexistent key is ignored and ``0`` is set.

       There is no difference between ``MISSING_IGNORE`` and
       ``MISSING_NIL`` for a reference scalar column.
     - ``"nonexistent"``
     - ``0``

Here is an example to show differences between ``MISSING_*`` flags for
a reference scalar column.

First this example defines columns for all ``MISSING_*`` flags:

.. groonga-command
.. include:: ../../example/reference/commands/column_create/missing_mode_scalar_schema.log
.. table_create \
..   --name MissingModeScalarReferred \
..   --flags TABLE_HASH_KEY \
..   --key_type ShortText
.. table_create \
..   --name MissingModeScalar \
..   --flags TABLE_HASH_KEY \
..   --key_type ShortText
.. column_create \
..   --table MissingModeScalar \
..   --name missing_add \
..   --flags COLUMN_SCALAR|MISSING_ADD \
..   --type MissingModeScalarReferred
.. column_create \
..   --table MissingModeScalar \
..   --name missing_ignore \
..   --flags COLUMN_SCALAR|MISSING_IGNORE \
..   --type MissingModeScalarReferred
.. column_create \
..   --table MissingModeScalar \
..   --name missing_nil \
..   --flags COLUMN_SCALAR|MISSING_NIL \
..   --type MissingModeScalarReferred

Then this example loads nonexistent keys to all columns. The specified
nonexistent key for ``MISSING_ADD`` is only added to
``MissingModeScalarReferred`` automatically and the specified
nonexistent keys for ``MISSING_IGNORE`` and ``MISSING_NIL`` aren't
added to ``MissingModeScalarReferred``. ``missing_ignore``'s value and
``missing_nil``'s value are showed as ``""`` because they refer a
record whose ID is ``0`` and record whose ID is ``0`` never exist:

.. groonga-command
.. include:: ../../example/reference/commands/column_create/missing_mode_scalar_load.log
.. load --table MissingModeScalar
.. [
.. {"_key": "key", "missing_add":    "nonexistent1"}
.. ]
.. load --table MissingModeScalar
.. [
.. {"_key": "key", "missing_ignore": "nonexistent2"}
.. ]
.. load --table MissingModeScalar
.. [
.. {"_key": "key", "missing_nil":    "nonexistent3"}
.. ]
.. select --table MissingModeScalar
.. select --table MissingModeScalarReferred

The following table describes the differences between ``MISSING_*``
flags when a vector value that has a nonexistent key element is
specified to a reference vector column:

.. list-table::
   :header-rows: 1

   * - Flag
     - Description
     - An example given value
     - An example set value

   * - ``MISSING_ADD``
     - The given nonexistent key is added to the referred table
       automatically and the ID of the newly added record is used for
       the element.

       This is the default.
     - ``["existent1", "nonexistent", "existent2"]``
     - The record IDs of ``"existent1"``, ``"nonexistent"`` and
       ``"existent2"``.
   * - ``MISSING_IGNORE``
     - The given nonexistent key element is ignored.
     - ``["existent1", "nonexistent", "existent2"]``
     - The record IDs of ``"existent1"`` and ``"existent2"``.
   * - ``MISSING_NIL``
     - The given nonexistent key element is ignored.

       If ``INVALID_WARN`` or ``INVALID_IGNORE`` are also specified,
       the element is replaced with ``0``. If ``INVALID_ERROR`` is
       specified or no ``INVALID_*`` are specified, the element is
       ignored.

       See also :ref:`column-create-invalid-mode`.
     - ``["existent1", "nonexistent", "existent2"]``
     - The record ID of ``"existent1"`` and ``0`` and the record ID of
       ``"existent2"`` for ``INVALID_WARN`` and ``INVALID_IGNORE``.

       The record IDs of ``"existent1"`` and ``"existent2"`` for
       ``INVALID_ERROR`` and no ``INVALID_*``.

Here is an example to show differences between ``MISSING_*`` flags for
a reference vector column.

First this example defines columns for all ``MISSING_*`` flags:

.. groonga-command
.. include:: ../../example/reference/commands/column_create/missing_mode_vector_schema.log
.. table_create \
..   --name MissingModeVectorReferred \
..   --flags TABLE_HASH_KEY \
..   --key_type ShortText
.. table_create \
..   --name MissingModeVector \
..   --flags TABLE_HASH_KEY \
..   --key_type ShortText
.. column_create \
..   --table MissingModeVector \
..   --name missing_add \
..   --flags COLUMN_VECTOR|MISSING_ADD \
..   --type MissingModeVectorReferred
.. column_create \
..   --table MissingModeVector \
..   --name missing_ignore \
..   --flags COLUMN_VECTOR|MISSING_IGNORE|INVALID_IGNORE \
..   --type MissingModeVectorReferred
.. column_create \
..   --table MissingModeVector \
..   --name missing_nil \
..   --flags COLUMN_VECTOR|MISSING_NIL|INVALID_IGNORE \
..   --type MissingModeVectorReferred

Then this example loads a vector that includes a nonexistent key to
all columns. The specified nonexistent key for ``MISSING_ADD`` is only
added to ``MissingModeVectorReferred`` automatically and the specified
nonexistent keys for ``MISSING_IGNORE`` and ``MISSING_NIL`` aren't
added to ``MissingModeVectorReferred``. The specified nonexistent key
element is removed from ``missing_ignore``'s value. The specified
nonexistent key element is replaced with ``0`` in ``missing_nil``'s
value because ``INVALID_IGNORE`` is also specified. And the element
replaced with ``0`` is showed as ``""`` because it refers a record
whose ID is ``0`` and record whose ID is ``0`` never exist:

.. groonga-command
.. include:: ../../example/reference/commands/column_create/missing_mode_vector_load.log
.. load --table MissingModeVectorReferred
.. [
.. {"_key": "existent1"},
.. {"_key": "existent2"}
.. ]
.. load --table MissingModeVector
.. [
.. {"_key": "key", "missing_add":    ["existent1", "nonexistent1", "existent2"]}
.. ]
.. load --table MissingModeVector
.. [
.. {"_key": "key", "missing_ignore": ["existent1", "nonexistent2", "existent2"]}
.. ]
.. load --table MissingModeVector
.. [
.. {"_key": "key", "missing_nil":    ["existent1", "nonexistent3", "existent2"]}
.. ]
.. select --table MissingModeVector
.. select --table MissingModeVectorReferred

.. _column-create-invalid-mode:

Invalid mode
^^^^^^^^^^^^

.. versionadded:: 12.0.2

You can control how to process an invalid value in the specified new
value of a data column by a ``INVALID_*`` flag. Here are available
``INVALID_*`` flags:

* ``INVALID_ERROR`` (default)
* ``INVALID_WARN``
* ``INVALID_IGNORE``

You can't specify multiple ``INVALID_*`` flags for a column.

``INVALID_*`` flags are meaningful only for a ``COLUMN_SCALAR`` column
and a ``COLUMN_VECTOR`` column.

If the target column is a reference column, an invalid value depends
on :ref:`column-create-missing-mode`. If you specify
``MISSING_IGNORE`` or ``MISSING_NIL``, a nonexistent key is an invalid
value. Note that an empty string key and string keys that are empty
strings by normalization aren't an invalid value with all
``MISSING_*`` flags. They are special.

If the target column isn't a reference column, an invalid value
depends on column's value type. For example, ``"invalid"`` is an
invalid value for an ``Int32`` scalar column.

The following table describes the differences between ``INVALID_*``
flags when an invalid value is specified to an ``Int32`` scalar column:

.. list-table::
   :header-rows: 1

   * - Flag
     - Description
     - An example given value
     - An example set value

   * - ``INVALID_ERROR``
     - The given invalid value is reported as an error in
       :ref:`process-log` and by :doc:`load`.

       The given invalid value isn't set.

       This is the default.
     - ``"invalid"``
     - The column isn't updated.
   * - ``INVALID_WARN``
     - The given invalid value is reported as a warning in
       :ref:`process-log`.

       The given invalid value is replaced with the default value of
       the target scalar column. For example, ``0`` is the default
       value for an ``Int32`` scalar column.
     - ``"nonexistent"``
     - ``0``
   * - ``INVALID_IGNORE``
     - The given invalid value is ignored.

       The given invalid value is replaced with the default value of
       the target scalar column. For example, ``0`` is the default
       value for an ``Int32`` scalar column.
     - ``"invalid"``
     - ``0``

Here is an example to show differences between ``INVALID_*`` flags for
an ``Int32`` scalar column.

First this example defines columns for all ``INVALID_*`` flags:

.. groonga-command
.. include:: ../../example/reference/commands/column_create/invalid_mode_scalar_schema.log
.. table_create \
..   --name InvalidModeScalar \
..   --flags TABLE_HASH_KEY \
..   --key_type ShortText
.. column_create \
..   --table InvalidModeScalar \
..   --name invalid_error \
..   --flags COLUMN_SCALAR|INVALID_ERROR \
..   --type Int32
.. column_create \
..   --table InvalidModeScalar \
..   --name invalid_warn \
..   --flags COLUMN_SCALAR|INVALID_WARN \
..   --type Int32
.. column_create \
..   --table InvalidModeScalar \
..   --name invalid_ignore \
..   --flags COLUMN_SCALAR|INVALID_IGNORE \
..   --type Int32

Then this example loads ``29`` as initial values for all columns to
show differences between them on update:

.. groonga-command
.. include:: ../../example/reference/commands/column_create/invalid_mode_scalar_load_initial.log
.. load --table InvalidModeScalar
.. [
..   {
..     "_key": "key",
..     "invalid_error":  29,
..     "invalid_warn":   29,
..     "invalid_ignore": 29
..   }
.. ]
.. select \
..   --table InvalidModeScalar \
..   --output_columns invalid_error,invalid_warn,invalid_ignore

Then this example update existing column values with invalid values.
The specified invalid value is reported as an error by :doc:`load`
only with ``INVALID_ERROR``. And the existing value isn't updated only
with ``INVALID_ERROR``. The existing value is updated with ``0`` with
``INVALID_WARN`` and ``INVALID_IGNORE``. You can't see differences
between ``INVALID_WARN`` and ``INVALID_IGNORE`` with this example but
a warning message is logged in :ref:`process-log` only with
``INVALID_WARN``:

.. groonga-command
.. include:: ../../example/reference/commands/column_create/invalid_mode_scalar_load_update.log
.. load --table InvalidModeScalar
.. [
.. {"_key": "key", "invalid_error":  "invalid"},
.. ]
.. load --table InvalidModeScalar
.. [
.. {"_key": "key", "invalid_warn":   "invalid"},
.. ]
.. load --table InvalidModeScalar
.. [
.. {"_key": "key", "invalid_ignore": "invalid"},
.. ]
.. select \
..   --table InvalidModeScalar \
..   --output_columns invalid_error,invalid_warn,invalid_ignore

The following table describes the differences between ``INVALID_*``
flags when a vector value that has an invalid element is specified to
an ``Int32`` vector column:

.. list-table::
   :header-rows: 1

   * - Flag
     - Description
     - An example given value
     - An example set value

   * - ``INVALID_ERROR``
     - The given invalid element is reported as an error in
       :ref:`process-log` but :doc:`load` doesn't report an error.

       If the target column is a reference vector column and
       ``MISSING_NIL`` flag is specified, invalid elements are
       replaced with ``0``. Invalid elements are ignored otherwise.

     - ``[1, "invalid", 3]``
     - ``[1, 3]``
   * - ``INVALID_WARN``
     - The given invalid element is reported as a warning in
       :ref:`process-log`.

       If the target column is a reference vector column and
       ``MISSING_NIL`` flag is specified, invalid elements are
       replaced with ``0``. Invalid elements are ignored otherwise.

     - ``[1, "invalid", 3]``
     - ``[1, 3]``
   * - ``INVALID_IGNORE``
     - The given invalid element is ignored.

       If the target column is a reference vector column and
       ``MISSING_NIL`` flag is specified, invalid elements are
       replaced with ``0``. Invalid elements are ignored otherwise.

     - ``[1, "invalid", 3]``
     - ``[1, 3]``

Here is an example to show differences between ``INVALID_*`` flags for
a reference vector column.

First this example defines columns for all ``INVALID_*`` flags:

.. groonga-command
.. include:: ../../example/reference/commands/column_create/invalid_mode_vector_schema.log
.. table_create \
..   --name InvalidModeVector \
..   --flags TABLE_HASH_KEY \
..   --key_type ShortText
.. column_create \
..   --table InvalidModeVector \
..   --name invalid_error \
..   --flags COLUMN_VECTOR|INVALID_ERROR \
..   --type Int32
.. column_create \
..   --table InvalidModeVector \
..   --name invalid_warn \
..   --flags COLUMN_VECTOR|INVALID_WARN \
..   --type Int32
.. column_create \
..   --table InvalidModeVector \
..   --name invalid_ignore \
..   --flags COLUMN_VECTOR|INVALID_IGNORE \
..   --type Int32

Then this example loads a vector that includes an invalid element to
all columns. The all specified invalid elements are ignored regardless
of ``INVALID_*`` flags. Messages in :ref:`process-log` are different
by ``INVALID_*`` flags. If ``INVALID_ERROR`` is specified, an error
message is logged in :ref:`process-log`. If ``INVALID_WARN`` is
specified, a warning message is logged in
:ref:`process-log`. ``INVALID_IGNORE`` is specified, no message is
logged in :ref:`process-log`:

.. groonga-command
.. include:: ../../example/reference/commands/column_create/invalid_mode_vector_load.log
.. load --table InvalidModeVector
.. [
.. {"_key": "key", "invalid_error":  [1, "invalid", 3]}
.. ]
.. load --table InvalidModeVector
.. [
.. {"_key": "key", "invalid_warn":   [1, "invalid", 3]}
.. ]
.. load --table InvalidModeVector
.. [
.. {"_key": "key", "invalid_ignore": [1, "invalid", 3]}
.. ]
.. select \
..   --table InvalidModeVector \
..   --output_columns invalid_error,invalid_warn,invalid_ignore

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

   * - ``COMPRESS_FILTER_SHUFFLE``
     - .. versionadded:: 13.0.8

       ``COMPRESS_FILTER_SHUFFLE`` は圧縮前にデータをフィルタリングすることで
       ``COMPRESS_ZLIB`` / ``COMPRESS_LZ4`` / ``COMPRESS_ZSTD`` の圧縮率を高めるためのフラグです。

       ただし、データによって効果があることもあれば効果がないこともあります。圧縮率が下がることもありえます。
       また、このフラグを有効にすることで追加の処理が入るのでカラムの保存・参照処理は確実に遅くなります。
       データに合わせて効果があるフィルターだけ有効にすることが重要です。

       なお、 ``COMPRESS_ZLIB`` / ``COMPRESS_LZ4`` / ``COMPRESS_ZSTD`` を指定しない場合は
       `BloscLZ <https://www.blosc.org/pages/blosc-in-depth/#blosc-as-a-meta-compressor>`_ という圧縮アルゴリズムが使われるので、
       このフラグを有効にすることでほとんどの場合は未圧縮の場合よりもサイズが小さくなります。
       しかし、データによってはこのフラグを有効にせずに単に ``COMPRESS_ZLIB`` / ``COMPRESS_LZ4`` / ``COMPRESS_ZSTD`` を指定するだけで
       十分であることもあるため、実際のデータにあわせて適切にフラグを設定するようにしてください。

       ``COMPRESS_FILTER_SHUFFLE`` フラグはBloscサポートが有効になっていないと無視されることに注意してください。
       各種パッケージでは有効になっていますが、自分でビルドするときは明示的に有効にする必要があります。
       自分でビルドする場合は :doc:`/install/others` を参照してください。

       このフラグは ``COLUMN_VECTOR`` でのみ使用できます。 ``COLUMN_SCALAR`` のときはこのフラグは無視されます。

       では、このフラグがどのようなデータに対して有効かを記載します。

       このフラグはNバイト目の要素に着目してデータを並び替えます。
       まず、ベクターカラム内の各要素の0バイト目だけのデータを集めて連続で配置します。
       その後、同様に1バイト目のデータだけを集めて連続で配置するということをすべてのバイトに対して繰り返します。

       ``COMPRESS_ZLIB`` や ``COMPRESS_LZ4`` や ``COMPRESS_ZSTD`` といった圧縮アルゴリズムは同じ値が連続しているデータだと圧縮率が高くなる傾向があります。
       このフラグのポイントは、Nバイトごとにデータを並び替えることで同じ値が連続しているデータを作り出すことです。

       具体的には、以下のように動作します。

       例えば、 ``UInt32`` のベクターカラム ``[1, 1051, 515]`` があるとします。 これをリトルエンディアンのバイト列で表現すると以下のようになります。

       .. code-block::

          | Byte 0 | Byte 1 |  | Byte 0 | Byte 1 |  | Byte 0 | Byte 1 |
          |--------|--------|  |--------|--------|  |--------|--------|
          | 0x00   | 0x01   |, | 0x04   | 0x1b   |, | 0x02   | 0x03   |

       このフラグは、上記の ``Byte 0`` の値をまとめ、その後 ``Byte 1`` のデータをまとめて、以下のようなデータを作るフラグです。
       この、Nバイト毎にまとめる操作を以後、シャッフルと呼びます。
       上記のデータをシャッフルすると以下のようになります。

       .. code-block::

          | Byte 0 | Byte 0 | Byte 0 |  | Byte 1 | Byte 1 | Byte 1 |
          |--------|--------|--------|  |--------|--------|--------|
          | 0x00   | 0x04   | 0x02   |, | 0x01   | 0x1b   | 0x03   |

       ポイントは「同じ値が連続する箇所があるかどうか？」です。 上記の例を見てみると、シャッフル後のデータは同じ値が連続している箇所がありません。
       このようなデータでは、シャッフルしても圧縮率の向上に寄与しないため、このフラグを適用するには不向きなデータです。

       一方で、 ``UInt32`` のベクターカラム ``[1, 2, 3]`` というデータでは、シャッフルすると以下のようになります。
       まず、説明のため ``UInt32`` の ``[1, 2, 3]`` を以下のようなリトルエンディアンのバイト列表現にします。

       .. code-block::

          | Byte 0 | Byte 1 |  | Byte 0 | Byte 1 |  | Byte 0 | Byte 1 |
          |--------|--------|  |--------|--------|  |--------|--------|
          | 0x00   | 0x01   |, | 0x00   | 0x02   |, | 0x00   | 0x03   |


       そして、 ``UInt32`` の ``[1, 2, 3]`` をシャッフルすると以下のようなデータになります。

       .. code-block::

          | Byte 0 | Byte 0 | Byte 0 |  | Byte 1 | Byte 1 | Byte 1 |
          |--------|--------|--------|  |--------|--------|--------|
          | 0x00   | 0x00   | 0x00   |, | 0x01   | 0x02   | 0x03   |

       ``UInt32`` の ``[1, 2, 3]`` というデータだと、シャッフル後に ``Byte 1`` のデータがすべて ``0x00`` になり、同じ値が連続することになります。
       したがって、このようなデータは圧縮率の向上に寄与するので、このフラグを適用するのに向いているデータです。

       浮動小数点数であれば、符号と指数部が同じデータが該当します。
       IEEE 754 形式の単精度浮動小数点数の場合、符号は31ビット目に配置され、指数部は30ビット目から23ビット目に配置されます。
       符号ビットと指数部の上位7ビットで最上位のバイトが構成されるので符号ビットと指数部の上位7ビットが同じになれば、シャッフル後は同じ値が連続することになるからです。

       例えば、 ``Float32`` の ``[0.5, 0.6]`` というデータは、IEEE 754 形式の単精度浮動小数点数で表現すると以下のように符号ビットと指数部が同一です。

       .. code-block::

          | 符号(1bit)　| 指数部(8bit) | 仮数部(23bit)                 |
          |------------|-------------|------------------------------|
          | 0          | 0111 1110   | 000 0000 0000 0000 0000 0000 |
          | 0          | 0111 1110   | 001 1001 1001 1001 1001 1010 |

       これを、シャッフルすると以下のようになり、 ``Byte 3`` に同じ値が連続することになります。

       .. code-block::

          | Byte 0 | Byte 0 |  | Byte 1 | Byte 1 |  | Byte 2 | Byte 2 |  | Byte 3 | Byte 3 | 
          |--------|--------|  |--------|--------|  |--------|--------|  |--------|--------| 
          | 0x00   | 0x9a   |, | 0x00   | 0x99   |, | 0x00   | 0x19   |, | 0x3f   | 0x3f   | 

       ``Float/Float32`` 型のデータの場合は、 ``COMPRESS_FILTER_TRUNCATE_PRECISION_1BYTE`` または ``COMPRESS_FILTER_TRUNCATE_PRECISION_2BYTES`` を組み合わせて使うこともできるので、 ``COMPRESS_FILTER_TRUNCATE_PRECISION_1BYTE`` と ``COMPRESS_FILTER_TRUNCATE_PRECISION_2BYTES`` の説明も合わせて参照してください。

   * - ``COMPRESS_FILTER_BYTE_DELTA``
     - .. versionadded:: 13.0.8

       ``COMPRESS_FILTER_BYTE_DELTA`` は圧縮前にデータをフィルタリングすることで
       ``COMPRESS_ZLIB`` / ``COMPRESS_LZ4`` / ``COMPRESS_ZSTD`` の圧縮率を高めるためのフラグです。

       ただし、データによって効果があることもあれば効果がないこともあります。圧縮率が下がることもありえます。
       また、このフラグを有効にすることで追加の処理が入るのでカラムの保存・参照処理は確実に遅くなります。
       データに合わせて効果があるフィルターだけ有効にすることが重要です。

       なお、 ``COMPRESS_ZLIB``/``COMPRESS_LZ4``/``COMPRESS_ZSTD`` を指定しない場合は
       `BloscLZ <https://www.blosc.org/pages/blosc-in-depth/#blosc-as-a-meta-compressor>`_ という圧縮アルゴリズムが使われるので、
       このフラグを有効にすることでほとんどの場合は未圧縮の場合よりもサイズが小さくなります。
       しかし、データによってはこのフラグを有効にせずに単に ``COMPRESS_ZLIB`` / ``COMPRESS_LZ4`` / ``COMPRESS_ZSTD`` を指定するだけで
       十分であることもあるため、実際のデータにあわせて適切にフラグを設定するようにしてください。

       ``COMPRESS_FILTER_SHUFFLE`` フラグはBloscサポートが有効になっていないと無視されることに注意してください。
       各種パッケージでは有効になっていますが、自分でビルドするときは明示的に有効にする必要があります。
       自分でビルドする場合は `:doc:/install/others` を参照してください。

       このフラグは ``COLUMN_VECTOR`` でのみ使用できます。 ``COLUMN_SCALAR`` のときはこのフラグは無視されます。

       では、このフラグがどのようなデータに対して有効かを記載します。

       このフラグは、圧縮対象の値のバイト間の差分を計算するフラグです。
       例えば ``UInt8`` のベクターカラム[1,2,3,4,5]というデータの場合、このフラグを適用すると[1,(2-1),(3-2),(4-3),(5-4)]=[1,1,1,1,1]というデータになります。
       上記のように差分データを計算することで、同じ値が連続するデータを作り圧縮率を向上させることを狙っています。
       ポイントは、各要素間の差分を計算することで、同じ値が連続するデータを作り出すことにあります。

       したがって、差分を計算しても同じ値が連続するデータを作り出すことができないデータは、このフラグを適用するには不向きなデータです。
       どのようなデータであれば、差分を計算して同じ値が連続するようになるのかを以下に記載します。

       まずは、前述の例でもあげた、 ``UInt8``:[1,2,3,4,5]というような一定のペースで値が増加していくデータです。
       一定のペースなので、 ``UInt8``:[1,11,21,31,41]のように10ずつ値が増加するようなデータでも良いです。

       次に、 ``UInt8``:[5,5,5,5,5]のように同じ値が連続するデータです。
       このデータは差分を計算すると[5,0,0,0,0]となり、0が連続するようになります。
       逆に、 ``UInt8``:[1, 255, 2, 200, 1]のように差分を計算しても同じ値が連続しないデータは、このフラグには不向きなデータです。

       ただ、差分を計算しても同じ値が連続しなかったり、差分自体が大きい値になる場合でも、 ``COMPRESS_FILTER_SHUFFLE`` を合わせて使うことで
       圧縮率を向上させられるケースもあります。

       例えば、下記のデータでは、単に ``COMPRESS_FILTER_BYTE_DELTA`` を適用しただけでは、同じ値も連続しませんし、差分自体も大きい値です。
       ``UInt32``: [4526677, 4592401, 4658217, 4723879] -> 差分データ:[4526677, 65724, 65816, 65662]

       しかし、このデータに ``COMPRESS_FILTER_SHUFFLE`` を適用すると以下のようになります。
       説明のため、まず以下のように ``UInt32``: [4526677, 4592401, 4658217, 4723879] をリトルエンディアンのバイト列で表現します。

       .. code-block::

          | Byte 0 | Byte 1 | Byte 2 |  | Byte 0 | Byte 1 | Byte 2 |  | Byte 0 | Byte 1 | Byte 2 |  | Byte 0 | Byte 1 | Byte 2 | 
          |--------|--------|--------|  |--------|--------|--------|  |--------|--------|--------|  |--------|--------|--------| 
          | 0x55   | 0x12   | 0x45   |, | 0x11   | 0x13   | 0x46   |, | 0x29   | 0x14   | 0x47   |, | 0xA7   | 0x14   | 0x48   | 

       このデータに対して、 ``COMPRESS_FILTER_SHUFFLE`` を適用すると以下のようなバイト列になります。

       .. code-block::

          | Byte 0 | Byte 0 | Byte 0 | Byte 0 |  | Byte 1 | Byte 1 | Byte 1 | Byte 1 |  | Byte 2 | Byte 2 | Byte 2 | Byte 2 | 
          |--------|--------|--------|--------|  |--------|--------|--------|--------|  |--------|--------|--------|--------| 
          | 0x55   | 0x11   | 0x29   | 0xA7   |, | 0x12   | 0x13   | 0x14   | 0x14   |, | 0x45   | 0x46   | 0x47   | 0x48   | 

       シャッフル後のByte1のデータとByte2のデータに注目してください。
       Byte1のデータの差分は["\x12", "\x1", "\x1", "\x0"], Byte2のデータの差分は["\x45", "\x1", "\x1", "\x1"]となり、差分の値が小さく、また同じ値が連続するようになります。

       このように、 ``COMPRESS_FILTER_BYTE_DELTA`` を適用しただけでは圧縮率の向上が見込めないデータでも、 ``COMPRESS_FILTER_SHUFFLE`` と合わせて使うことで
       圧縮率の向上を見込めるケースがあります。
       ただし、 ``COMPRESS_FILTER_SHUFFLE`` は同じバイトのデータを集めるフィルターなので、1バイトのデータに対して適用しても
       意味がありません。（1バイトのデータの場合は、シャッフルしてもしなくても同じデータ列になるためです。）
       したがって、 ``COMPRESS_FILTER_BYTE_DELTA`` と ``COMPRESS_FILTER_SHUFFLE`` を合わせて使う場合は、 ``Int8`` / ``UInt8`` 以外
       のデータに対して使用してください。

   * - ``COMPRESS_FILTER_TRUNCATE_PRECISION_1BYTE``
     - .. versionadded:: 13.0.8

       ``COMPRESS_FILTER_TRUNCATE_PRECISION_1BYTE`` は圧縮前にデータをフィルタリングすることで
       ``COMPRESS_ZLIB`` / ``COMPRESS_LZ4`` / ``COMPRESS_ZSTD`` の圧縮率を高めるためのフラグです。

       ただし、データによって効果があることもあれば効果がないこともあります。圧縮率が下がることもありえます。
       また、このフラグを有効にすることで追加の処理が入るのでカラムの保存・参照処理は確実に遅くなります。
       データに合わせて効果があるフィルターだけ有効にすることが重要です。

       なお、 ``COMPRESS_ZLIB``/ ``COMPRESS_LZ4`` / ``COMPRESS_ZSTD`` を指定しない場合は
       `BloscLZ <https://www.blosc.org/pages/blosc-in-depth/#blosc-as-a-meta-compressor>`_ という圧縮アルゴリズムが使われるので、
       このフラグを有効にすることでほとんどの場合は未圧縮の場合よりもサイズが小さくなります。
       しかし、データによってはこのフラグを有効にせずに単に ``COMPRESS_ZLIB`` / ``COMPRESS_LZ4`` / ``COMPRESS_ZSTD`` を指定するだけで
       十分であることもあるため、実際のデータにあわせて適切にフラグを設定するようにしてください。

       ``COMPRESS_FILTER_SHUFFLE`` フラグはBloscサポートが有効になっていないと無視されることに注意してください。
       各種パッケージでは有効になっていますが、自分でビルドするときは明示的に有効にする必要があります。
       自分でビルドする場合は `:doc:/install/others` を参照してください。

       このフラグは ``COLUMN_VECTOR`` でのみ使用できます。 ``COLUMN_SCALAR`` のときはこのフラグは無視されます。
       このフラグは、 ``Float/Float32`` でのみ使用できます。また、 ``COMPRESS_FILTER_SHUFFLE`` と組み合わせて使うことを想定しています。

       では、このフラグがどのようなデータに対して有効かを記載します。

       このフラグは、 ``Float`` / ``Float32`` 型のベクターカラムの各要素の精度を1バイト落とします。
       精度を落とすとは、浮動小数点数の最下位バイトをすべて0にすることです。

       例えば、 ``Float32``:[1.25, 3.67, 4.55]というデータは、IEEE 754 形式の単精度浮動小数点数で表現すると以下のように表現できます。

       .. code-block::

          | 符号(1bit)　| 指数部(8bit) | 仮数部(23bit)                 |
          |------------|-------------|------------------------------|
          | 0          | 0111 1111   | 010 0000 0000 0000 0000 0000 |
          | 0          | 1000 0000   | 110 1010 1110 0001 0100 1000 |
          | 0          | 1000 0001   | 001 0001 1001 1001 1001 1010 |

       このフラグを適用すると、最下位バイトをすべて0にするので以下のようなデータになります。
       仮数部の最下位バイトが0になっていることに注目してください。

       .. code-block::

          | 符号(1bit)　| 指数部(8bit) | 仮数部(23bit)                 |
          |------------|-------------|------------------------------|
          | 0          | 0111 1111   | 010 0000 0000 0000 0000 0000 |
          | 0          | 1000 0000   | 110 1010 1110 0001 0000 0000 |
          | 0          | 1000 0001   | 001 0001 1001 1001 0000 0000 |

       ここまでが、このフラグ単独で使用した時に実行される操作です。
       前述の通り、このフラグは ``COLUMN_FILTER_SHUFFLE`` と組み合わせて使うことを想定しています。
       そのため、ここから更にデータをシャッフルすることで、圧縮率の向上を期待しています。

       ``COMPRESS_FILTER_TRUNCATE_PRECISION_1BYTE`` を適用したデータをシャッフルすると以下のようなデータになります。

       .. code-block::

          | Byte 0 | Byte 0 | Byte 0 |  | Byte 1 | Byte 1 | Byte 1 |  | Byte 2 | Byte 2 | Byte 2 |  | Byte 3 | Byte 3 | Byte 3 | 
          |--------|--------|--------|  |--------|--------|--------|  |--------|--------|--------|  |--------|--------|--------| 
          | 0x00   | 0x00   | 0x00   |, | 0x00   | 0xe1   | 0x99   |, | 0xa0   | 0x6a   | 0x91   |, | 0x3f   | 0x40   | 0x40   | 

       ``Byte 0`` に注目してください。 ``Byte 0`` は0が連続するデータになります。

       ``COMPRESS_FILTER_TRUNCATE_PRECISION_1BYTE`` を適用せずにシャッフルすると以下のようなデータになり、どのバイトも同じ値は連続していません。

       .. code-block::

          | Byte 0 | Byte 0 | Byte 0 |  | Byte 1 | Byte 1 | Byte 1 |  | Byte 2 | Byte 2 | Byte 2 |  | Byte 3 | Byte 3 | Byte 3 | 
          |--------|--------|--------|  |--------|--------|--------|  |--------|--------|--------|  |--------|--------|--------| 
          | 0x00   | 0x48   | 0x9a   |, | 0x00   | 0xe1   | 0x99   |, | 0xa0   | 0x6a   | 0x91   |, | 0x3f   | 0x40   | 0x40   | 

       このように単純にシャッフルしただけでは、圧縮率の向上が見込めないデータでも ``COMPRESS_FILTER_TRUNCATE_PRECISION_1BYTE`` を適用することで、圧縮率の向上が見込める場合があります。

       ただし、圧縮対象の浮動小数点数の精度が1バイト分落ちるので、その分不正確なデータになることに注意してください。

   * - ``COMPRESS_FILTER_TRUNCATE_PRECISION_2BYTES``
     - .. versionadded:: 13.0.8

       ``COMPRESS_FILTER_TRUNCATE_PRECISION_2BYTES`` は圧縮前にデータをフィルタリングすることで
       ``COMPRESS_ZLIB`` / ``COMPRESS_LZ4`` / ``COMPRESS_ZSTD`` の圧縮率を高めるためのフラグです。

       ただし、データによって効果があることもあれば効果がないこともあります。圧縮率が下がることもありえます。
       また、このフラグを有効にすることで追加の処理が入るのでカラムの保存・参照処理は確実に遅くなります。
       データに合わせて効果があるフィルターだけ有効にすることが重要です。

       なお、 ``COMPRESS_ZLIB``/ ``COMPRESS_LZ4`` / ``COMPRESS_ZSTD`` を指定しない場合は
       `BloscLZ <https://www.blosc.org/pages/blosc-in-depth/#blosc-as-a-meta-compressor>`_ という圧縮アルゴリズムが使われるので、
       このフラグを有効にすることでほとんどの場合は未圧縮の場合よりもサイズが小さくなります。
       しかし、データによってはこのフラグを有効にせずに単に ``COMPRESS_ZLIB`` / ``COMPRESS_LZ4`` / ``COMPRESS_ZSTD`` を指定するだけで
       十分であることもあるため、実際のデータにあわせて適切にフラグを設定するようにしてください。

       ``COMPRESS_FILTER_SHUFFLE`` フラグはBloscサポートが有効になっていないと無視されることに注意してください。
       各種パッケージでは有効になっていますが、自分でビルドするときは明示的に有効にする必要があります。
       自分でビルドする場合は `:doc:/install/others` を参照してください。

       このフラグは ``COLUMN_VECTOR`` でのみ使用できます。 ``COLUMN_SCALAR`` のときはこのフラグは無視されます。
       このフラグは、 ``Float/Float32`` でのみ使用できます。また、 ``COMPRESS_FILTER_SHUFFLE`` と組み合わせて使うことを想定しています。

       では、このフラグがどのようなデータに対して有効かを記載します。

       このフラグは、 ``Float`` / ``Float32`` 型のベクターカラムの各要素の精度を2バイト落とします。
       精度を落とすとは、浮動小数点数の下位2バイトをすべて0にすることです。

       例えば、 ``Float32``:[1.25, 3.67, 4.55]というデータは、IEEE 754 形式の単精度浮動小数点数で表現すると以下のように表現できます。

       .. code-block::

          | 符号(1bit)　| 指数部(8bit) | 仮数部(23bit)                 |
          |------------|-------------|------------------------------|
          | 0          | 0111 1111   | 010 0000 0000 0000 0000 0000 |
          | 0          | 1000 0000   | 110 1010 1110 0001 0100 1000 |
          | 0          | 1000 0001   | 001 0001 1001 1001 1001 1010 |

       このフラグを適用すると、下位2バイトをすべて0にするので以下のようなデータになります。
       仮数部の下位2バイトがすべて0になっていることに注目してください。

       .. code-block::

          | 符号(1bit)　| 指数部(8bit) | 仮数部(23bit)                 |
          |------------|-------------|------------------------------|
          | 0          | 0111 1111   | 010 0000 0000 0000 0000 0000 |
          | 0          | 1000 0000   | 110 1010 0000 0000 0000 0000 |
          | 0          | 1000 0001   | 001 0001 0000 0000 0000 0000 |

       ここまでが、このフラグ単独で使用した時に実行される操作です。
       前述の通り、このフラグは ``COLUMN_FILTER_SHUFFLE`` と組み合わせて使うことを想定しています。
       そのため、ここから更にデータをシャッフルすることで、圧縮率の向上を期待しています。

       ``COMPRESS_FILTER_TRUNCATE_PRECISION_2BYTE`` を適用したデータをシャッフルすると以下のようなデータになります。

       .. code-block::

          | Byte 0 | Byte 0 | Byte 0 |  | Byte 1 | Byte 1 | Byte 1 |  | Byte 2 | Byte 2 | Byte 2 |  | Byte 3 | Byte 3 | Byte 3 | 
          |--------|--------|--------|  |--------|--------|--------|  |--------|--------|--------|  |--------|--------|--------| 
          | 0x00   | 0x00   | 0x00   |, | 0x00   | 0x00   | 0x00   |, | 0xa0   | 0x6a   | 0x91   |, | 0x3f   | 0x40   | 0x40   | 

       ``Byte 0`` と ``Byte 1`` に注目してください。 ``Byte 0`` と ``Byte 1`` は0が連続するデータになります。

       ``COMPRESS_FILTER_TRUNCATE_PRECISION_2BYTE`` を適用せずにシャッフルすると以下のようなデータになり、どのバイトも同じ値は連続していません。

       .. code-block::

          | Byte 0 | Byte 0 | Byte 0 |  | Byte 1 | Byte 1 | Byte 1 |  | Byte 2 | Byte 2 | Byte 2 |  | Byte 3 | Byte 3 | Byte 3 | 
          |--------|--------|--------|  |--------|--------|--------|  |--------|--------|--------|  |--------|--------|--------| 
          | 0x00   | 0x48   | 0x9a   |, | 0x00   | 0xe1   | 0x99   |, | 0xa0   | 0x6a   | 0x91   |, | 0x3f   | 0x40   | 0x40   | 

       このように単純にシャッフルしただけでは、圧縮率の向上が見込めないデータでも ``COMPRESS_FILTER_TRUNCATE_PRECISION_2BYTE`` を適用することで、圧縮率の向上が見込める場合があります。

       ただし、圧縮対象の浮動小数点数の精度が2バイト分落ちるので、その分不正確なデータになることに注意してください。

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

   * - ``WEIGHT_FLOAT32``
     - .. versionadded:: 10.0.3

       You can use 32bit floating point instead of 32bit unsigned
       integer for weight value.

       You also need to specify ``WITH_WEIGHT``.

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

   * - ``MISSING_ADD``
     - .. versionadded:: 12.0.2

       You can't specify multiple ``MISSING_*`` flags. They are
       exclusive.

       This is meaningful only for reference scalar and vector
       columns.

       If this flag is specified and nonexistent key in the referred
       table is specified to the column's value, a new record is
       created in the referred table automatically.

       If you don't specify any ``MISSING_*`` flag, ``MISSING_ADD`` is
       used as the default.

       See also :ref:`column-create-missing-mode`.

       This flag is available only for ``COLUMN_SCALAR`` and
       ``COLUMN_VECTOR``.

   * - ``MISSING_IGNORE``
     - .. versionadded:: 12.0.2

       You can't specify multiple ``MISSING_*`` flags. They are
       exclusive.

       This is meaningful only for reference scalar and vector
       columns.

       If this flag is specified and nonexistent key in the referred
       table is specified to the column's value, the value is just
       ignored. If the column is a scalar column, ``GRN_ID_NIL``
       (``0``) is stored because Groonga doesn't support the NULL
       value. If the column is a vector column, the element is just
       removed from the value. For example, ``["existent1",
       "nonexistent", "existent2"]`` is set to the vector column and
       ``"nonexistent"`` record doesn't exist in the referred table,
       ``["existent1", "existent2"]`` are set to the vector column.

       See also :ref:`column-create-missing-mode`.

       This flag is available only for ``COLUMN_SCALAR`` and
       ``COLUMN_VECTOR``.

   * - ``MISSING_NIL``
     - .. versionadded:: 12.0.2

       You can't specify multiple ``MISSING_*`` flags. They are
       exclusive.

       This is meaningful only for reference scalar and vector
       columns.

       If this flag is specified and nonexistent key in the referred
       table is specified to the column's value, the value is replaced
       with ``GRN_ID_NIL`` (``0``).

       See also :ref:`column-create-missing-mode`.

       This flag is available only for ``COLUMN_SCALAR`` and
       ``COLUMN_VECTOR``.

   * - ``INVALID_ERROR``
     - .. versionadded:: 12.0.2

       You can't specify multiple ``INVALID_*`` flags. They are
       exclusive.

       If this flag is specified and an invalid value is specified, an
       error is reported to :ref:`process-log`.

       For example, ``"STRING"`` for ``Int32`` column is an invalid
       value.

       If the column is a scalar column, :doc:`load` also reports an
       error.

       If the column is a vector column, :doc:`load` doesn't reports
       an error but invalid values in a vector value are removed or
       replaced with ``GRN_ID_NIL`` (``0``) depending on ``MISSING_*``
       flag of the column.

       .. note::

          This is an incompatible change at 12.0.2. :doc:`load`
          also reports an error for a vector column before 12.0.2.

       If you don't specify any ``INVALID_*`` flag, ``INVALID_ERROR`` is
       used as the default.

       See also :ref:`column-create-invalid-mode`.

       This flag is available only for ``COLUMN_SCALAR`` and
       ``COLUMN_VECTOR``.

   * - ``INVALID_WARN``
     - .. versionadded:: 12.0.2

       You can't specify multiple ``INVALID_*`` flags. They are
       exclusive.

       If this flag is specified and an invalid value is specified, a
       warning is reported to :ref:`process-log` but no error is
       reported.

       For example, ``"STRING"`` for ``Int32`` column is an invalid
       value.

       If the column is a vector column, invalid values in a vector
       value are removed or replaced with ``GRN_ID_NIL`` (``0``)
       depending on ``MISSING_*`` flag of the column.

       See also :ref:`column-create-invalid-mode`.

       This flag is available only for ``COLUMN_SCALAR`` and
       ``COLUMN_VECTOR``.

   * - ``INVALID_IGNORE``
     - .. versionadded:: 12.0.2

       You can't specify multiple ``INVALID_*`` flags. They are
       exclusive.

       If this flag is specified and an invalid value is specified,
       it's just ignored.

       For example, ``"STRING"`` for ``Int32`` column is an invalid
       value.

       If the column is a vector column, invalid values in a vector
       value are removed or replaced with ``GRN_ID_NIL`` (``0``)
       depending on ``MISSING_*`` flag of the column.

       See also :ref:`column-create-invalid-mode`.

       This flag is available only for ``COLUMN_SCALAR`` and
       ``COLUMN_VECTOR``.

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
