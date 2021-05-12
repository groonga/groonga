.. -*- rst -*-

.. groonga-command
.. database: schema

``schema``
==========

Summary
-------

.. versionadded:: 5.0.9

``schema`` command returns schema in the database.

This command is useful when you want to inspect the database. For
example, visualizing the database, creating GUI for the database and
so on.

Syntax
------

This command takes no parameters::

  schema

Usage
-----

Here is an example schema to show example output:

.. groonga-command
.. include:: ../../example/reference/commands/schema/sample.log
.. table_create Memos TABLE_HASH_KEY ShortText
.. column_create Memos content COLUMN_SCALAR Text
..
.. table_create Terms TABLE_PAT_KEY ShortText \
..   --default_tokenizer TokenBigram \
..   --normalizer NormalizerAuto
.. column_create Terms memos_content_index \
..   COLUMN_INDEX|WITH_POSITION \
..   Memos content

Here is an output of ``schema`` command against this example schema:

.. groonga-command
.. include:: ../../example/reference/commands/schema/output.log
.. schema

Parameters
----------

This section describes all parameters.

Required parameters
^^^^^^^^^^^^^^^^^^^

There is no required parameter.

Optional parameters
^^^^^^^^^^^^^^^^^^^

There is no optional parameter.

Return value
------------

``schema`` command returns schema in the database::

  [HEADER, SCHEMA]

``HEADER``
^^^^^^^^^^

See :doc:`/reference/command/output_format` about ``HEADER``.

``SCHEMA``
^^^^^^^^^^

``SCHEMA`` is an object that consists of the following information::

  {
    "plugins":       PLUGINS,
    "types":         TYPES,
    "tokenizers":    TOKENIZERS,
    "normalizers":   NORMALIZERS,
    "token_filters": TOKEN_FITLERS,
    "tables":        TABLES
  }

``PLUGINS``
^^^^^^^^^^^

``PLUGINS`` is an object. Its key is plugin name and its value is
plugin detail::

  {
    "PLUGIN_NAME_1": PLUGIN_1,
    "PLUGIN_NAME_2": PLUGIN_2,
    ...
    "PLUGIN_NAME_n": PLUGIN_n
  }

``PLUGIN``
^^^^^^^^^^

``PLUGIN`` is an object that describes plugin detail::

  {
    "name": PLUGIN_NAME
  }

Here are properties of ``PLUGIN``:

.. list-table::
   :header-rows: 1

   * - Name
     - Description
   * - ``name``
     - The plugin name. It's used in :doc:`plugin_register`.

``TYPES``
^^^^^^^^^

``TYPES`` is an object. Its key is type name and its value is
type detail::

  {
    "TYPE_NAME_1": TYPE_1,
    "TYPE_NAME_2": TYPE_2,
    ...
    "TYPE_NAME_n": TYPE_n
  }

``TYPE``
^^^^^^^^

``TYPE`` is an object that describes type detail::

  {
    "name": TYPE_NAME,
    "size": SIZE_OF_ONE_VALUE_IN_BYTE,
    "can_be_key_type": BOOLEAN,
    "can_be_value_type": BOOLEAN
  }

Here are properties of ``TYPE``:

.. list-table::
   :header-rows: 1

   * - Name
     - Description
   * - ``name``
     - The type name.
   * - ``size``
     - The number of bytes of one value.
   * - ``can_be_key_type``
     - ``true`` when the type can be used for table key, ``false``
       otherwise.
   * - ``can_be_value_type``
     - ``true`` when the type can be used for table value, ``false``
       otherwise.

``TOKENIZERS``
^^^^^^^^^^^^^^

``TOKENIZERS`` is an object. Its key is tokenizer name and its value
is tokenizer detail::

  {
    "TOKENIZER_NAME_1": TOKENIZER_1,
    "TOKENIZER_NAME_2": TOKENIZER_2,
    ...
    "TOKENIZER_NAME_n": TOKENIZER_n
  }

``TOKENIZER``
^^^^^^^^^^^^^

``TOKENIZER`` is an object that describes tokenizer detail::

  {
    "name": TOKENIZER_NAME
  }

Here are properties of ``TOKENIZER``:

.. list-table::
   :header-rows: 1

   * - Name
     - Description
   * - ``name``
     - The tokenizer name. It's used for
       :ref:`table-create-default-tokenizer`.

``NORMALIZERS``
^^^^^^^^^^^^^^^

``NORMALIZERS`` is an object. Its key is normalizer name and its value
is normalizer detail::

  {
    "NORMALIZER_NAME_1": NORMALIZER_1,
    "NORMALIZER_NAME_2": NORMALIZER_2,
    ...
    "NORMALIZER_NAME_n": NORMALIZER_n
  }

``NORMALIZER``
^^^^^^^^^^^^^^

``NORMALIZER`` is an object that describes normalizer detail::

  {
    "name": NORMALIZER_NAME
  }

Here are properties of ``NORMALIZER``:

.. list-table::
   :header-rows: 1

   * - Name
     - Description
   * - ``name``
     - The normalizer name. It's used for
       :ref:`table-create-normalizer`.

``TOKEN_FILTERS``
^^^^^^^^^^^^^^^^^

``TOKEN_FILTERS`` is an object. Its key is token filter name and its value
is token filter detail::

  {
    "TOKEN_FILTER_NAME_1": TOKEN_FILTER_1,
    "TOKEN_FILTER_NAME_2": TOKEN_FILTER_2,
    ...
    "TOKEN_FILTER_NAME_n": TOKEN_FILTER_n
  }

``TOKEN_FILTER``
^^^^^^^^^^^^^^^^

``TOKEN_FILTER`` is an object that describes token filter detail::

  {
    "name": TOKEN_FILTER_NAME
  }

Here are properties of ``TOKEN_FILTER``:

.. list-table::
   :header-rows: 1

   * - Name
     - Description
   * - ``name``
     - The token filter name. It's used for
       :ref:`table-create-token-filters`.

``TABLES``
^^^^^^^^^^

``TABLES`` is an object. Its key is table name and its value
is table detail::

  {
    "TABLE_NAME_1": TABLE_1,
    "TABLE_NAME_2": TABLE_2,
    ...
    "TABLE_NAME_n": TABLE_n
  }

``TABLE``
^^^^^^^^^

``TABLE`` is an object that describes table detail::

  {
    "name": TABLE_NAME
    "type": TYPE,
    "key_type": KEY_TYPE,
    "value_type": VALUE_TYPE,
    "tokenizer": TOKENIZER,
    "normalizer": NORMALIZER,
    "token_filters": [
      TOKEN_FILTER_1,
      TOKEN_FILTER_2,
      ...,
      TOKEN_FILTER_n,
    ],
    "indexes": [
      INDEX_1,
      INDEX_2,
      ...,
      INDEX_n
    ],
    "command": COMMAND,
    "columns": {
      "COLUMN_NAME_1": COLUMN_1,
      "COLUMN_NAME_2": COLUMN_2,
      ...,
      "COLUMN_NAME_3": COLUMN_3,
    }
  }

Here are properties of ``TABLE``:

.. list-table::
   :header-rows: 1

   * - Name
     - Description
   * - ``name``
     - The table name.
   * - ``type``
     - The table type.

       This is one of the followings:

         * ``array``: :ref:`table-no-key`
         * ``hash``: :ref:`table-hash-key`
         * ``patricia trie``: :ref:`table-pat-key`
         * ``double array trie``: :ref:`table-dat-key`
   * - ``key_type``
     - The type of the table's key.

       If the table type is ``array``, this is ``null``.

       If the table type isn't ``array``, this is an object
       that has the following properties:

         * ``name``: The type name.
         * ``type``: ``reference`` if the type is an table, ``type``
           otherwise.
   * - ``value_type``
     - The type of the table's value.

       If the table doesn't use value, this is ``null``.

       If the table uses value, this is an object that has the
       following properties:

         * ``name``: The type name.
         * ``type``: ``reference`` if the type is an table, ``type``
           otherwise.
   * - ``tokenizer``
     - The tokenizer of the table. It's specified by
       :ref:`table-create-default-tokenizer`.

       If the table doesn't use tokenizer, this is ``null``.

       If the table uses tokenizer, this is an object that has the
       following properties:

         * ``name``: The tokenizer name.
   * - ``normalizer``
     - The normalizer of the table. It's specified by
       :ref:`table-create-normalizer`.

       If the table doesn't use normalizer, this is ``null``.

       If the table uses normalizer, this is an object that has the
       following properties:

         * ``name``: The normalizer name.
   * - ``token_filters``
     - The token filters of the table. It's specified by
       :ref:`table-create-token-filters`.

       This is an array of an object. The object has the following
       properties:

         * ``name``: The token filter name.
   * - ``indexes``
     - The indexes of the table's key.

       This is an array of :ref:`schema-return-value-index`.
   * - ``command``
     - The Groonga command information to create the table.

       This is :ref:`schema-return-value-command`.
   * - ``columns``
     - The columns of the table.

       This is an object that its key is a column name and its value
       is :ref:`schema-return-value-column`.

.. _schema-return-value-index:

``INDEX``
^^^^^^^^^

``INDEX`` is an object that describes index detail::

  {
    "full_name": INDEX_COLUMN_NAME_WITH_TABLE_NAME,
    "table":     TABLE_NAME,
    "name":      INDEX_COLUMN_NAME,
    "section":   SECTION
  }

Here are properties of ``INDEX``:

.. list-table::
   :header-rows: 1

   * - Name
     - Description
   * - ``full_name``
     - The index column name with table name.

       For example, ``Terms.index``.
   * - ``table``
     - The table name of the index column.

       For example, ``Terms``.
   * - ``name``
     - The index column name.

       For example, ``index``.
   * - ``section``
     - The section number in the index column for the table's key.

       If the index column isn't multiple column index, this is ``0``.

.. _schema-return-value-command:

``COMMAND``
^^^^^^^^^^^

``COMMAND`` is an object that describes how to create the table or
column::

  {
    "name": COMMAND_NAME,
    "arguments": {
      "KEY_1": "VALUE_1",
      "KEY_2": "VALUE_2",
      ...,
      "KEY_n": "VALUE_n"
    },
    "command_line": COMMAND_LINE
  }

Here are properties of ``COMMAND``:

.. list-table::
   :header-rows: 1

   * - Name
     - Description
   * - ``name``
     - The Groonga command name to create the table or column.
   * - ``arguments``
     - The arguments of the Groonga command to create the
       table or column.

       This is an object that its key is argument name and its value
       is argument value.
   * - ``command_line``
     - The Groonga command line to create the table or column.

       This is a string that can be evaluated by Groonga.

.. _schema-return-value-column:

``COLUMN``
^^^^^^^^^^

``COLUMN`` is an object that describes column detail::

  {
    "name": COLUMN_NAME,
    "table": TABLE_NAME,
    "full_name": COLUMN_NAME_WITH_TABLE,
    "type": TYPE,
    "value_type": VALUE_TYPE,
    "compress": COMPRESS,
    "section": SECTION,
    "weight": WEIGHT,
    "compress": COMPRESS,
    "section": BOOLEAN,
    "weight": BOOLEAN,
    "position": BOOLEAN,
    "sources": [
      SOURCE_1,
      SOURCE_2,
      ...,
      SOURCE_n
    ],
    "indexes": [
      INDEX_1,
      INDEX_2,
      ...,
      INDEX_n
    ],
    "command": COMMAND
  }

Here are properties of ``COLUMN``:

.. list-table::
   :header-rows: 1

   * - Name
     - Description
   * - ``name``
     - The column name.

       For example, ``age``.
   * - ``table``
     - The table name of the column.

       For example, ``Users``.
   * - ``full_name``
     - The column name with table name.

       For example, ``Users.age``.
   * - ``type``
     - The column type.

       This is one of the followings:

         * ``scalar``: :doc:`/reference/columns/scalar`
         * ``vector``: :doc:`/reference/columns/vector`
         * ``index``: :doc:`/reference/columns/index`
   * - ``value_type``
     - The type of the column's value.

       This is an object that has the following properties:

         * ``name``: The type name.
         * ``type``: ``reference`` if the type is an table, ``type``
           otherwise.
   * - ``compress``
     - The compression method of the column.

       If the column doesn't use any compression methods, this is
       ``null``.

       If the column uses a compression method, this is one of the
       followings:

         * ``zlib``: The column uses zlib to compress column value.
         * ``lz4``: The column uses LZ4 to compress column value.
   * - ``section``
     - Whether the column can store section information or not.

       ``true`` if the column is created with ``WITH_SECTION`` flag,
       ``false`` otherwise.

       Normally, if the column isn't an index column, this is ``false``.
   * - ``weight``
     - Whether the column can store weight information or not.

       ``true`` if the column is created with ``WITH_WEIGHT`` flag,
       ``false`` otherwise.
   * - ``position``
     - Whether the column can store position information or not.

       ``true`` if the column is created with ``WITH_POSITION`` flag,
       ``false`` otherwise.

       Normally, if the column isn't an index column, this is ``false``.
   * - ``sources``
     - The source columns of the index column.

       This is an array of :ref:`schema-return-value-source`.

       Normally, if the column isn't an index column, this is an
       empty array.
   * - ``indexes``
     - The indexes of the column.

       This is an array of :ref:`schema-return-value-index`.
   * - ``command``
     - The Groonga command information to create the column.

       This is :ref:`schema-return-value-command`.

.. _schema-return-value-source:

``SOURCE``
^^^^^^^^^^

``SOURCE`` is an object that describes source detail::

  {
    "name":      COLUMN_NAME,
    "table":     TABLE_NAME,
    "full_name": COLUMN_NAME_WITH_TABLE_NAME
  }

Here are properties of ``SOURCE``:

.. list-table::
   :header-rows: 1

   * - Name
     - Description
   * - ``name``
     - The source column name.

       For example, ``content``.

       This may be a ``_key`` pseudo column.
   * - ``table``
     - The table name of the source column.

       For example, ``Memos``.
   * - ``full_name``
     - The source column name with table name.

       For example, ``Memos.content``.

See also
--------

  * :doc:`table_create`
  * :doc:`column_create`
