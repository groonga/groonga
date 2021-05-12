.. -*- rst -*-

Data types
==========

Name
----

Groonga data types

Description
-----------

Groonga identifies data types to store.

A primary key of table and column value belong to some kind of data types in Groonga database. And normally, column values become in common with all records in one table.

A primary key type and column type can be specified Groonga defined types, user defined types or user defined table.

If you specify other table to primary key type, this table becomes subset of the table of primary key type.

If you specify other table to column type, this column becomes reference key of the table of column type.

Builtin types
-------------

The following types are defined as builtin types.

.. _builtin-type-bool:

``Bool``
^^^^^^^^

Boolean type. The possible values are true and false. (default: false)

To store a value by :doc:`/reference/commands/load` command, becomes false if you specify false, 0 or empty string, becomes true if you specify others.

.. _builtin-type-int8:

``Int8``
^^^^^^^^

Signed 8bit integer. It's -128 or more and 127 or less. (default: 0)

.. _builtin-type-uint8:

``UInt8``
^^^^^^^^^

Unsigned 8bit integer. Is't 0 or more and 255 or less. (default: 0)

.. _builtin-type-int16:

``Int16``
^^^^^^^^^

Signed 16bit integer. It's -32,768 or more and 32,767 or less. (default: 0)

.. _builtin-type-uint16:

``UInt16``
^^^^^^^^^^

Unsigned 16bit integer. It's 0 or more and 65,535 or less. (default: 0)

.. _builtin-type-int32:

``Int32``
^^^^^^^^^

Signed 32bit integer. It's -2,147,483,648 or more and 2,147,483,647 or less. (default: 0)

.. _builtin-type-uint32:

``UInt32``
^^^^^^^^^^

Unsigned 32bit integer. It's 0 or more and 4,294,967,295 or less. (default: 0)

.. _builtin-type-int64:

``Int64``
^^^^^^^^^

Signed 64bit integer. It's -9,223,372,036,854,775,808 or more and 9,223,372,036,854,775,807 or less. (default: 0)

.. _builtin-type-uint64:

``UInt64``
^^^^^^^^^^

Unsigned 64bit integer. It's 0 or more and 18,446,744,073,709,551,615 or less. (default: 0)

.. _builtin-type-float:

``Float32``
^^^^^^^^^^^

.. versionadded:: 10.0.2

Single-precision floating-point number of IEEE 754 as a real number. (default: 0.0)

See `IEEE floating point - Wikipedia, the free encyclopedia <http://en.wikipedia.org/wiki/IEEE_floating_point>`_ or `IEEE 754: Standard for Binary Floating-Point <http://grouper.ieee.org/groups/754/>`_ for details of IEEE 754 format.

``Float``
^^^^^^^^^

Double-precision floating-point number of IEEE 754 as a real number. (default: 0.0)

See `IEEE floating point - Wikipedia, the free encyclopedia <http://en.wikipedia.org/wiki/IEEE_floating_point>`_ or `IEEE 754: Standard for Binary Floating-Point <http://grouper.ieee.org/groups/754/>`_ for details of IEEE 754 format.

.. _builtin-type-time:

``Time``
^^^^^^^^

Date and Time, the number of seconds that have elapsed since 1970-01-01 00:00:00 by 64 bit signed integer. (default: 0)

To store a value by :doc:`/reference/commands/load` command, specifies the number of elapsed seconds since 1970-01-01 00:00:00. To specify the detailed date and time than seconds, use the decimal.

.. _builtin-type-short-text:

``ShortText``
^^^^^^^^^^^^^

String of 4,095 or less bytes. (default: "")

.. _builtin-type-text:

``Text``
^^^^^^^^

String of 65,535 or less bytes. (default: "")

.. _builtin-type-long-text:

``LongText``
^^^^^^^^^^^^

String of 2,147,483,647 or less bytes. (default: "")

.. _builtin-type-tokyo-geo-point:

``TokyoGeoPoint``
^^^^^^^^^^^^^^^^^

旧日本測地系による経緯度であり、経度と緯度をミリ秒単位で表現した整数の組により表現します。（デフォルト値: 0x0）

度分秒形式でx度y分z秒となる経度・緯度は、(((x * 60) + y) * 60 + z) * 1000という計算式でミリ秒単位へと変換されます。

:doc:`/reference/commands/load` コマンドで値を格納するときは、"ミリ秒単位の経度xミリ秒単位の緯度" もしくは "経度の小数表記x緯度の小数表記" という文字列表現を使って指定します。経度と緯度の区切りとしては、'x' のほかに ',' を使うことができます。

測地系の詳細については、 `測地系 - Wikipedia <http://ja.wikipedia.org/wiki/%E6%B8%AC%E5%9C%B0%E7%B3%BB>`_ を参照してください。

.. _builtin-type-wgs84-geo-point:

``WGS84GeoPoint``
^^^^^^^^^^^^^^^^^

世界測地系（World Geodetic System, WGS 84）による経緯度であり、経度と緯度をミリ秒単位で表現した整数の組により表現します。（デフォルト値: 0x0）

度分秒形式からミリ秒形式への変換方法や :doc:`/reference/commands/load` コマンドにおける指定方法はTokyoGeoPointと同じです。

Limitations about types
-----------------------

Types that can't be specified in primary key of table
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

``Text`` and ``LongText`` can't be specified in primary key of table.

ベクターとして格納できない型
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Groongaのカラムは、ある型のベクターを保存することができます。しかし、ShortText, Text, LongTextの３つの型についてはベクターとして保存したり出力したりすることはできますが、検索条件やドリルダウン条件に指定することができません。

テーブル型は、ベクターとして格納することができます。よって、ShortTextのベクターを検索条件やドリルダウン条件に使用したい場合には、主キーがShortText型のテーブルを別途作成し、そのテーブルを型として利用します。
