.. -*- rst -*-

.. groonga-command
.. database: commands_logical_parameters

``logical_parameters``
======================

Summary
-------

.. versionadded:: 5.0.6

``logical_parameters`` is a command for test. Normally, you don't need
to use this command.

``logical_parameters`` provides the following two features:

  * It returns the current parameters for ``logical_*`` commands.
  * It sets new parameters for ``logical_*`` commands.

Here is a list of parameters:

  * :ref:`logical-parameters-range-index`

.. note::

   The parameters are independent in each thread. (To be exact, each
   :c:type:`grn_ctx`.) If you want to control the parameters
   perfectly, you should reduce the max number of threads to ``1`` by
   :doc:`/reference/commands/thread_limit` while you're using the
   parameters.

Syntax
------

This command takes only one optional parameter::

  logical_parameters [range_index=null]

Usage
-----

You need to register ``sharding`` plugin to use this command:

.. groonga-command
.. include:: ../../example/reference/commands/logical_parameters/usage_register.log
.. plugin_register sharding

You can get the all current parameter values by calling without
parameters:

.. groonga-command
.. include:: ../../example/reference/commands/logical_parameters/usage_get.log
.. logical_parameters

You can set new values by calling with parameters:

.. groonga-command
.. include:: ../../example/reference/commands/logical_parameters/usage_set.log
.. logical_parameters --range_index never

``logical_parameters`` returns the parameter values before new values
are set when you set new values.

Parameters
----------

This section describes parameters.

Required parameters
^^^^^^^^^^^^^^^^^^^

There is no required parameter.

Optional parameters
^^^^^^^^^^^^^^^^^^^

There is one optional parameter.

.. _logical-parameters-range-index:

``range_index``
"""""""""""""""

Specifies how to use range index in :doc:`logical_range_filter` by
keyword.

Here are available keywords:

  * ``auto`` (default)
  * ``always``
  * ``never``

If ``auto`` is specified, range index is used only when it'll be
efficient. This is the default value.

.. groonga-command
.. include:: ../../example/reference/commands/logical_parameters/range_index_auto.log
.. logical_parameters --range_index auto

If ``always`` is specified, range index is always used. It'll be
useful for testing a case that range index is used.

.. groonga-command
.. include:: ../../example/reference/commands/logical_parameters/range_index_always.log
.. logical_parameters --range_index always

If ``never`` is specified, range index is never used. It'll be useful
for testing a case that range index isn't used.

.. groonga-command
.. include:: ../../example/reference/commands/logical_parameters/range_index_never.log
.. logical_parameters --range_index never

Return value
------------

The command returns the current parameters for ``logical_*`` command::

  [
    HEADER,
    {"range_index": HOW_TO_USE_RANGE_INDEX}
  ]

``HOW_TO_USE_RANGE_INDEX`` value is one of the followings:

  * ``"auto"``
  * ``"always"``
  * ``"never"``

See :doc:`/reference/command/output_format` for ``HEADER``.
