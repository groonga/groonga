.. -*- rst -*-


``table_copy``
==============

Summary
-------

.. versionadded:: 6.0.8

TOOD: Write me

Syntax
------

This command takes two parameters.

All parameters are required::

  table_copy from_name
             to_name

Usage
-----


Parameters
----------

This section describes parameters.

Required parameters
^^^^^^^^^^^^^^^^^^^

All parameters are required.

``from_name``
"""""""""""""

Specifies the table name of source table.

``to_name``
"""""""""""

Specifies the destination table name.

Optional parameters
^^^^^^^^^^^^^^^^^^^

There is no optional parameter.

Return value
------------

The command returns ``true`` as body on success such as::

  [HEADER, true]

If the command fails, error details are in ``HEADER``.

See :doc:`/reference/command/output_format` for ``HEADER``.
