.. -*- rst -*-

.. groonga-command
.. database: functions_geo_distance


``geo_distance``
================

Summary
-------

``geo_distance`` calculates the value of distance between specified two points.

Syntax
------

``geo_distance`` requires two point.
The parameter ``approximate_type`` is optional::

    geo_distance(point1, point2)
    geo_distance(point1, point2, approximate_type)

The default value of ``approximate_type`` is ``"rectangle"``.
If you omit ``approximate_type``, ``geo_distance`` calculates the value of
distance as if ``"rectangle"`` was specified.

Usage
-----

``geo_distance`` is one of the Groonga builtin functions.

You can call a builtin function in :doc:`/reference/grn_expr`

``geo_distance`` function calculates the value of distance (approximate value)
between the coordinate of ``point1`` and the coordinate of ``point2``.

.. note::

    Groonga provides three built in functions for calculating the value of distance.
    There are ``geo_distance()``, ``geo_distance2()`` and ``geo_distance3()``.
    The difference of them is the algorithm of calculating distance.
    ``geo_distance2()`` and ``geo_distance3()`` were deprecated since version 1.2.9.
    Use ``geo_distance(point1, point2, "sphere")`` instead of ``geo_distance2(point1, point2)``.
    Use ``geo_distance(point1, point2, "ellipsoid")`` instead of ``geo_distance3(point1, point2)``.

Lets's learn about ``geo_distance`` usage with examples.
This section shows simple usages.

Here are two schema definition and sample data to show the difference according to the usage.
Those samples show how to calculate the value of distance between New York City and London.

#. Using the column value of location for calculating the distance (``Cities`` table)
#. Using the explicitly specified coordinates for calculating the distance (``Geo`` table)

Using the column value of location
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Here are a schema definition of ``Cities`` table and sample data to show usage.

::

  table_create Cities TABLE_HASH_KEY ShortText
  column_create Cities location COLUMN_SCALAR WGS84GeoPoint
  load --table Cities
  [
  ["_key", "location"],
  ["New York City", "146566000x-266422000"]
  ]

.. groonga-command
.. include:: ../../example/reference/functions/geo_distance_setup_location.log
.. table_create Cities TABLE_HASH_KEY ShortText
.. column_create Cities location COLUMN_SCALAR WGS84GeoPoint
.. load --table Cities
.. [
.. ["_key", "location"],
.. ["New York City", "146566000x-266422000"]
.. ]


This execution example creates a table named ``Cities`` which has one column named ``location``.
``location`` column stores the value of coordinate.
The coordinate of Tokyo is stored as sample data.

.. groonga-command
.. include:: ../../example/reference/functions/geo_distance_location_rectangle.log
.. select Cities --output_columns _score --filter 1 --scorer '_score = geo_distance(location, "185428000x-461000", "rectangle")'

This sample shows that ``geo_distance`` use the value of ``location`` column
and the value of coordinate to calculate distance.

The value ("185428000x-461000") passed to ``geo_distance`` as the second argument is
the coordinate of London.


Using the explicitly specified value of location
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Here are a schema definition of ``Geo`` table and sample data to show usage.

::

 table_create Geo TABLE_HASH_KEY ShortText
 column_create Geo distance COLUMN_SCALAR Int32
 load --table Geo
 [
   {
     "_key": "the record for geo_distance() result"
   }
 ]


.. groonga-command
.. include:: ../../example/reference/functions/geo_distance_setup_distance.log
.. table_create Geo TABLE_HASH_KEY ShortText
.. column_create Geo distance COLUMN_SCALAR Int32
.. load --table Geo
.. [
..   {
..     "_key": "the record for geo_distance() result"
..   }
.. ]

This execution example creates a table named ``Geo`` which has one column named ``distance``.
``distance`` column stores the value of distance.

.. groonga-command
.. include:: ../../example/reference/functions/geo_distance_distance_rectangle.log
.. select Geo --output_columns distance --scorer 'distance = geo_distance("146566000x-266422000", "185428000x-461000", "rectangle")'

This sample shows that ``geo_distance`` use the coordinate of London
and the coordinate of New York to calculate distance.

Parameters
----------

Required parameters
^^^^^^^^^^^^^^^^^^^

There are two required parameter, ``point1`` and ``point2``.

``point1``
""""""""""

Specifies the start point that
you want to calculate the value of distance between two points.

You can specify the value of GeoPoint type. [#]_

See :doc:`/reference/types` about GeoPoint.

``point2``
""""""""""

Specifies the end point that
you want to calculate the value of distance between two points.

You can specify the value of GeoPoint type or
the string indicating the coordinate.

See :doc:`/reference/types` about GeoPoint and the coordinate.

Optional parameter
^^^^^^^^^^^^^^^^^^

There is a optional parameter, ``approximate_type``.

``approximate_type``
""""""""""""""""""""

Specifies how to approximate the geographical features for calculating
the value of distance.

You can specify the value of ``approximate_type`` by one of the followings.

  * ``rectangle``
  * ``sphere``
  * ``ellipsoid``

.. note::

    There is a limitation about ``geo_distance``. ``geo_distance`` can not
    calculate the value of distance between two points across meridian,
    equator or the date line if you use ``sphere`` or ``ellipsoid`` as
    approximate type. There is not such a limitation for ``rectangle``.
    This is temporary limitation according to the implementation of Groonga,
    but it will be fixed in the future release.

``rectangle``
...............

This parameter require to approximate the geographical features
by square approximation for calculating the distance.

Since the value of distance is calculated by simple formula,
you can calculate the value of distance fast.
But, the error of distance increases as it approaches the pole.

You can also specify ``rect`` as abbrev expression.

Here is a sample about calculating the value of distance with column value.

.. include:: ../../example/reference/functions/geo_distance_location_rectangle.log

Here is a sample about calculating the value of distance with explicitly specified point.

.. include:: ../../example/reference/functions/geo_distance_distance_rectangle.log

Here are samples about calculating the value of distance with explicitly specified point across meridian, equator, the date line.

.. groonga-command
.. include:: ../../example/reference/functions/geo_distance_distance_rectangle_across_meridian.log
.. select Geo --output_columns distance --scorer 'distance = geo_distance("175904000x8464000", "145508000x-13291000", "rectangle")'

This sample shows the value of distance across meridian.
The return value of ``geo_distance("175904000x8464000", "145508000x-13291000", "rectangle")`` is the value of distance from Paris, Flance to Madrid, Spain.

.. groonga-command
.. include:: ../../example/reference/functions/geo_distance_distance_rectangle_across_equator.log
.. select Geo --output_columns distance --scorer 'distance = geo_distance("146566000x-266422000", "-56880000x-172310000", "rectangle")'

This sample shows the value of distance across equator.
The return value of ``geo_distance("146566000x-266422000", "-56880000x-172310000", "rectangle")`` is the value of distance from New York, The United Status to Brasillia, Brasil.

.. groonga-command
.. include:: ../../example/reference/functions/geo_distance_distance_rectangle_across_the_date_line.log
.. select Geo --output_columns distance --scorer 'distance = geo_distance("143660000x419009000", "135960000x-440760000", "rectangle")'

This sample shows the value of distance across the date line.
The return value of ``geo_distance("143660000x419009000", "135960000x-440760000", "rectangle")`` is the value of distance from Beijin, China to San Francisco, The United States.

.. note::

    ``geo_distance`` uses square approximation as default. If you omit ``approximate_type``, ``geo_distance`` behaves like ``rectangle`` was specified.

.. note::

    ``geo_distance`` accepts the string indicating the coordinate as
    the value of ``point1`` when the value of ``approximate_type`` is
    ``"rectangle"``.
    If you specified the string indicating the coordinate as the value
    of ``point1`` with ``sphere`` or ``ellipsoid``, ``geo_distance``
    returns 0 as the value of distance.


``sphere``
...............

This parameter require to approximate the geographical features
by spherical approximation for calculating the distance.

It is slower than ``rectangle``, but the error of distance becomes
smaller than ``rectangle``.

You can also specify ``sphr`` as abbrev expression.

Here is a sample about calculating the value of distance with column value.

.. groonga-command
.. include:: ../../example/reference/functions/geo_distance_location_sphere.log
.. select Cities --output_columns _score --filter 1 --scorer '_score = geo_distance(location, "185428000x-461000", "sphere")'

``ellipsoid``
.............

This parameter require to approximate the geographical features
by ellipsoid approximation for calculating the distance.

It uses the calculation of distance by the formula of Hubeny.
It is slower than ``sphere``, but the error of distance becomes
smaller than ``sphere``.

You can also specify ``ellip`` as abbrev expression.

Here is a sample about calculating the value of distance with column value.

.. groonga-command
.. include:: ../../example/reference/functions/geo_distance_location_ellipsoid.log
.. select Cities --output_columns _score --filter 1 --scorer '_score = geo_distance(location, "185428000x-461000", "ellipsoid")'

Return value
------------

``geo_distance`` returns the value of distance in float type.
The unit of return value is meter.

.. rubric:: Footnote

.. [#] You can specify whether TokyoGeoPoint or WGS84GeoPoint.
