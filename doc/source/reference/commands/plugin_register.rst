.. -*- rst -*-

.. highlightlang:: none

.. groonga-command
.. database: commands_plugin_register

``plugin_register``
===================

.. versionadded:: 5.0.1

Summary
-------

``plugin_register`` command registers a plugin. You need to register a plugin
before you use a plugin.

You need just one ``plugin_register`` command for a plugin in the same
database because registered plugin information is written into the
database.  When you restart your ``groonga`` process, ``groonga``
process loads all registered plugins without ``plugin_register`` command.

Syntax
------

``plugin_register`` has a parameter ``path``. It is required parameter::

  plugin_register path

Usage
-----

Here is a sample that registers ``QueryExpanderTSV`` query expander
that is included in
``${PREFIX}/lib/groonga/plugins/query_expanders/tsv.so``.

.. groonga-command
.. include:: ../../example/reference/commands/plugin_register/query_expanders_tsv.log
.. register query_expanders/tsv

You can omit ``${PREFIX}/lib/groonga/plugins/`` and suffix (``.so``).
They are completed automatically.

You can specify absolute path such as ``plugin_register
/usr/lib/groonga/plugins/query_expanders/tsv.so``.

Return value
------------

``plugin_register`` returns ``true`` as body on success such as::

  [HEADER, true]

If ``plugin_register`` fails, error details are in ``HEADER``.

See :doc:`/reference/command/output_format` for ``HEADER``.
