.. -*- rst -*-

Geolocation search
==================

Groonga supports geolocation search. It uses index for
search. It means that you can search by geolocation fast
like fulltext search.

Supported features
------------------

Groonga supports only point as data type. Line, surface and
so on aren't supported yet. Here is a feature list:

#. Groonga can store a point to a column.
#. Groonga can search records that have a point in the specified rectangle.
#. Groonga can search records that have a point in the specified circle.
#. Groonga can calculate distance between two points.
#. Groonga can sort records by distance from the specified
   point in ascending order.

Here are use cases for Groonga's geolocation search:

* You list McDonald's around a station.
* You list KFC around the current location sort by distance
  from the current location in ascending order with distance.

Here are not use cases:

* You search McDonald's in a city. (Groonga doesn't support
  geolocation search by a shape except a rectangle and a
  circle.)
* You store a region instead of a point as a lake
  record. (A column can't has geolocation data except a
  point.)

The following figures show about Groonga's geolocation
search features.

Here is a figure that only has records. A black point
describes a record. The following figures shows how records
are treated.

.. image:: /images/geo-points.png
   :alt: only records


Coming soon...

