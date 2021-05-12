.. -*- rst -*-

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

This command takes only one optional parameter::

  lock_clear [target_name=null]

If ``target_name`` parameters is omitted, database is used for the
target object. It means that all locks in the database are cleared.

Usage
-----

Here is an example to clear all locks in the database:

.. groonga-command
.. include:: ../../example/reference/commands/lock_clear/database.log
.. lock_clear

Here is an example to clear locks of ``Entries`` table and ``Entries``
table columns:

.. groonga-command
.. include:: ../../example/reference/commands/lock_clear/table.log
.. table_create Entries TABLE_NO_KEY
.. column_create Entries body COLUMN_SCALAR Text
.. lock_clear Entries

Here is an example to clear the lock of ``Sites.title`` column:

.. groonga-command
.. include:: ../../example/reference/commands/lock_clear/column.log
.. table_create Sites TABLE_HASH_KEY ShortText
.. column_create Sites title COLUMN_SCALAR ShortText
.. lock_clear Sites.title

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

``lock_clear`` command returns whether lock is cleared successfully or
not::

 [HEADER, SUCCEEDED_OR_NOT]

``HEADER``

  See :doc:`/reference/command/output_format` about ``HEADER``.

``SUCCEEDED_OR_NOT``

  If command succeeded, it returns true, otherwise it returns false on error.
