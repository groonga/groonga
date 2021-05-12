.. -*- rst -*-

.. groonga-command
.. database: commands_register

``register``
============

.. deprecated:: 5.0.1
   Use :doc:`plugin_register` instead.

Summary
-------

``register`` command registers a plugin. You need to register a plugin
before you use a plugin.

You need just one ``register`` command for a plugin in the same
database because registered plugin information is written into the
database.  When you restart your ``groonga`` process, ``groonga``
process loads all registered plugins without ``register`` command.

.. note::

   Registered plugins can be removed since Groonga 5.0.1. Use
   :doc:`plugin_unregister` in such a case.

Syntax
------

This command takes only one required parameter::

  register path

Usage
-----

Here is a sample that registers ``QueryExpanderTSV`` query expander
that is included in
``${PREFIX}/lib/groonga/plugins/query_expanders/tsv.so``.

.. groonga-command
.. include:: ../../example/reference/commands/register/query_expanders_tsv.log
.. register query_expanders/tsv

You can omit ``${PREFIX}/lib/groonga/plugins/`` and suffix (``.so``).
They are completed automatically.

You can specify absolute path such as ``register
/usr/lib/groonga/plugins/query_expanders/tsv.so``.

Return value
------------

``register`` returns ``true`` as body on success such as::

  [HEADER, true]

If ``register`` fails, error details are in ``HEADER``.

See :doc:`/reference/command/output_format` for ``HEADER``.

See also
--------

* :doc:`plugin_register`
* :doc:`plugin_unregister`
