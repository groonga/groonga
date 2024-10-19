.. -*- rst -*-

.. groonga-command
.. database: columns_scalar

Scalar column
=============

Summary
-------

Scalar column is a data store object.

It can store one scalar value per record. Scalar value is one of the
following type values:

* ``Bool``
* ``Int8``
* ``Int16``
* ``Int32``
* ``Int64``
* ``UInt8``
* ``UInt16``
* ``UInt32``
* ``UInt64``
* ``BFloat16``
* ``Float32``
* ``Float``
* ``Time``
* ``ShortText``
* ``Text``
* ``LongText``
* ``TokyoGeoPoint``
* ``WGS84GeoPoint``

If you want to store zero or more values per record, you can use
multiple scalar columns or one :doc:`vector`. If these values are
related and use the same types such as tags (zero or more strings),
:doc:`vector` is suitable. If these values aren't related such as
title and location, multiple scalar columns are suitable.

Usage
-----

See :doc:`../commands/column_create` how to create a column.

You can generate a column value from other column's value
automatically. It's called as a generated column. See
:ref:`column-create-generated-column` how to create a generated
column.

See also
--------

* :doc:`vector`
* :doc:`../commands/column_create`
