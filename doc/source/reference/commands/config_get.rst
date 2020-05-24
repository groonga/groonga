.. -*- rst -*-

.. highlightlang:: none

.. groonga-command
.. database: config_get

``config_get``
==============

Summary
-------

.. versionadded:: 5.1.2

``config_get`` command returns the value of the specified
configuration item.

Syntax
------

This command takes only one required parameter::

  config_get key

Usage
-----

Here is an example to set a value to ``alias.column``
configuration item and get the value:

.. groonga-command
.. include:: ../../example/reference/commands/config_get/existent.log
.. config_set alias.column Aliases.real_name
.. config_get alias.column

Here is an example to get nonexistent configuration item value:

.. groonga-command
.. include:: ../../example/reference/commands/config_get/nonexistent.log
.. config_get nonexistent

``config_get`` returns an empty string for nonexistent configuration
item key.

Parameters
----------

This section describes all parameters.

Required parameters
^^^^^^^^^^^^^^^^^^^

There is only one required parameter.

.. _config-get-key:

``key``
"""""""

Specifies the key of target configuration item.

The max key size is 4KiB.

You can't use an empty string as key.

Optional parameters
^^^^^^^^^^^^^^^^^^^

There is no optional parameter.

Return value
------------

``config_get`` command returns the value of the specified
configuration item::

  [HEADER, VALUE]

``HEADER``
^^^^^^^^^^

See :doc:`/reference/command/output_format` about ``HEADER``.

``VALUE``
^^^^^^^^^^

``VALUE`` is the value of the configuration item specified by
``key``. It's a string.

See also
--------

  * :doc:`/reference/configuration`
  * :doc:`config_set`
  * :doc:`config_delete`
