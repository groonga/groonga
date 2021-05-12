.. -*- rst -*-

.. groonga-include : introduction.rst

.. groonga-command
.. database: tutorial

Various data types
==================

Groonga is a full text search engine but also serves as a column-oriented data store. Groonga supports various data types, such as numeric types, string types, date and time type, longitude and latitude types, etc. This tutorial shows a list of data types and explains how to use them.

Overview
--------

The basic data types of Groonga are roughly divided into 5 groups --- boolean type, numeric types, string types, date/time type and longitude/latitude types. The numeric types are further divided according to whether integer or floating point number, signed or unsigned and the number of bits allocated to each integer. The string types are further divided according to the maximum length. The longitude/latitude types are further divided according to the geographic coordinate system. For more details, see :doc:`/reference/types`.

In addition, Groonga supports reference types and vector types. Reference types are designed for accessing other tables. Vector types are designed for storing a variable number of values in one element.

First, let's create a table for this tutorial.

.. groonga-command
.. include:: ../example/tutorial/data-1.log
.. table_create --name ToyBox --flags TABLE_HASH_KEY --key_type ShortText

Boolean type
------------

The boolean type is used to store true or false. To create a boolean type column, specify Bool to the `type` parameter of :doc:`/reference/commands/column_create` command. The default value of the boolean type is false.

The following example creates a boolean type column and adds three records. Note that the third record has the default value because no value is specified.

.. groonga-command
.. include:: ../example/tutorial/data-2.log
.. column_create --table ToyBox --name is_animal --type Bool
.. load --table ToyBox
.. [
.. {"_key":"Monkey","is_animal":true}
.. {"_key":"Flower","is_animal":false}
.. {"_key":"Block"}
.. ]
.. select --table ToyBox --output_columns _key,is_animal

Numeric types
-------------

The numeric types are divided into integer types and a floating point number type. The integer types are further divided into the signed integer types and unsigned integer types. In addition, you can choose the number of bits allocated to each integer. For more details, see :doc:`/reference/types`. The default value of the numeric types is 0.

The following example creates an Int8 column and a Float column, and then updates existing records. The :doc:`/reference/commands/load` command updates the weight column as expected. On the other hand, the price column values are different from the specified values because 15.9 is not an integer and 200 is too large. 15.9 is converted to 15 by removing the fractional part. 200 causes an overflow and the result becomes -56. Note that the result of an overflow/underflow is undefined.

.. groonga-command
.. include:: ../example/tutorial/data-3.log
.. column_create --table ToyBox --name price --type Int8
.. column_create --table ToyBox --name weight --type Float
.. load --table ToyBox
.. [
.. {"_key":"Monkey","price":15.9}
.. {"_key":"Flower","price":200,"weight":0.13}
.. {"_key":"Block","weight":25.7}
.. ]
.. select --table ToyBox --output_columns _key,price,weight

String types
------------

The string types are divided according to the maximum length. For more details, see :doc:`/reference/types`. The default value is the zero-length string.

The following example creates a ``ShortText`` column and updates
existing records. The third record (``"Block"`` key record) has the
default value (zero-length string) because it's not updated.

.. groonga-command
.. include:: ../example/tutorial/data-4.log
.. column_create --table ToyBox --name name --type ShortText
.. load --table ToyBox
.. [
.. {"_key":"Monkey","name":"Grease"}
.. {"_key":"Flower","name":"Rose"}
.. ]
.. select --table ToyBox --output_columns _key,name

Date and time type
------------------

The date and time type of Groonga is Time. Actually, a Time column stores a date and time as the number of microseconds since the Epoch, 1970-01-01 00:00:00. A Time value can represent a date and time before the Epoch because the actual data type is a signed integer. Note that :doc:`/reference/commands/load` and :doc:`/reference/commands/select` commands use a decimal number to represent a data and time in seconds. The default value is 0.0, which means the Epoch.

.. note::

   Groonga internally holds the value of Epoch as pair of integer. The first integer represents the value of seconds, on the other hand, the second integer represents the value of micro seconds.
   So, Groonga shows the value of Epoch as floating point. Integral part means the value of seconds, fraction part means the value of micro seconds.

The following example creates a ``Time`` column and updates existing
records. The first record (``"Monkey"`` key record) has the default
value (``0.0``) because it's not updated.

.. groonga-command
.. include:: ../example/tutorial/data-5.log
.. column_create --table ToyBox --name time --type Time
.. load --table ToyBox
.. [
.. {"_key":"Flower","time":1234567890.1234569999}
.. {"_key":"Block","time":-1234567890}
.. ]
.. select --table ToyBox --output_columns _key,time

Longitude and latitude types
----------------------------

The longitude and latitude types are divided according to the geographic coordinate system. For more details, see :doc:`/reference/types`. To represent a longitude and latitude, Groonga uses a string formatted as follows:

* "longitude x latitude" in milliseconds (e.g.: "128452975x503157902")
* "longitude x latitude" in degrees (e.g.: "35.6813819x139.7660839")

A number with/without a decimal point represents a longitude or latitude in milliseconds/degrees respectively. Note that a combination of a number with a decimal point and a number without a decimal point (e.g. 35.1x139) must not be used. A comma (',') is also available as a delimiter. The default value is "0x0".

The following example creates a ``WGS84GeoPoint`` column and updates
existing records. The second record (``"Flower"`` key record) has the
default value (``"0x0"``) because it's not updated.

.. groonga-command
.. include:: ../example/tutorial/data-6.log
.. column_create --table ToyBox --name location --type WGS84GeoPoint
.. load --table ToyBox
.. [
.. {"_key":"Monkey","location":"128452975x503157902"}
.. {"_key":"Block","location":"35.6813819x139.7660839"}
.. ]
.. select --table ToyBox --output_columns _key,location

Reference types
---------------

Groonga supports a reference column, which stores references to records in its associated table. In practice, a reference column stores the IDs of the referred records in the associated table and enables access to those records.

You can specify a column in the associated table to the ``output_columns`` parameter of a :doc:`/reference/commands/select` command. The format is ``Src.Dest`` where Src is the name of the reference column and Dest is the name of the target column. If only the reference column is specified, it is handled as ``Src._key``. Note that if a reference does not point to a valid record, a :doc:`/reference/commands/select` command outputs the default value of the target column.

The following example adds a reference column to the ``Site`` table
that was created in :ref:`tutorial-introduction-create-table`. The new
column, named ``link``, is designed for storing links among records in
the ``Site`` table.

.. groonga-command
.. include:: ../example/tutorial/data-7.log
.. column_create --table Site --name link --type Site
.. load --table Site
.. [
.. {"_key":"http://example.org/","link":"http://example.net/"}
.. ]
.. select --table Site --output_columns _key,title,link._key,link.title --query title:@this

The `type` parameter of the :doc:`/reference/commands/column_create` command specifies the table to be associated with the reference column. In this example, the reference column is associated with the own table. Then, the :doc:`/reference/commands/load` command registers a link from "http://example.org" to "http://example.net". Note that a reference column requires the primary key, not the ID, of the record to be referred to. After that, the link is confirmed by the :doc:`/reference/commands/select` command. In this case, the primary key and the title of the referred record are output because link._key and link.title are specified to the `output_columns` parameter.

Vector types
------------

Groonga supports a vector column, in which each element can store a variable number of values. To create a vector column, specify the COLUMN_VECTOR flag to the `flags` parameter of a :doc:`/reference/commands/column_create` command. A vector column is useful to represent a many-to-many relationship.

The previous example used a regular column, so each record could have at most one link. Obviously, the specification is insufficient because a site usually has more than one links. To solve this problem, the following example uses a vector column.

.. FIXME: _idの配列ではダメなのかどうか。検証する。

.. groonga-command
.. include:: ../example/tutorial/data-8.log
.. column_create --table Site --name links --flags COLUMN_VECTOR --type Site
.. load --table Site
.. [
.. {"_key":"http://example.org/","links":["http://example.net/","http://example.org/","http://example.com/"]},
.. ]
.. select --table Site --output_columns _key,title,links._key,links.title --query title:@this

The only difference at the first step is the `flags` parameter that specifies to create a vector column. The `type` parameter of the :doc:`/reference/commands/column_create` command is the same as in the previous example. Then, the :doc:`/reference/commands/load` command registers three links from "http://example.org/" to "http://example.net/", "http://example.org/" and "http://example.com/". After that, the links are confirmed by the :doc:`/reference/commands/select` command. In this case, the primary keys and the titles are output as arrays because links._key and links.title are specified to the `output_columns` parameter.
