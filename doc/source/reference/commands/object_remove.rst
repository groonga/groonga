.. -*- rst -*-

.. groonga-command
.. database: commands_object_remove

``object_remove``
=================

Summary
-------

.. versionadded:: 6.0.0

``object_remove`` removes an object. You can remove any object
including table, column, command and so on. Normally, you should use
specific remove command such as :doc:`table_remove` and
:doc:`column_remove`.

``object_remove`` is danger because you can remove any object. You
should use ``object_remove`` carefully.

``object_remove`` has "force mode". You can remove a broken object by
"force mode". "Force mode" is useful to resolve problems reported by
:doc:`/reference/executables/grndb`.

Syntax
------

This command takes two parameters::

  object_remove name
                [force=no]

Usage
-----

You can remove an object in the database specified by ``name``:

.. groonga-command
.. include:: ../../example/reference/commands/object_remove/usage.log
.. object_remove Users
.. table_create Users TABLE_HASH_KEY ShortText
.. object_remove Users

The ``object_remove Users`` returns ``false`` before you create
``Users`` table.

The ``object_remove Users`` returns ``true`` after you create ``Users``
table.

You can't remove a broken object by default:

.. groonga-command
.. include:: ../../example/reference/commands/object_remove/usage_broken_without_force.log
.. table_create Users TABLE_HASH_KEY ShortText
.. thread_limit 1
.. database_unmap
.. % echo "BROKEN" > ${DB_PATH}.0000100
.. object_remove Users
.. object_exist Users

You can remove a broken object by ``--force yes``:

.. groonga-command
.. include:: ../../example/reference/commands/object_remove/usage_broken_with_force.log
.. object_remove Users --force yes
.. object_exist Users

``--force yes`` means you enable "force mode". You can remove a broken
object in "force mode".

Parameters
----------

This section describes all parameters.

Required parameters
^^^^^^^^^^^^^^^^^^^

There is only one required parameter.

.. _object-remove-name:

``name``
""""""""

Specifies the object name to be removed.

If you want to remove a column, use ``TABLE_NAME.COLUMN_NAME`` format
like the following:

.. groonga-command
.. include:: ../../example/reference/commands/object_remove/name_column.log
.. table_create Logs TABLE_NO_KEY
.. column_create Logs timestamp COLUMN_SCALAR Time
.. object_remove Logs.timestamp

``Logs`` is table name and ``timestamp`` is column name in
``Logs.timestamp``.

Optional parameters
^^^^^^^^^^^^^^^^^^^

There is one optional parameter.

``force``
"""""""""

Specifies whether removing the object in "force mode".

You can't remove a broken object by default. But you can remove a
broken object in "force mode".

``force`` value must be ``yes`` or ``no``. ``yes`` means that "force
mode" is enabled. ``no`` means that "force mode" is disabled.

The default value is ``no``. It means that "force mode" is disabled by
default.

Return value
------------

The command returns ``true`` as body when the command removed the
specified object without any error. For example::

  [HEADER, true]

The command returns ``false`` as body when the command gets any
errors. For example::

  [HEADER, false]

See :doc:`/reference/command/output_format` for ``HEADER``.

Note that ``false`` doesn't mean that "the command can't remove the
object". If you enable "force mode", the command removes the object
even if the object is broken. In the case, the object is removed and
``false`` is returned as body.
