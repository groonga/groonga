.. -*- rst -*-

.. highlightlang:: none

.. groonga-command
.. database: in_values

in_values
=========

Summary
-------

``in_values`` function is added since Groonga 4.0.7.

``in_values`` enables you to simplify the query which uses multiple ``OR`` or ``==``. It is recommended to use this function in point of view about performance improvements in such a case.

Syntax
------

``query`` requires two or more arguments - ``target_value`` and multiple ``value``.

::

  in_values(target_value, value1, ..., valueN)

Usage
-----

Parameters
----------

Return value
------------


