.. -*- rst -*-

.. highlightlang:: none

.. groonga-command
.. database: logical_range_filter

``logical_range_filter``
========================

Summary
-------

.. note::

   This command is an experimental feature.

.. versionadded:: 5.0.0

TODO: Write summary

Syntax
------

The required parameters are ``logical_table`` and ``shard_key``::

  logical_range_filter
                logical_table
                shard_key
                [min]
                [min_border]
                [max]
                [max_border]
                [order]
                [filter]
                [offset]
                [output_columns]
                [use_range_index]

Usage
-----

Register ``sharding`` plugin to use ``logical_range_filter`` command in advance.

.. groonga-command
.. register sharding

Parameters
----------

This section describes parameters of ``logical_range_filter``.

Required parameter
^^^^^^^^^^^^^^^^^^

There are required parameters, ``logical_table`` and ``shard_key``.

``logical_table``
"""""""""""""""""

Specifies logical table name. It means table name without "_YYYYMMDD" postfix.
If you use actual table such as "Logs_20150203", "Logs_20150203" and so on, logical table name is "Logs".

``shard_key``
"""""""""""""

Specifies column name which is treated as shared key in each parted table.

Optional parameters
^^^^^^^^^^^^^^^^^^^

There are optional parameters.

``min``
"""""""

Specifies the min value of ``shard_key``

``min_border``
""""""""""""""

Specifies whether the min value of borderline must be include or not.
Specify ``include`` or ``exclude`` as the value of this parameter.

``max``
"""""""

Specifies the max value of ``shard_key``.

``max_border``
""""""""""""""

Specifies whether the max value of borderline must be include or not.
Specify ``include`` or ``exclude`` as the value of this parameter.

``order``
""""""""""

``filter``
""""""""""

``offset``
""""""""""

``output_columns``
""""""""""""""""""

``use_range_index``
"""""""""""""""""""

Specifies whether range_index is used or not.
Note that it's a parameter for test. It should not be used for production.

Return value
------------

::

  [HEADER, LOGICAL_FILTERED]
