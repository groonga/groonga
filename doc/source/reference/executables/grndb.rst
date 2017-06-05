.. -*- rst -*-

.. highlightlang:: none

``grndb``
=========

Summary
-------

.. versionadded:: 4.0.9

``grndb`` manages a Groonga database.

Here are features:

  * Checks whether database is broken or not.
  * Recovers broken database automatically if the database is
    recoverable.

Syntax
------

``grndb`` requires command and database path::

  grndb COMMAND [OPTIONS] DATABASE_PATH

Here are available commands:

  * ``check`` - Checks whether database is broken or not.
  * ``recover`` - Recovers database.

Usage
-----

Here is an example to check the database at ``/var/lib/groonga/db/db``::

  % grndb check /var/lib/groonga/db/db

Here is an example to recover the database at ``/var/lib/groonga/db/db``::

  % grndb recover /var/lib/groonga/db/db

Commands
--------

This section describes available commands.

``check``
^^^^^^^^^

It checks an existing Groonga database. If the database is broken,
``grndb`` reports reasons and exits with non-``0`` exit status.

.. note::

   You must not use this command for opened database. If the database
   is opened, this command may report wrong result.

``check`` has some options.

``--target``
""""""""""""

.. versionadded:: 5.1.2

It specifies a check target object.

If your database is large and you know an unreliable object, this
option will help you. ``check`` need more time for large database. You
can reduce check time by ``--target`` option to reduce check target.

The check target is checked recursive. Because related objects of
unreliable object will be unreliable.

If the check target is a table, all columns of the table are also
checked recursive.

If the check target is a table and its key type is another table, the
another table is also checked recursive.

If the check target is a column and its value type is a table, the
table is also checked recursive.

If the check target is an index column, the table specified as value
type and all sources are also checked recursive.

Here is an example that checks only ``Entries`` table and its
columns::

  % grndb check --target Entries /var/lib/groonga/db/db

Here is an example that checks only ``Entries.name`` column::

  % grndb check --target Entries.name /var/lib/groonga/db/db

``--log-path``
""""""""""""""

.. versionadded:: 7.0.4

It specifies a path of ``grndb`` log.

Here is an example that specifies ``--log-path`` option::

  % grndb check --log-path /var/log/groonga/grndb.log /var/lib/groonga/db/db


``recover``
^^^^^^^^^^^

It recovers an existing broken Groonga database.

If the database is not broken, ``grndb`` does nothing and exits with
``0`` exit status.

If the database is broken and one or more index columns are only
broken, ``grndb`` recovers these index columns and exists with ``0``
exit status. It may take a long time for large indexed data.

If the database is broken and tables or data columns are broken,
``grndb`` reports broken reasons and exits with non-``0`` exit
status. You can know whether the database is recoverable or not by
``check`` command.

.. note::

   You must not use this command for opened database. If the database
   is opened, this command may break the database.

``recover`` has some options.

``--log-path``
""""""""""""""

.. versionadded:: 7.0.4

It specifies a path of ``grndb`` log.

Here is an example that specifies ``--log-path`` option::

  % grndb recover --log-path /var/log/groonga/grndb.log /var/lib/groonga/db/db

