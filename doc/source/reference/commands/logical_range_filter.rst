.. -*- rst -*-

.. highlightlang:: none

.. groonga-command
.. database: logical_range_filter

``logical_range_filter``
========================

Summary
-------

.. versionadded:: 5.0.0

TODO: Write summary

Syntax
------

This command takes many parameters.

The required parameters are ``logical_table`` and ``shard_key``::

  logical_range_filter
    logical_table
    shard_key
    [min=null]
    [min_border=null]
    [max=null]
    [max_border=null]
    [order=ascending]
    [filter=null]
    [offset=0]
    [limit=10]
    [output_columns=_key,*]
    [use_range_index=null]

There are some parameters that can be only used as named
parameters. You can't use these parameters as ordered parameters. You
must specify parameter name.

Here are parameters that can be only used as named parameters:

  * ``cache=no``

Usage
-----

Register ``sharding`` plugin to use ``logical_range_filter`` command in advance.

.. groonga-command
.. plugin_register sharding

TODO: Add examples

Parameters
----------

This section describes parameters of ``logical_range_filter``.

Required parameters
^^^^^^^^^^^^^^^^^^^

There are required parameters, ``logical_table`` and ``shard_key``.

``logical_table``
"""""""""""""""""

Specifies logical table name. It means table name without "_YYYYMMDD" postfix.
If you use actual table such as "Logs_20150203", "Logs_20150203" and so on, logical table name is "Logs".

TODO: Add examples

``shard_key``
"""""""""""""

Specifies column name which is treated as shared key in each parted table.

TODO: Add examples

Optional parameters
^^^^^^^^^^^^^^^^^^^

There are optional parameters.

``min``
"""""""

Specifies the min value of ``shard_key``

TODO: Add examples

``min_border``
""""""""""""""

Specifies whether the min value of borderline must be include or not.
Specify ``include`` or ``exclude`` as the value of this parameter.

TODO: Add examples

``max``
"""""""

Specifies the max value of ``shard_key``.

TODO: Add examples

``max_border``
""""""""""""""

Specifies whether the max value of borderline must be include or not.
Specify ``include`` or ``exclude`` as the value of this parameter.

TODO: Add examples

``order``
""""""""""

TODO

``filter``
""""""""""

TODO

``offset``
""""""""""

TODO

``limit``
"""""""""

TODO

``output_columns``
""""""""""""""""""

TODO

``use_range_index``
"""""""""""""""""""

Specifies whether range_index is used or not.
Note that it's a parameter for test. It should not be used for production.

TODO: Add examples

Cache related parameter
^^^^^^^^^^^^^^^^^^^^^^^

.. _logical-range-filter-cache:

``cache``
"""""""""

Specifies whether caching the result of this query or not.

If the result of this query is cached, the next same query returns
response quickly by using the cache.

It doesn't control whether existing cached result is used or not.

Here are available values:

.. list-table::
   :header-rows: 1

   * - Value
     - Description
   * - ``no``
     - Don't cache the output of this query.
   * - ``yes``
     - Cache the output of this query.
       It's the default value.

TODO: Add examples

.. Here is an example to disable caching the result of this query:

.. .. groonga-command
.. .. include:: ../../example/reference/commands/logical_range_filter/cache_no.log
.. .. logical_range_filter ... --cache no

The default value is ``yes``.

Return value
------------

TODO

::

  [HEADER, LOGICAL_FILTERED]
