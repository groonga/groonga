.. -*- rst -*-

.. groonga-command
.. database: functions_cast_loose

``cast_loose``
================

Summary
-------

.. versionadded:: 8.0.8

``cast_loose`` cast loosely a string to the type specified.
If the target string can cast, ``cast_loose`` has cast the string to the type specified by the argument.
If the target string can't cast, ``cast_loose`` set the default value specified by the argument.

Syntax
------

``cast_loose`` has three parameters::

  cast_loose(type, value, defaul_value)

``type`` : Specify the type of after casted value.

``value`` : Specify the target of a cast.

``default_value`` : Speficy the value of setting when failed a cast.

Usage
-----

Here are a schema definition and sample data to show usage.

.. groonga-command
.. include:: ../../example/reference/functions/cast_loose/usage_setup.log
.. table_create Data TABLE_HASH_KEY ShortText
.. load --table Data
.. [
.. {"_key": "100abc"}
.. {"_key": "100"}
.. ]


The following example is cast "100" and "100abc" with ``cast_loose``.

.. groonga-command
.. include:: ../../example/reference/functions/cast_loose/usage_basic.log
.. select Data   --output_columns '_key, cast_loose(Int64, _key, 10)'

``cast_loose`` cast "100" to 100 and "100abc" to 10.
Because "100" can cast to the Int64 and "100abc" can't cast to the Int64.

Return value
------------

``cast_loose`` returns the casted value or default value.

See also
--------

* :doc:`/reference/commands/select`
