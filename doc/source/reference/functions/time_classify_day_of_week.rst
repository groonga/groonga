.. -*- rst -*-

.. groonga-command
.. database: functions_time_classify_day_of_week

``time_classify_day_of_week``
=============================

Summary
-------

.. versionadded:: 8.0.5

It returns the day of the week of the given time as a ``UInt8``
value. ``0`` is Sunday. ``6`` is Saturday.

To enable this function, register ``functions/time`` plugin by the
following command::

  plugin_register functions/time

.. _time-classify-day-of-the-week-syntax:

Syntax
------

This function has only one parameter::

  time_classify_day_of_week(time)

Usage
-----

You need to register ``functions/time`` plugin at first:

.. groonga-command
.. include:: ../../example/reference/functions/time_classify_day_of_week/usage_register.log
.. plugin_register functions/time

Here is a schema definition and sample data.

Sample schema:

.. groonga-command
.. include:: ../../example/reference/functions/time_classify_day_of_week/usage_setup_schema.log
.. table_create  Memos TABLE_HASH_KEY ShortText
.. column_create Memos created_at COLUMN_SCALAR Time

Sample data:

.. groonga-command
.. include:: ../../example/reference/functions/time_classify_day_of_week/usage_setup_data.log
.. load --table Memos
.. [
.. {"_key": "Sunday",    "created_at": "2018-07-01 00:00:00"},
.. {"_key": "Monday",    "created_at": "2018-07-02 00:00:00"},
.. {"_key": "Tuesday",   "created_at": "2018-07-03 00:00:00"},
.. {"_key": "Wednesday", "created_at": "2018-07-04 00:00:00"},
.. {"_key": "Thursday",  "created_at": "2018-07-05 00:00:00"},
.. {"_key": "Friday",    "created_at": "2018-07-06 00:00:00"},
.. {"_key": "Saturday",  "created_at": "2018-07-07 00:00:00"}
.. ]

Here is a simple usage of ``time_classify_day_of_week``:

.. groonga-command
.. include:: ../../example/reference/functions/time_classify_day_of_week/usage_classify.log
.. select \
..   --table Memos \
..   --output_columns '_key, time_classify_day_of_week(created_at)'

It returns ``0`` for Sunday, ``1`` for Monday, ... and ``6`` for
Saturday.

Return value
------------

The day of the week as ``UInt8``. Here are available values:

.. list-table::
   :header-rows: 1

   * - Value
     - The day of the week
   * - ``0``
     - Sunday
   * - ``1``
     - Monday
   * - ``2``
     - Tuesday
   * - ``3``
     - Wednesday
   * - ``4``
     - Thursday
   * - ``5``
     - Friday
   * - ``6``
     - Saturday
