.. -*- rst -*-

.. highlightlang:: none

.. groonga-command
.. database: functions_number_classify

``number_classify``
===================

.. versionadded:: 6.0.3

Summary
-------

``number_classify`` classifies specific range of value as same value.
It can group values by specific interval. The value of interval is
customized in function parameter.

..

Syntax
------

``number_classify`` requires two parameters::

  number_classify(number, casted_interval)

Usage
-----

Here are a schema definition and sample data to show usage.

.. groonga-command
.. include:: ../../example/reference/functions/number_classify/usage_setup.log
.. plugin_register functions/number
.. table_create Prices TABLE_PAT_KEY Int32
.. load --table Prices
.. [
.. {"_key": 0},
.. {"_key": 1},
.. {"_key": 99},
.. {"_key": 100},
.. {"_key": 101},
.. {"_key": 199},
.. {"_key": 200},
.. {"_key": 201}
.. ]

``number_classify`` can be used in ``--output_columns`` or ``--filter`` in
:doc:`/reference/commands/select`.

The following example uses ``--filter 'number_classify(_key, 100) == 100``.

.. groonga-command
.. include:: ../../example/reference/functions/functions/number_classify/usage_basic.log
.. select Prices --sort_keys _id --output_columns '_id, _key, number_classify(_key, 100)' --filter 'number_classify(_key, 100) == 100'

In this case, as interval is set to 100, the value between 100, 101
and 199 are normalized to 100. As a result, above query returns result
that the return value of ``number_classify(_key, 100)`` is matched
to 100.

You can filter values easily in same specific range of prices.

Parameters
----------

This section describes all parameters.

Required parameters
^^^^^^^^^^^^^^^^^^^

There are two required parameters.

``number``
""""""""""

The column that column data type is numerical.

``casted_interval``
"""""""""""""""""""

The value of interval which is regarded in same range of value.

Optional parameters
^^^^^^^^^^^^^^^^^^^

There is no optional parameter.

Return value
------------

``number_classify`` returns a normalized value in every specified value.

See also
--------

* :doc:`/reference/commands/select`
