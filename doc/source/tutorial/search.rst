.. -*- rst -*-

.. groonga-include : data.rst

.. groonga-command
.. database: tutorial

Various search conditions
=========================

Groonga supports to narrow down by using syntax like JavaScript, sort by the calculated value. Additionally, Groonga also supports to narrow down & sort search results by using location information (latitude & longitude).

Narrow down & Full-text search by using syntax like JavaScript
--------------------------------------------------------------

The ``filter`` parameter of ``select`` command accepts the search condition.
There is one difference between ``filter`` parameter and ``query`` parameter, you need to specify the condition by syntax like JavaScript for ``filter`` parameter.

.. groonga-command
.. include:: ../example/tutorial/search-1.log
.. select --table Site --filter "_id <= 1" --output_columns _id,_key

See the detail of above query. Here is the condition which is specified as ``filter`` parameter::

  _id <= 1

In this case, this query returns the records which meets the condition that the value of ``_id`` is equal to or less than 1.

Moreover, you can use ``&&`` for AND search, ``||`` for OR search.

.. groonga-command
.. include:: ../example/tutorial/search-2.log
.. select --table Site --filter "_id >= 4 && _id <= 6" --output_columns _id,_key
.. select --table Site --filter "_id <= 2 || _id >= 7" --output_columns _id,_key

If you specify ``query`` parameter and ``filter`` parameter at the same time, you can get the records which meets both of the condition as a result.

Sort by using ``scorer``
------------------------

``select`` command accepts ``scorer`` parameter which is used to process each record of full-text search results.

This parameter accepts the conditions which is specified by syntax like JavaScript as same as ``filter`` parameter.

.. groonga-command
.. include:: ../example/tutorial/search-3.log
.. select --table Site --filter "true" --scorer "_score = rand()" --output_columns _id,_key,_score --sort_keys _score
.. select --table Site --filter "true" --scorer "_score = rand()" --output_columns _id,_key,_score --sort_keys _score

'_score' is one of a pseudo column. The score of full-text search is assigned to it.
See :doc:`/reference/columns/pseudo` about '_score' column.

In the above query, the condition of ``scorer`` parameter is::

  _score = rand()

In this case, the score of full-text search is overwritten by the value of rand() function.

The condition of ``sort_keys`` parameter is::

  _score

This means that sorting the search result by ascending order.

As a result, the order of search result is randomized.

Narrow down & sort by using location information
------------------------------------------------

Groonga supports to store location information (Longitude & Latitude) and not only narrow down but also sort by using it.

Groonga supports two kind of column types to store location information. One is ``TokyoGeoPoint``, the other is ``WGS84GeoPoint``. ``TokyoGeoPoint`` is used for Japan geodetic system. ``WGS84GeoPoint`` is used for world geodetic system.

Specify longitude and latitude as follows:

* "[latitude in milliseconds]x[longitude in milliseconds]"（e.g.: "128452975x503157902"）
* "[latitude in milliseconds],[longitude in milliseconds]"（e.g.: "128452975,503157902"）
* "[latitude in degrees]x[longitude in degrees]"（e.g.: "35.6813819x139.7660839"）
* "[latitude in degrees],[longitude in degrees]"（e.g.: "35.6813819,139.7660839"）

Let's store two location information about station in Japan by WGS. One is Tokyo station, the other is Shinjyuku station. Both of them are station in Japan. The latitude of Tokyo station is 35 degrees 40 minutes 52.975 seconds, the longitude of Tokyo station is 139 degrees 45 minutes 57.902 seconds. The latitude of Shinjyuku station is  35 degrees 41 minutes  27.316 seconds, the longitude of Shinjyuku station is 139 degrees 42 minutes 0.929 seconds. Thus, location information in milliseconds are "128452975x503157902" and "128487316x502920929" respectively. location information in degrees are  "35.6813819x139.7660839" and "35.6909211x139.7002581" respectively.

Let's register location information in milliseconds.

.. groonga-command
.. include:: ../example/tutorial/search-4.log
.. column_create --table Site --name location --type WGS84GeoPoint
.. load --table Site
.. [
..  {"_key":"http://example.org/","location":"128452975x503157902"}
..  {"_key":"http://example.net/","location":"128487316x502920929"},
.. ]
.. select --table Site --query "_id:1 OR _id:2" --output_columns _key,location

Then assign the value of geo distance which is calculated by :doc:`/reference/functions/geo_distance` function to ``scorer`` parameter.

Let's show geo distance from Akihabara station in Japan. In world geodetic system, the latitude of Akihabara station is  35 degrees 41 minutes 55.259 seconds, the longitude of Akihabara station is 139 degrees 46 minutes 27.188 seconds. Specify "128515259x503187188" for geo_distance function.

.. groonga-command
.. include:: ../example/tutorial/search-5.log
.. select --table Site --query "_id:1 OR _id:2" --output_columns _key,location,_score --scorer '_score = geo_distance(location, "128515259x503187188")'

As you can see, the geo distance between Tokyo station and Akihabara station is 2054 meters, the geo distance between Akihabara station and Shinjyuku station is 6720 meters.

The return value of geo_distance function is also used for sorting by specifying pseudo ``_score`` column to ``sort_keys`` parameter.

.. groonga-command
.. include:: ../example/tutorial/search-6.log
.. select --table Site --query "_id:1 OR _id:2" --output_columns _key,location,_score --scorer '_score = geo_distance(location, "128515259x503187188")' --sort_keys -_score

Groonga also supports to narrow down by "a certain point within specified meters".

In such a case, use :doc:`/reference/functions/geo_in_circle` function in ``filter`` parameter.

For example, search the records which exists within 5000 meters from Akihabara station.

.. groonga-command
.. include:: ../example/tutorial/search-7.log
.. select --table Site --output_columns _key,location --filter 'geo_in_circle(location, "128515259x503187188", 5000)'

There is :doc:`/reference/functions/geo_in_rectangle` function which is used to search a certain point within specified region.
