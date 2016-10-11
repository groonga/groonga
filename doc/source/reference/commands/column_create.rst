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

You must specify ``COLUMN_SCALAR`` to ``flags`` parameter.

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

You must specify ``COLUMN_VECTOR`` to ``flags`` parameter.

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

You must specify table name to ``type`` parameter.

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

TODO

.. _column-create-index-small:

Create a small index column
^^^^^^^^^^^^^^^^^^^^^^^^^^^

TODO

.. _column-create-index-medium:

Create a medium index column
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

TODO

Parameters
----------

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
