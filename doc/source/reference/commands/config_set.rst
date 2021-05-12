.. -*- rst -*-

.. groonga-command
.. database: config_set

``config_set``
==============

Summary
-------

.. versionadded:: 5.1.2

``config_set`` command sets a value to the specified
configuration item.

Syntax
------

This command takes two required parameters::

  config_set key value

Usage
-----

Here is an example to set a value to ``alias.column``
configuration item and confirm the set value:

.. groonga-command
.. include:: ../../example/reference/commands/config_set/set_and_get.log
.. config_set alias.column Aliases.real_name
.. config_get alias.column

Parameters
----------

This section describes all parameters.

Required parameters
^^^^^^^^^^^^^^^^^^^

There are required parameters.

.. _config-set-key:

``key``
"""""""

Specifies the key of target configuration item.

The max key size is 4KiB.

You can't use an empty string as key.

.. _config-set-value:

``value``
"""""""""

Specifies the value of the target configuration item specified by
``key``.

The max value size is 4091B (= 4KiB - 5B).

Optional parameters
^^^^^^^^^^^^^^^^^^^

There is no optional parameter.

Return value
------------

``config_set`` command returns whether setting a configuration item
value is succeeded or not::

  [HEADER, SUCCEEDED_OR_NOT]

``HEADER``
^^^^^^^^^^

See :doc:`/reference/command/output_format` about ``HEADER``.

``SUCCEEDED_OR_NOT``
^^^^^^^^^^^^^^^^^^^^

If command succeeded, it returns true, otherwise it returns false on error.

See also
--------

  * :doc:`/reference/configuration`
  * :doc:`config_get`
  * :doc:`config_delete`
