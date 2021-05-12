.. -*- rst -*-

.. groonga-command
.. database: vector_size

``vector_size``
===============

Summary
-------

.. versionadded:: 5.0.3

``vector_size`` returns the value of vector column size.

To enable this function, register ``functions/vector`` plugin by
the following command::

  plugin_register functions/vector

Syntax
------

``vector_size`` requires one argument - ``target``.

::

  vector_size(target)

Usage
-----

Here is a schema definition and sample data.

Sample schema:

.. groonga-command
.. plugin_register functions/vector
.. include:: ../../example/reference/functions/vector_size/usage_setup_schema.log
.. table_create Memos TABLE_HASH_KEY ShortText
.. column_create Memos tags COLUMN_VECTOR ShortText

Sample data:

.. groonga-command
.. include:: ../../example/reference/functions/vector_size/usage_setup_data.log
.. load --table Memos
.. [
.. {"_key": "Groonga",          "tags": ["Groonga"]},
.. {"_key": "Rroonga",          "tags": ["Groonga", "Ruby"]},
.. {"_key": "Nothing"}
.. ]

Here is the simple usage of ``vector_size`` function which returns tags and size - the value of ``tags`` column and size of it.

.. groonga-command
.. include:: ../../example/reference/functions/vector_size/usage_only.log
.. select Memos --output_columns 'tags, vector_size(tags)'

Parameters
----------

There is only one required parameter.

``target``
^^^^^^^^^^

Specifies a vector column of table that is specified by ``table`` parameter in ``select``.

Return value
------------

``vector_size`` returns the value of target vector column size.
