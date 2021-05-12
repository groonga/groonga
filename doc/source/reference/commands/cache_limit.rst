.. -*- rst -*-
.. Groonga Project

.. groonga-command
.. database: commands_cache_limit

``cache_limit``
===============

Summary
-------

``cache_limit`` gets or sets the max number of query cache
entries. Query cache is used only by :doc:`select` command.

If the max number of query cache entries is 100, the recent 100
``select`` commands are only cached. The cache expire algorithm is LRU
(least recently used).

Syntax
------

This command takes only one optional parameter::

  cache_limit [max=null]

Usage
-----

You can get the current max number of cache entries by executing
``cache_limit`` without parameter.

.. groonga-command
.. include:: ../../example/reference/commands/cache_limit/get.log
.. cache_limit

You can set the max number of cache entries by executing ``cache_limit``
with ``max`` parameter.

Here is an example that sets ``10`` as the max number of cache
entries.

.. groonga-command
.. include:: ../../example/reference/commands/cache_limit/set.log
.. cache_limit 10
.. cache_limit

If ``max`` parameter is used, the return value is the max number of
cache entries before ``max`` parameter is set.


Parameters
----------

This section describes all parameters.

``max``
"""""""

Specifies the max number of query cache entries as a number.

If ``max`` parameter isn't specified, the current max number of query
cache entries isn't changed. ``cache_limit`` just returns the current
max number of query cache entries.

Return value
------------

``cache_limit`` returns the current max number of query cache entries::

  [HEADER, N_ENTRIES]

``HEADER``

  See :doc:`/reference/command/output_format` about ``HEADER``.

``N_ENTRIES``

  ``N_ENTRIES`` is the current max number of query cache entries. It
  is a number.

See also
--------

* :doc:`select`
