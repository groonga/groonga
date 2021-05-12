.. -*- rst -*-

.. groonga-command
.. database: functions_time_classify_day

``time_classify_day``
=====================

Summary
-------

.. versionadded:: 6.0.3

``time_classify_day`` rounds time to a day unit.

This rounded ``2020-01-30 11:50:11.000000`` and ``2020-01-30 22:50:11.000000`` to ``2020-01-30 00:00:00.000000``.

To enable this function, register ``functions/time`` plugin by the
following command::

  plugin_register functions/time

.. _time-classify-day-syntax:

Syntax
------

This function has only one parameter::

  time_classify_day(time)

Usage
-----

Here are a schema definition and sample data to show usage.

You need to register ``functions/time`` plugin at first:

.. groonga-command
.. include:: ../../example/reference/functions/time_classify_day/usage_register.log
.. plugin_register functions/time

Here is a schema definition and sample data.

Sample schema:

.. groonga-command
.. include:: ../../example/reference/functions/time_classify_day/usage_setup_schema.log
.. table_create Sales TABLE_NO_KEY
.. column_create Sales name COLUMN_SCALAR ShortText
.. column_create Sales price COLUMN_SCALAR UInt32
.. column_create Sales timestamp COLUMN_SCALAR Time

Sample data:

.. groonga-command
.. include:: ../../example/reference/functions/time_classify_day/usage_setup_data.log
.. load --table Sales
.. [
.. {"name": "Apple" , "price": "256", "timestamp": "2020-05-01 11:50:11.000000"},
.. {"name": "Apple" , "price": "256", "timestamp": "2020-05-01 10:20:00.000000"},
.. {"name": "Orange", "price": "122", "timestamp": "2020-05-01 11:44:12.000001"},
.. {"name": "Apple" , "price": "256", "timestamp": "2020-05-01 19:50:23.000020"},
.. {"name": "Banana", "price": "88" , "timestamp": "2020-05-01 11:00:02.000000"},
.. {"name": "Banana", "price": "88" , "timestamp": "2020-05-01 21:34:12.000001"}
.. ]

Here is a simple usage of ``time_classify_day``:

.. groonga-command
.. include:: ../../example/reference/functions/time_classify_day/usage_classify.log
.. select \
..   --table Sales \
..   --output_columns 'name, time_classify_day(timestamp)'

Parameters
----------

Required parameters
^^^^^^^^^^^^^^^^^^^

There is only one required parameter.

``column``
""""""""""

Specify a target column.

Return value
------------

It returns a value that rounded time to a day unit.

The return value is UNIX time.
