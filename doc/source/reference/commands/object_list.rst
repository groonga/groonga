.. -*- rst -*-

.. groonga-command
.. database: commands_object_list

``object_list``
===============

Summary
-------

.. versionadded:: 6.0.7

``object_list`` lists objects in database. Object information is taken
from metadata in database. ``object_list`` doesn't open any
objects. So ``object_list`` is a light command for database that has
many tables and/or columns.

Normally, :doc:`schema` is a useful command than
``object_list``. Because :doc:`schema` returns more information than
``object_list``.

Syntax
------

This command takes no parameters::

  object_list

Usage
-----

Here is an example schema to show example output:

.. groonga-command
.. include:: ../../example/reference/commands/object_list/sample.log
.. table_create Memos TABLE_HASH_KEY ShortText
.. column_create Memos content COLUMN_SCALAR Text
..
.. table_create Terms TABLE_PAT_KEY ShortText \
..   --default_tokenizer TokenBigram \
..   --normalizer NormalizerAuto
.. column_create Terms memos_content_index \
..   COLUMN_INDEX|WITH_POSITION \
..   Memos content

Here is an output of ``object_list`` command against this database:

.. groonga-command
.. include:: ../../example/reference/commands/object_list/output.log
.. object_list

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

``object_list`` command returns a list of object information in the
database::

  [HEADER, OBJECTS]

``HEADER``
^^^^^^^^^^

See :doc:`/reference/command/output_format` about ``HEADER``.

``OBJECTS``
^^^^^^^^^^^

``OBJECTS`` are pairs of object name and object details::

  {
    "OBJECT_1": OBJECT_1,
    "OBJECT_2": OBJECT_2,
    ...,
    "OBJECT_n": OBJECT_n,
  }

Each ``OBJECT`` consists of common properties and object type specific
properties.

``OBJECT`` (common properties)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Here are common properties::

  {
    "id":         ID,
    "name":       NAME,
    "opened":     OPENED,
    "value_size": N_BYTES,
    "n_elements": N_ELEMENTS,
    "type":       OBJECT_TYPE,
    "flags":      FLAGS,
    "path":       PATH
  }

.. list-table::
   :header-rows: 1

   * - Name
     - Description
   * - ``id``
     - The ID of the object.
   * - ``name``
     - The name of the object.
   * - ``opened``
     - Whether the object is opened or not. It's ``true`` or
       ``false``.  If the object is opened, this value is
       ``true``. Otherwise, this value is ``false``.
   * - ``value_size``
     - The number of bytes of the metadata. It's an integer.

       It's appeared only when the metadata are broken. If this
       property is appeared, ``type``, ``n_elements``, ``flags`` and
       ``path`` aren't appeared. Because they can't be retrieved from
       broken metadata.
   * - ``n_elements``
     - The number of internal elements in the metadata. It's an
       integer.
   * - ``type``
     - The type of the object. See
       :ref:`object-list-return-value-object-type` for details.
   * - ``flags``
     - The flags of the object. See
       :ref:`object-list-return-value-flags` for details.
   * - ``path``
     - The path that contains data of the object. It's ``null`` for
       objects that doesn't have data. For example, command object
       doesn't have data.

.. _object-list-return-value-object-type:

``OBJECT_TYPE``
^^^^^^^^^^^^^^^

``OBJECT_TYPE`` represents the type of object. For example, patricia
trie table, hash table and index column are object types.

``OBJECT_TYPE`` has the following properties::

  {
    "id":   ID,
    "name": NAME
  }

.. list-table::
   :header-rows: 1

   * - Name
     - Description
   * - ``id``
     - The ID of the type.
   * - ``name``
     - The name of the type. For example, :ref:`table-hash-key` is
       ``"table:hash_key"``, :doc:`/reference/columns/vector` is
       ``"column:var_size"`` and :doc:`/reference/columns/index` is
       ``"column:index"``.

.. _object-list-return-value-flags:

``FLAGS``
^^^^^^^^^

``FLAGS`` represents the flags of object. The most flags are the
specified flags in :doc:`table_create` or :doc:`column_create`. Some
flags are added internally.

``FLAGS`` has the following properties::

  {
    "value": VALUE,
    "names": NAMES
  }

.. list-table::
   :header-rows: 1

   * - Name
     - Description
   * - ``value``
     - The value of the flags. It's an integer.
   * - ``names``
     - The names of each flag. It's a string. Each name is separated
       by ``|`` such as ``TABLE_HASH_KEY|PERSISTENT``.

``OBJECT`` (``"type"`` object type specific properties)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Here are ``"type"`` object type specific properties in ``OBJECT``::

  {
    "size": SIZE
  }

.. list-table::
   :header-rows: 1

   * - Name
     - Description
   * - ``size``
     - The size of the type. If the type is fixed size type, the size
       is the size of each value. If the type is variable size type,
       the size is the maximum size of each value.

``OBJECT`` (``"proc"`` object type specific properties)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Here is a list of object that is ``"proc"`` object type:

  * :doc:`/reference/command`
  * :doc:`/reference/function`
  * :doc:`/reference/normalizers`
  * :doc:`/reference/tokenizers`
  * :doc:`/reference/token_filters`

Here are ``"proc"`` object type specific properties in ``OBJECT``::

  {
    "plugin_id": PLUGIN_ID
  }

.. list-table::
   :header-rows: 1

   * - Name
     - Description
   * - ``plugin_id``
     - The plugin ID of the ``"proc"`` object. If the ``"proc"``
       object is defined by plugin, the value is ``1`` or more larger
       integer. Plugin ID is ``0`` for builtin ``"proc"`` object.

``OBJECT`` (``"table:*"`` object types specific properties)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Here are ``"table:*"`` object types specific properties in
``OBJECT``::

  {
    "range":         RANGE,
    "token_filters": TOKEN_FILTERS
  }

.. list-table::
   :header-rows: 1

   * - Name
     - Description
   * - ``range``
     - The type of the table's value. See
       :ref:`object-list-return-value-range` for details.
   * - ``token_filters``
     - The token filters of the table. See
       :ref:`object-list-return-value-token-filters` for details.

.. _object-list-return-value-range:

``RANGE``
^^^^^^^^^

``RANGE`` represents the type of the range.

``RANGE`` has the following properties::

  {
    "id":   ID,
    "name": NAME
  }

.. list-table::
   :header-rows: 1

   * - Name
     - Description
   * - ``id``
     - The ID of the type of the range. It's an integer. If the object
       doesn't have range, ID is ``0``.
   * - ``name``
     - The name of the type of the range. It's a string or ``null``.
       If the object doesn't have range, name is ``null``.

.. _object-list-return-value-token-filters:

``TOKEN_FILTERS``
^^^^^^^^^^^^^^^^^

``TOKEN_FILTERS`` represents the token filters of the table.

``TOKEN_FILTERS`` is an array of ``TOKEN_FILTER``::

  [
    TOKEN_FILTER_1,
    TOKEN_FILTER_2,
    ...,
    TOKEN_FILTER_n
  ]

``TOKEN_FILTER`` has the following properties::

  {
    "id":   ID,
    "NAME": NAME
  }

.. list-table::
   :header-rows: 1

   * - Name
     - Description
   * - ``id``
     - The ID of the token filter. It's an integer.
   * - ``name``
     - The name of the token filter. It's a string.

``OBJECT`` (``"column:*"`` object types specific properties)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Here are ``"column:*"`` object types specific properties in
``OBJECT``::

  {
    "range": RANGE
  }

.. list-table::
   :header-rows: 1

   * - Name
     - Description
   * - ``range``
     - The type of the column's value. See
       :ref:`object-list-return-value-range` for details.

``OBJECT`` (``"column:index"`` object type specific properties)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Here are ``"column:index"`` object type specific properties in
``OBJECT``::

  {
    "sources": SOURCES
  }

.. list-table::
   :header-rows: 1

   * - Name
     - Description
   * - ``sources``
     - The sources of the index column. See
       :ref:`object-list-return-value-sources` for details.

.. _object-list-return-value-sources:

``SOURCES``
^^^^^^^^^^^

``SOURCES`` represents the sources of the index column.

``SOURCES`` is an array of ``SOURCE``::

  [
    SOURCE_1,
    SOURCE_2,
    ...,
    SOURCE_n
  ]

``SOURCE`` has the following properties::

  {
    "id":   ID,
    "NAME": NAME
  }

.. list-table::
   :header-rows: 1

   * - Name
     - Description
   * - ``id``
     - The ID of the source table or column. It's an integer.
   * - ``name``
     - The name of the source table or column. It's a string.

See also
--------

  * :doc:`schema`
