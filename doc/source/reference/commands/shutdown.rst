.. -*- rst -*-

.. groonga-command
.. database: commands_table_shutdown

``shutdown``
============

Summary
-------

``shutdown`` stops the Groonga server process.

``shutdown`` uses graceful shutdown by default. If there are some
running commands, the Groonga server process stops after these running
commands are finished. New command requests aren't processed after
``shutdown`` command is executed.

.. versionadded:: 6.0.1

   ``shutdown`` uses immediate shutdown by specifying ``immediate`` to
   ``mode`` parameter. The Groonga server process stops immediately
   even when there are some running commands.

   .. note::

      You need to set :doc:`/reference/command/request_id` to all
      requests to use immediate shutdown.

.. versionadded:: 9.1.2

   The Groonga HTTP server accepts immediate shutdown immediately even when all threads are used.

   .. note::

     This feature can only use on the Groonga HTTP server.

Syntax
------

This command takes only one optional parameter::

  shutdown [mode=graceful]

Usage
-----

``shutdown`` use graceful shutdown by default:

.. groonga-command
.. include:: ../../example/reference/commands/shutdown/default.log
.. shutdown

You can specify ``graceful`` to ``mode`` parameter explicitly:

.. groonga-command
.. database: commands_table_shutdown
.. include:: ../../example/reference/commands/shutdown/graceful.log
.. shutdown --mode graceful

You can choose immediate shutdown by specifying ``immediate`` to
``mode`` parameter:

.. groonga-command
.. database: commands_table_shutdown
.. include:: ../../example/reference/commands/shutdown/immediate.log
.. shutdown --mode immediate

Immediate shutdown is useful when you don't have time for graceful
shutdown. For example, Windows kills service that takes long time to
stop on Windows shutdown.

Parameters
----------

This section describes parameters of this command.

Required parameters
^^^^^^^^^^^^^^^^^^^

There is no required parameter.

Optional parameters
^^^^^^^^^^^^^^^^^^^

There are optional parameters.

``mode``
""""""""

Specifies shutdown mode. Here are available shutdown modes:

.. list-table::
   :header-rows: 1

   * - Value
     - Description
   * - ``graceful``
     - Stops after running commands are finished.

       This is the default.
   * - ``immediate``
     - .. versionadded:: 6.0.1

          Stops immediately even if there are some running commands.

Return value
------------

``shutdown`` returns ``true`` as body when shutdown is
accepted::

  [HEADER, true]

If ``shutdown`` doesn't accept shutdown, error details are in
``HEADER``.

See :doc:`/reference/command/output_format` for ``HEADER``.
