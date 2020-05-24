.. -*- rst -*-

.. highlightlang:: none

.. groonga-command
.. database: commands_object_exist

``object_exist``
================

Summary
-------

.. versionadded:: 5.0.6

``object_exist`` returns whether object with the specified name exists
or not in database.

It's a light operation. It just checks existence of the name in the
database. It doesn't load the specified object from disk.

``object_exist`` doesn't check object type. The existing object may be
table, column, function and so on.

Syntax
------

This command takes only one required parameter::

  object_exist name

Usage
-----

You can check whether the name is already used in database:

.. groonga-command
.. include:: ../../example/reference/commands/object_exist/usage.log
.. object_exist Users
.. table_create Users TABLE_HASH_KEY ShortText
.. object_exist Users

The ``object_exist Users`` returns ``false`` before you create
``Users`` table.

The ``object_exist Users`` returns ``true`` after you create ``Users``
table.

Parameters
----------

This section describes all parameters.

Required parameters
^^^^^^^^^^^^^^^^^^^

There is only one required parameter.

.. _object-exist-name:

``name``
""""""""

Specifies the object name to be checked.

If you want to check existence of a column, use
``TABLE_NAME.COLUMN_NAME`` format like the following:

.. groonga-command
.. include:: ../../example/reference/commands/object_exist/name_column.log
.. table_create Logs TABLE_NO_KEY
.. column_create Logs timestamp COLUMN_SCALAR Time
.. object_exist Logs.timestamp

``Logs`` is table name and ``timestamp`` is column name in
``Logs.timestamp``.

Optional parameters
^^^^^^^^^^^^^^^^^^^

There is no optional parameter.

Return value
------------

The command returns ``true`` as body if object with the specified name
exists in database such as::

  [HEADER, true]

The command returns ``false`` otherwise such as::

  [HEADER, false]

See :doc:`/reference/command/output_format` for ``HEADER``.
