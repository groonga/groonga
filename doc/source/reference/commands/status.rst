.. -*- rst -*-

.. groonga-command
.. database: commands_status

``status``
==========

Summary
-------

``status`` returns the current status of the context that processes
the request.

Context is an unit that processes requests. Normally, context is
created for each thread.

Syntax
------

This command takes no parameters::

  status

.. _status-usage:

Usage
-----

Here is a simple example:

.. groonga-command
.. include:: ../../example/reference/commands/status.log
.. status

It returns the current status of the context that processes the
request. See :ref:`status-return-value` for details.

Parameters
----------

This section describes all parameters.

Required parameters
^^^^^^^^^^^^^^^^^^^

There is no required parameter.

Optional parameters
^^^^^^^^^^^^^^^^^^^

There is no optional parameter.

.. _status-return-value:

Return value
------------

The command returns the current status as an object::

  [
    HEADER,
    {
      "alloc_count": ALLOC_COUNT,
      "cache_hit_rate": CACHE_HIT_RATE,
      "command_version": COMMAND_VERSION,
      "default_command_version": DEFAULT_COMMAND_VERSION,
      "max_command_version": MAX_COMMAND_VERSION,
      "n_queries": N_QUERIES,
      "start_time": START_TIME,
      "starttime": STARTTIME,
      "uptime": UPTIME,
      "version": VERSION
    }
  ]

See :doc:`/reference/command/output_format` for ``HEADER``.

Here are descriptions about values. See :ref:`status-usage` for real
values:

.. list-table::
   :header-rows: 1

   * - Key
     - Description
     - Example
   * - ``alloc_count``
     - The number of allocated memory blocks that aren't freed.  If
       this value is continuously increased, there may be a memory
       leak.
     - ``1400``
   * - ``cache_hit_rate``
     - Percentage of cache used responses in the Groonga process. If
       there are 10 requests and 7 responses are created from cache,
       ``cache_hit_rate`` is ``70.0``. The percentage is computed from
       only requests that use commands that support cache.

       Here are commands that support cache:

         * :doc:`select`
         * :doc:`logical_select`
         * :doc:`logical_range_filter`
         * :doc:`logical_count`

     - ``29.4``
   * - ``command_version``
     - The :doc:`/reference/command/command_version` that is used by
       the context.
     - ``1``
   * - ``default_command_version``
     - The default :doc:`/reference/command/command_version` of the
       Groonga process.
     - ``1``
   * - ``max_command_version``
     - The max :doc:`/reference/command/command_version` of the
       Groonga process.
     - ``2``
   * - ``n_queries``
     - The number of requests processed by the Groonga process. It
       counts only requests that use commands that support cache.

       Here are commands that support cache:

         * :doc:`select`
         * :doc:`logical_select`
         * :doc:`logical_range_filter`
         * :doc:`logical_count`

     - ``29``
   * - ``start_time``
     - .. versionadded:: 5.0.8

       The time that the Groonga process started in UNIX time.
     - ``1441761403``
   * - ``starttime``
     - .. deprecated:: 5.0.8
          Use ``start_time`` instead.
     - ``1441761403``
   * - ``uptime``
     - The elapsed time since the Groonga process started in second.

       For example, ``216639`` means that ``2.5`` (= ``216639 / 60 /
       60 / 24 = 2.507``) days.

     - ``216639``
   * - ``version``
     - The version of the Groonga process.
     - ``5.0.7``
