.. -*- rst -*-

.. highlightlang:: none

.. groonga-command
.. database: lock_clear

``lock_clear``
==============

Summary
-------

.. versionadded:: 4.0.9

``lock_clear`` command clear the lock of the target object
recursively. The target object is one of database, table and column.

.. note::

   This is a dangerous command. You must not use this command while
   other process or thread is doing a write operation to the target
   object. If you do it, your database may be broken and/or your
   process may be crashed.

Syntax
------

``lock_clear`` command takes only one parameter.

There is no required parameter::

  lock_clear [target_name=null]

If ``target_name`` parameters is omitted, database is used for the
target object. It means that all locks in the database are cleared.

Usage
-----

Here is an example to clear all locks in the database:

.. groonga-command
.. include:: ../../example/reference/commands/lock_clear/database.log
.. lock_clear

Here is an example to clear locks of ``Entry`` table and ``Entry``
table columns:

.. groonga-command
.. include:: ../../example/reference/commands/lock_clear/table.log
.. table_create Entry TABLE_NO_KEY
.. column_create Entry body COLUMN_SCALAR Text
.. lock_clear Entry

Here is an example to clear the lock of ``Site.title`` column:

.. groonga-command
.. include:: ../../example/reference/commands/lock_clear/column.log
.. table_create Site TABLE_HASH_KEY ShortText
.. column_create Site title COLUMN_SCALAR ShortText
.. lock_clear Site.title

Parameters
----------

This section describes all parameters.

``target_name``
"""""""""""""""

Specifies the name of table or column.

If you don't specify it, database is used for the target object.

The default is none. It means that the target object is database.

Return value
------------

::

 [HEADER, SUCCEEDED_OR_NOT]

``HEADER``

  See :doc:`/reference/command/output_format` about ``HEADER``.

``SUCCEEDED_OR_NOT``

  If command succeeded, it returns true, otherwise it returns false on error.
