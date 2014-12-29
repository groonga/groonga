.. -*- rst -*-

.. highlightlang:: none

``grndb``
=========

Summary
-------

.. note::

   This executable command is an experimental feature.

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

  $ grndb check /var/lib/groonga/db/db

Here is an example to recover the database at ``/var/lib/groonga/db/db``::

  $ grndb recover /var/lib/groonga/db/db

Commands
--------

This section describes available commands.

``check``
"""""""""

It checks an existing Groonga database. If the database is broken,
``grndb`` reports reasons and exits with non-``0`` exit status.

.. note::

   You must not use this command for opened database. If the database
   is opened, this command may report wrong result.

``recover``
"""""""""""

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
