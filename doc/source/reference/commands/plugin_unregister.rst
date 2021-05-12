.. -*- rst -*-

.. groonga-command
.. database: commands_plugin_unregister

``plugin_unregister``
=====================

.. note::

   This command is an experimental feature.

.. versionadded:: 5.0.1

Summary
-------

``plugin_unregister`` command unregisters a plugin.

Syntax
------

This command takes only one required parameter::

  plugin_unregister name

Usage
-----

Here is a sample that unregisters ``QueryExpanderTSV`` query expander
that is included in
``${PREFIX}/lib/groonga/plugins/query_expanders/tsv.so``.

.. groonga-command
.. plugin_register query_expanders/tsv

.. groonga-command
.. include:: ../../example/reference/commands/plugin_unregister/query_expanders_tsv.log
.. plugin_unregister query_expanders/tsv

You can omit ``${PREFIX}/lib/groonga/plugins/`` and suffix (``.so``).
They are completed automatically.

You can specify absolute path such as ``plugin_unregister
/usr/lib/groonga/plugins/query_expanders/tsv.so``.

Return value
------------

``plugin_unregister`` returns ``true`` as body on success such as::

  [HEADER, true]

If ``plugin_unregister`` fails, error details are in ``HEADER``.

See :doc:`/reference/command/output_format` for ``HEADER``.

See also
--------

* :doc:`plugin_register`
