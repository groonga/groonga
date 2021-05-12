.. -*- rst -*-

.. groonga-command
.. database: lock_acquire

``lock_acquire``
================

Summary
-------

.. versionadded:: 5.1.2

``lock_acquire`` command acquires the lock of the target object. The
target object is one of database, table and column.

.. note::

   This is a dangerous command. You must release locks by
   :doc:`lock_release` that you acquire when these locks are no longer
   needed. If you forget to release these locks, your database may be
   broken.

Syntax
------

This command takes only one optional parameter::

  lock_acquire [target_name=null]

If ``target_name`` parameters is omitted, database is used for the
target object.

Usage
-----

Here is an example to acquire the lock of the database:

.. groonga-command
.. include:: ../../example/reference/commands/lock_acquire/database.log
.. lock_acquire

If the database is locked, you can't create a new table and
column. Release the lock of the database to show another examples.

.. groonga-command
.. include:: ../../example/reference/commands/lock_acquire/database_release.log
.. lock_release

Here is an example to acquire the lock of ``Entries`` table:

.. groonga-command
.. include:: ../../example/reference/commands/lock_acquire/table.log
.. table_create Entries TABLE_NO_KEY
.. lock_acquire Entries

Here is an example to acquire the lock of ``Sites.title`` column:

.. groonga-command
.. include:: ../../example/reference/commands/lock_acquire/column.log
.. table_create Sites TABLE_HASH_KEY ShortText
.. column_create Sites title COLUMN_SCALAR ShortText
.. lock_acquire Sites.title

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

``lock_acquire`` command returns whether lock is acquired or not::

 [HEADER, SUCCEEDED_OR_NOT]

``HEADER``
""""""""""

See :doc:`/reference/command/output_format` about ``HEADER``.

``SUCCEEDED_OR_NOT``
""""""""""""""""""""

If command succeeded, it returns true, otherwise it returns false on error.

See also
--------

  * :doc:`lock_release`
  * :doc:`lock_clear`
