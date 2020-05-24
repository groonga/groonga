.. -*- rst -*-

.. highlightlang:: none

.. groonga-command
.. database: math_abs

``math_abs``
===============

Summary
-------

.. versionadded:: 7.0.4

``math_abs`` returns the absolute value of value.

To enable this function, register ``functions/math`` plugin by following the command::

  plugin_register functions/math

Syntax
------

``math_abs`` requires one argument - ``target``.

::

  math_abs(target)

Usage
-----

Here is a schema definition and sample data.

Sample schema:

.. groonga-command
.. plugin_register functions/math
.. include:: ../../example/reference/functions/math_abs/usage_setup_schema.log
.. table_create Shops TABLE_HASH_KEY ShortText
.. column_create Shops from_station COLUMN_SCALAR Int32
.. column_create Shops from_office COLUMN_SCALAR Int32

Sample data:

.. groonga-command
.. include:: ../../example/reference/functions/math_abs/usage_setup_data.log
.. load --table Shops
.. [
.. {"_key": "Coffee Shop",         "from_station":  50},
.. {"_key": "Donut & Coffee Shop", "from_station": 400},
.. {"_key": "Cake & Coffee Shop",  "from_station": 200}
.. ]

Here is the simple usage of ``math_abs`` function which returns nearest shops from office.

To detect nearest shops, we need to calculate distance.
If the distance of your office from station is 250 meters, you can calculate it by ``math_abs(250 - from_station)``.

.. groonga-command
.. include:: ../../example/reference/functions/math_abs/nearest_shops.log
.. select Shops --filter true --output_columns '_key, from_office' --scorer 'from_office = math_abs(250 - from_station)' --sort_keys from_office

By specifying ``--sort_keys from_office``, you can show nearest shops by ascending order.

Parameters
----------

There is only one required parameter.

``target``
^^^^^^^^^^

Specifies a column of table that is specified by ``table`` parameter in ``select``.

Return value
------------

``math_abs`` returns the absolute value of target column value.
