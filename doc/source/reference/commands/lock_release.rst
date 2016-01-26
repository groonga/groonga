.. -*- rst -*-

.. highlightlang:: none

.. groonga-command
.. database: lock_release

``lock_release``
================

Summary
-------

.. versionadded:: 5.1.2

``lock_release`` command releases the lock of the target object. The
target object is one of database, table and column.

.. note::

   This is a dangerous command. You must only release locks that you
   acquire by :doc:`lock_acquire`. If you release locks without
   :doc:`lock_acquire`, your database may be broken.

Syntax
------

This command takes only one optional parameter::

  lock_clear [target_name=null]

If ``target_name`` parameters is omitted, database is used for the
target object.

Usage
-----

Here is an example to release the lock of the database:

.. groonga-command
.. include:: ../../example/reference/commands/lock_release/database.log
.. lock_acquire
.. lock_release

Here is an example to release the lock of ``Entry`` table:

.. groonga-command
.. include:: ../../example/reference/commands/lock_release/table.log
.. table_create Entry TABLE_NO_KEY
.. lock_acquire Entry
.. lock_release Entry

Here is an example to release the lock of ``Site.title`` column:

.. groonga-command
.. include:: ../../example/reference/commands/lock_release/column.log
.. table_create Site TABLE_HASH_KEY ShortText
.. column_create Site title COLUMN_SCALAR ShortText
.. lock_acquire Site.title
.. lock_release Site.title

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

``lock_release`` command returns whether lock is released successfully
or not::

 [HEADER, SUCCEEDED_OR_NOT]

``HEADER``
""""""""""

See :doc:`/reference/command/output_format` about ``HEADER``.

``SUCCEEDED_OR_NOT``
""""""""""""""""""""

If command succeeded, it returns true, otherwise it returns false on error.

See also
--------

  * :doc:`lock_acquire`
  * :doc:`lock_clear`
