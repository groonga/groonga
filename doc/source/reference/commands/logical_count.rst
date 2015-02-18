.. -*- rst -*-

.. highlightlang:: none

.. groonga-command
.. database: logical_count

``logical_count``
=================

Summary
-------

.. note::

  ``logical_count`` command is an experimental feature.

.. versionadded:: 5.0.0

``logical_count`` is a command to count matched records even though actual records are stored into parted tables. It is useful for users because there is less need to care about maximum records of table :doc:`/limitations`.

Note that this feature is not matured yet, so there are some limitations.

* Create parted tables which contains "_YYYYMMDD" postfix. It is hardcoded, so you must create tables by each day.
* Load proper data into parted tables on your own.

Syntax
------

Usage
-----

Register ``sharding`` plugin to use ``logical_count`` command in advance.

Note that ``logical_count`` is implemented as an experimental plugin, and the specification may be changed in the future.

Here is the simple example which shows how to use this feature. Let's consider to count specified logs which are stored into multiple tables.

Here is the schema and data.

.. groonga-command
.. include:: ../../example/reference/commands/logical_count/setup_schema.log
.. table_create Logs_20150203 TABLE_NO_KEY
.. column_create Logs_20150203 timestamp COLUMN_SCALAR Time
.. column_create Logs_20150203 message COLUMN_SCALAR Text
.. table_create Logs_20150204 TABLE_NO_KEY
.. column_create Logs_20150204 timestamp COLUMN_SCALAR Time
.. column_create Logs_20150204 message COLUMN_SCALAR Text
.. table_create Logs_20150205 TABLE_NO_KEY
.. column_create Logs_20150205 timestamp COLUMN_SCALAR Time
.. column_create Logs_20150205 message COLUMN_SCALAR Text

.. groonga-command
.. include:: ../../example/reference/commands/logical_count/setup_data.log
.. load --table Logs_20150203
.. [
.. {"timestamp": "2015-02-03 23:59:58", "message": "Start"},
.. {"timestamp": "2015-02-03 23:59:58", "message": "Shutdown"},
.. {"timestamp": "2015-02-03 23:59:59", "message": "Start"},
.. {"timestamp": "2015-02-03 23:59:59", "message": "Shutdown"}
.. ]
.. load --table Logs_20150204
.. [
.. {"timestamp": "2015-02-04 00:00:00", "message": "Start"},
.. {"timestamp": "2015-02-04 00:00:00", "message": "Shutdown"},
.. {"timestamp": "2015-02-04 00:00:01", "message": "Start"},
.. {"timestamp": "2015-02-04 00:00:01", "message": "Shutdown"},
.. {"timestamp": "2015-02-04 23:59:59", "message": "Start"},
.. {"timestamp": "2015-02-04 23:59:59", "message": "Shutdown"}
.. ]
.. load --table Logs_20150205
.. [
.. {"timestamp": "2015-02-05 00:00:00", "message": "Start"},
.. {"timestamp": "2015-02-05 00:00:00", "message": "Shutdown"},
.. {"timestamp": "2015-02-05 00:00:01", "message": "Start"},
.. {"timestamp": "2015-02-05 00:00:01", "message": "Shutdown"}
.. ]

There are three tables which are mapped each day from 2015 Feb 03 to 2015 Feb 05.

* Logs_20150203
* Logs_20150204
* Logs_20150205

Then, it loads data into each table which correspond to.

Let's count logs which contains "Shutdown" in timestamp column and the value of timestamp is "2015-02-04 00:00:00" or later.

Here is the query to achieve above purpose.

.. groonga-command
.. include:: ../../example/reference/commands/logical_count/count_shutdown.log
.. logical_count Logs timestamp --filter 'message == "Shutdown"' --min "2015-02-04 00:00:00" --min_border "include"

There is a well known limitation about the number of records. By sharding feature, you can overcome such limitations because such a limitation is applied per table.

.. note::

   There is no convenient query such as PARTITIONING BY in SQL. Thus, you must create table by ``table_create`` for each tables which contains "_YYYYMMDD" postfix in table name.

Parameters
----------

Return value
------------

