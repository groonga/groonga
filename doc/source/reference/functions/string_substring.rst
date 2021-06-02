.. -*- rst -*-

.. groonga-command
.. database: string_substring

``string_substring``
====================

.. versionadded:: 6.0.7

Summary
-------

``string_substring`` extracts a substring of a string by position.

To enable this function, register ``functions/string`` plugin by following the command::

  plugin_register functions/string

Syntax
------

``string_substring`` requires two to four parameters.

::

  string_substring(target, nth[, options])
  string_substring(target, nth, length[, options])

``options`` uses the following format. All of key-value pairs are optional::

  {
    "default_value": default_value
  }

Usage
-----

Here are a schema definition and sample data to show usage.

Sample schema:

.. groonga-command
.. include:: ../../example/reference/functions/string_substring/usage_setup_schema.log
.. plugin_register functions/string
.. table_create Memos TABLE_HASH_KEY ShortText

Sample data:

.. groonga-command
.. include:: ../../example/reference/functions/string_substring/usage_setup_data.log
.. load --table Memos
.. [
.. {"_key": "Groonga"}
.. ]

Here is a simple example for the extraction by position.

.. groonga-command
.. include:: ../../example/reference/functions/string_substring/usage_number.log
.. select Memos --output_columns '_key, string_substring(_key, 2, 3)'

In the following example, specifying the default value.

.. groonga-command
.. include:: ../../example/reference/functions/string_substring/usage_default.log
.. select Memos --output_columns '_key, string_substring(_key, 50, 1, { "default_value" : "default" })'

You can specify string literal instead of column.

.. groonga-command
.. include:: ../../example/reference/functions/string_substring/usage_string_literal.log
.. select Memos --output_columns 'string_substring("Groonga", 2, 3)'

Parameters
----------

There are two required parameters, ``target`` and ``nth``.

There are two optional parameters, ``length`` and ``options``.

``target``
^^^^^^^^^^

Specify a string literal or a string type column.

``nth``
^^^^^^^

Specify a 0-based index number of charactors where to start the extraction from ``target``.

If you specify a negative value, it counts from the end of ``target``.

``length``
^^^^^^^^^^

Specify a number of characters to extract from ``nth``.

If you omit or specify a negative value, this function extracts from ``nth`` to the end.

``options``
^^^^^^^^^^^

.. versionadded:: 11.0.3

Specify the following key.

``default_value``
  Specify a string to be returned when a substring is an empty string except when specifying 0 for ``length``.
  
  The default is an empty string.

Return value
------------

``string_substring`` returns a substring extracted under the specified conditions from ``target``.