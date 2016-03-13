.. -*- rst -*-

.. highlightlang:: none

.. groonga-command
.. database: logical_table_remove

``logical_table_remove``
========================

Summary
-------

.. note::

   This command is an experimental feature.

.. versionadded:: 5.0.5

TODO

Syntax
------

This command takes many parameters.

The required parameters are ``logical_table`` and ``shard_key``::

  logical_table_remove logical_table
                       shard_key
                       [min]
                       [min_border]
                       [max]
                       [max_border]

Usage
-----

TODO

Parameters
----------

This section describes parameters of ``logical_table_remove``.

Required parameters
^^^^^^^^^^^^^^^^^^^

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

Return value
------------

TODO

::

  [HEADER, true]
