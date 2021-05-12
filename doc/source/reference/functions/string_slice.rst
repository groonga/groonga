.. -*- rst -*-

.. groonga-command
.. database: functions_string_slice

``string_slice``
================

.. versionadded:: 11.0.3

Summary
-------

``string_slice`` extracts a substring of a string. You can use two different extraction methods depending on the arguments.

* Extraction by position
* Extraction by regular expression

Groonga uses the same regular expression syntax in Ruby.

To enable this function, register ``functions/string`` plugin by following the command::

  plugin_register functions/string

Syntax
------

``string_slice`` requires two to four parameters. The required parametars are depending on the extraction method.

Extraction by position
^^^^^^^^^^^^^^^^^^^^^^

::

  string_slice(target, nth[, options])``
  string_slice(target, nth, length[, options])``

``options`` uses the following format. All of key-value pairs are optional::

  {
    "default_value": default_value
  }

Extraction by regular expression
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 
::

  string_slice(target, regexp, nth[, options])
  string_slice(target, regexp, name[, options])

``options`` uses the following format. All of key-value pairs are optional::

  {
    "default_value": default_value
  }


Usage
-----

Here are a schema definition and sample data to show usage.

Sample schema:

.. groonga-command
.. include:: ../../example/reference/functions/string_slice/usage_setup_schema.log
.. plugin_register functions/string
.. table_create Memos TABLE_HASH_KEY ShortText


Sample data:

.. groonga-command
.. include:: ../../example/reference/functions/string_slice/usage_setup_data.log
.. load --table Memos
.. [
.. {"_key": "Groonga"}
.. ]

Here is a simple example for the extraction by position.

.. groonga-command
.. include:: ../../example/reference/functions/string_slice/usage_number.log
.. select Memos --output_columns '_key, string_slice(_key, 2, 3)'

Here are simple examples for the extraction by regular expression.

In the following example, extracting by specifying the group number of the capturing group: ``(subexp)``.

.. groonga-command
.. include:: ../../example/reference/functions/string_slice/usage_regexp_number.log
.. select Memos --output_columns '_key, string_slice(_key, "(Gro+)(.*)", 2)'

In the following example, extracting by specifying the name of the named capturing group: ``(?<name>subexp)``.

.. groonga-command
.. include:: ../../example/reference/functions/string_slice/usage_regexp_name.log
.. select Memos --output_columns '_key, string_slice(_key, "(Gr)(?<Name1>o*)(?<Name2>.*)", "Name1")'

In the following example, specifying the default value.

.. groonga-command
.. include:: ../../example/reference/functions/string_slice/usage_regexp_default.log
.. select Memos --output_columns '_key, string_slice(_key, "mismatch", 2, { "default_value" : "default" })'

You can specify string literal instead of column.

.. groonga-command
.. include:: ../../example/reference/functions/string_slice/usage_string_literal.log
.. select Memos --output_columns 'string_slice("Groonga", "(roon)(g)", 2)'

Parameters
----------

Extraction by position
^^^^^^^^^^^^^^^^^^^^^^

There are two required parameters, ``target`` and ``nth``.

There are two optional parameters, ``length`` and ``options``.

``target``
~~~~~~~~~~

Specify a string literal or a string type column.

``nth``
~~~~~~~

Specify a 0-based index number of charactors where to start the extraction from ``target``.

If you specify a negative value, it counts from the end of ``target``.

``length``
~~~~~~~~~~

Specify a number of characters to extract from ``nth``.

The default is 1.

``options``
~~~~~~~~~~~

Specify the following key.

``default_value``
  Specify a string to be returned when a substring is an empty string except when specifying 0 for ``length``.
  
  The default is an empty string.

Extraction by regular expression
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

There are three required parameters, ``target`` and ``regexp`` and ``nth`` or ``name``. Specify either ``nth`` or ``name``.

There is one optional parameter, ``options``.

``target``
~~~~~~~~~~

Specify a string literal or a string type column.

``regexp``
~~~~~~~~~~

Specify a regular expression string.

When you use ``nth`` and specify a value greater than 0, you must use capturing group: ``(subexp)``.

When you use ``name``, you must use named capturing group: ``(?<name>subexp)``, ``(?'name'subexp)``.

``nth``
~~~~~~~

Specify a number of the capturing group for ``regexp``.

A captured string of the ``nth`` capturing group is returned when ``regexp`` is matched to ``target``.

If 0 is specified for ``nth``, the entire string that matches ``regexp`` is returned.

Specify either ``nth`` or ``name``.

``name``
~~~~~~~~

Specify a name of the named capturing group for ``regexp``.

A captured string of the named capturing group that matches ``name`` is returned 
when ``regexp`` is matched to ``target``.

Specify either ``nth`` or ``name``.

``options``
~~~~~~~~~~~

Specify the following key.

``default_value``
  Specify a string returned if ``regexp`` does not match to ``target``.
  This value also be returned when the value of ``nth`` or ``name`` is incorrect.

  The default is an empty string.

Return value
------------

``string_slice`` returns a substring extracted under the specified conditions from ``target``.

