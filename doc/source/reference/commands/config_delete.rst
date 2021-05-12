.. -*- rst -*-

.. groonga-command
.. database: config_delete

``config_delete``
=================

Summary
-------

.. versionadded:: 5.1.2

``config_delete`` command deletes the specified configuration item.

Syntax
------

This command takes only one required parameter::

  config_delete key

Usage
-----

Here is an example to delete ``alias.column`` configuration item:

.. groonga-command
.. include:: ../../example/reference/commands/config_delete/existent.log
.. config_set alias.column Aliases.real_name
.. config_get alias.column
.. config_delete alias.column
.. config_get alias.column

Here is an example to delete nonexistent configuration item:

.. groonga-command
.. include:: ../../example/reference/commands/config_delete/nonexistent.log
.. config_delete nonexistent

``config_delete`` returns an error when you try to delete nonexistent
configuration item.

Parameters
----------

This section describes all parameters.

Required parameters
^^^^^^^^^^^^^^^^^^^

There is only one required parameter.

.. _config-delete-key:

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

``config_delete`` command returns whether deleting a configuration
item is succeeded or not::

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
  * :doc:`config_set`
