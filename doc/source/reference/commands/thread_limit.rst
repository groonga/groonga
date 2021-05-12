.. -*- rst -*-

.. groonga-command
.. database: commands_thread_limit

.. groonga-command
.. thread_limit 2

``thread_limit``
================

Summary
-------

.. versionadded:: 5.0.7

``thread_limit`` has the following two features:

  * It returns the max number of threads.
  * It sets the max number of threads.

:doc:`/reference/executables/groonga` is the only Groonga server that
supports full ``thread_limit`` features.

:doc:`/reference/executables/groonga-httpd` supports only one feature
that returns the max number of threads. The max number of threads of
:doc:`/reference/executables/groonga-httpd` always returns ``1``
because :doc:`/reference/executables/groonga-httpd` uses single thread
model.

If you're using Groonga as a library, ``thread_limit`` doesn't work
without you set custom functions by
:c:func:`grn_thread_set_get_limit_func()` and
:c:func:`grn_thread_set_set_limit_func()`. If you set a function by
:c:func:`grn_thread_set_get_limit_func()`, the feature that returns
the max number of threads works. If you set a function by
:c:func:`grn_thread_set_set_limit_func()`, the feature that sets the
max number of threads works.

Syntax
------

This command takes only one optional parameter::

  thread_limit [max=null]

Usage
-----

You can get the max number of threads by calling without any parameters:

.. groonga-command
.. include:: ../../example/reference/commands/thread_limit/usage_get.log
.. thread_limit

If it returns ``0``, your Groonga server doesn't support the feature.

You can set the max number of threads by calling ``max`` parameter:

.. groonga-command
.. include:: ../../example/reference/commands/thread_limit/usage_set.log
.. thread_limit --max 4

It returns the previous max number of threads when you pass
``max`` parameter.

Parameters
----------

This section describes all parameters.

Required parameters
^^^^^^^^^^^^^^^^^^^

There is no required parameter.

Optional parameters
^^^^^^^^^^^^^^^^^^^

There is one optional parameter.

.. _thread-limit-max:

``max``
"""""""

Specifies the new max number of threads.

You must specify positive integer:

.. groonga-command
.. include:: ../../example/reference/commands/thread_limit/max.log
.. thread_limit --max 3

If you specify ``max`` parameter, ``thread_limit`` returns the
max number of threads before ``max`` is applied.

Return value
------------

The command returns the max number of threads as body::

  [HEADER, N_MAX_THREADS]

If ``max`` is specified, ``N_MAX_THREADS`` is the max number of
threads before ``max`` is applied.

See :doc:`/reference/command/output_format` for ``HEADER``.
