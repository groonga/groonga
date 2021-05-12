.. -*- rst -*-

.. groonga-command
.. database: commands_delete

``delete``
==========

Summary
-------

``delete`` command deletes specified record of table.

.. _cascade-delete:

Cascade delete
^^^^^^^^^^^^^^

There is a case that multiple table is associated. For example, the key of one table are referenced by other table's records. In such a case, if you delete the key of one table, other table's records are also removed.

Note that the type of other table's column is COLUMN_VECTOR, only the value of referencing key is removed from the vector value.

Syntax
------
::

 delete table [key [id [filter]]]

Usage
-----

Here are a schema definition and sample data to show usage.

.. groonga-command
.. table_create Entry TABLE_HASH_KEY UInt32
.. column_create Entry status --type ShortText
.. load --table Entry
.. [
.. {"_key": 1, "status": "OK"}
.. {"_key": 2, "status": "NG"}
.. ]

Delete the record from Entry table which has "2" as the key.

.. groonga-command
.. include:: ../../example/reference/commands/delete/status.log
.. delete Entry 2
.. select Entry

Here is the example about cascaded delete.

The country column of Users table associates with Country table.

"Cascaded delete" removes the records which matches specified key and refers that key.

.. groonga-command
.. include:: ../../example/reference/commands/delete/cascade.log
.. table_create Country TABLE_HASH_KEY ShortText
.. table_create Users TABLE_HASH_KEY UInt32
.. column_create Users name COLUMN_SCALAR ShortText
.. column_create Users country COLUMN_SCALAR Country
.. load --table Users
.. [
.. {"_key": 1, "name": "John", country: "United States"}
.. {"_key": 2, "name": "Mike", country: "United States"}
.. {"_key": 3, "name": "Takashi", country: "Japan"}
.. {"_key": 4, "name": "Hanako", country: "Japan"}
.. ]
.. load --table Country
.. [
.. {"_key": "United States"}
.. {"_key": "Japan"}
.. ]
.. delete Country "United States"
.. select Country
.. select Users


Parameters
----------

``table``

  Specifies the name of table to delete the records.

``key``

  Specifies the key of record to delete. If you use the table with TABLE_NO_KEY, the key is just ignored.
  (Use ``id`` parameter in such a case)

``id``

  Specifies the id of record to delete. If you specify ``id`` parameter, you must not specify ``key`` parameter.

``filter``

  Specifies the expression of grn_expr to identify the record. If you specify ``filter`` parameter,
  you must not specify ``key`` and ``id`` parameter.

Return value
------------

::

 [HEADER, SUCCEEDED_OR_NOT]

``HEADER``

  See :doc:`/reference/command/output_format` about ``HEADER``.

``SUCCEEDED_OR_NOT``

  If command succeeded, it returns true, otherwise it returns false on error.


See also
--------

:doc:`load`

