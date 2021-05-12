.. -*- rst -*-

.. groonga-command
.. database: functions_vector_find

``vector_find``
===============

Summary
-------

.. versionadded:: 8.0.4

It returns the first element that matches the given condition from the
given vector. If no element is found, it returns ``null``.

You can use not only equal condition but also less than condition,
prefix equal condition and so on.

To enable this function, register ``functions/vector`` plugin by
the following command::

  plugin_register functions/vector

.. _vector-find-syntax:

Syntax
------

``vector_find`` has two or three parameters::

  vector_find(vector, value)
  vector_find(vector, value, mode)

If you omit the third argument, each element in the ``vector`` is
compared with ``value`` by equality comparison.

.. _vector-find-usage:

Usage
-----

You need to register ``functions/vector`` plugin at first:

.. groonga-command
.. include:: ../../example/reference/functions/vector_find/usage_register.log
.. plugin_register functions/vector

Here is a schema definition and sample data.

Sample schema:

.. groonga-command
.. include:: ../../example/reference/functions/vector_find/usage_setup_schema.log
.. table_create  Memos TABLE_HASH_KEY ShortText
.. column_create Memos tags COLUMN_VECTOR ShortText

Sample data:

.. groonga-command
.. include:: ../../example/reference/functions/vector_find/usage_setup_data.log
.. load --table Memos
.. [
.. {"_key": "Groonga is fast",          "tags": ["groonga"]},
.. {"_key": "Mroonga is fast",          "tags": ["mroonga", "groonga"]},
.. {"_key": "Groonga is very good!",    "tags": ["groonga"]},
.. {"_key": "Droonga is fast",          "tags": ["droonga", "groonga"]},
.. {"_key": "Groonga is a HTTP server", "tags": ["groonga", "http"]}
.. ]

Here is a simple usage of ``vector_find`` that searches an element in
``tags`` column and returns the first found element:

.. groonga-command
.. include:: ../../example/reference/functions/vector_find/usage_find.log
.. select \
..   --table Memos \
..   --output_columns 'tags, vector_find(tags, "mroonga")'

It returns ``"mroonga"`` when the ``tags`` column value includes
``"mroonga"`` element. It returns ``null`` otherwise.

You can customize how to compare with each value by the third
argument. Here is a usage to use full text search to find an element:

.. groonga-command
.. include:: ../../example/reference/functions/vector_find/usage_find_mode.log
.. select \
..   --table Memos \
..   --output_columns 'tags, vector_find(tags, "roonga", "@")'

It returns ``"groonga"``, ``"mroonga"`` or ``"droonga"`` when the
``tags`` column value includes one of them. The returned value is the
first found element. For example, ``"droonga"`` is returned for
``["droonga", "groonga"]``. ``"groonga"`` isn't returned because the
first element ``"droonga"`` is found before the second element
``"groonga"`` is searched.

It returns ``null`` when ``tags`` column value doesn't include them.

.. _vector-find-parameters:

Parameters
----------

It requires two parameters.

There is an optional parameter.

.. _vector-find-required-parameters:

Required parameters
^^^^^^^^^^^^^^^^^^^

``vector`` and ``value`` are required.

.. _vector-find-vector:

``vector``
""""""""""

Specifies a vector value to be searched an element.

.. _vector-find-value:

``value``
"""""""""

Specifies a value to be compared.

.. _vector-find-optional-parameters:

Optional parameters
^^^^^^^^^^^^^^^^^^^

``mode`` is optional.

.. _vector-find-mode:

``mode``
""""""""

Specifies a mode that specifies how to compare each element with
:ref:`vector-find-value`.

See :ref:`query-default-mode` for available mode names. All mode names
except ``"NEAR"``, ``"SIMILAR"`` and ``"SUFFIX"`` are supported.

The default mode is ``"EQUAL"``. Note that :ref:`query-default-mode`
says the default mode is ``"MATCH"`` but the default mode of
``vector_find`` is ``"EQUAL"``.

Return value
------------

The matched element on match, ``null`` otherwise.
