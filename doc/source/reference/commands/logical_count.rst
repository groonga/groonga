.. -*- rst -*-

.. highlightlang:: none

.. groonga-command
.. database: logical_count

``logical_count``
=================

Summary
-------

.. note::

  ``logical_count`` command is an experimental feature.

.. versionadded:: 5.0.0

``logical_count`` is a command to count matched records even though actual records are stored into parted tables. It is useful for users because there is less need to care about maximum records of table :doc:`/limitations`.

Note that this feature is not matured yet, so there are some limitations.

* Create parted tables which contains "_YYYYMMDD" postfix. It is hardcoded, so you must create tables by each day.
* Load proper data into parted tables on your own.

Syntax
------

Usage
-----

Register ``sharding`` plugin to use ``logical_count`` command in advance.

Note that ``logical_count`` is implemented as an experimental plugin, and the specification may be changed in the future.

Parameters
----------

Return value
------------

