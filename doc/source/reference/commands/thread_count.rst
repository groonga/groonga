.. -*- rst -*-

.. highlightlang:: none

.. groonga-command
.. database: commands_thread_count

.. groonga-command
.. thread_count 2

``thread_count``
================

Summary
-------

.. versionadded:: 5.0.7

``thread_count`` has the following two features:

  * It returns the max number of threads.
  * It sets the max number of threads.

:doc:`/reference/executables/groonga` is the only Groonga server that
supports full ``thread_count`` features.

:doc:`/reference/executables/groonga-httpd` supports only one feature
that returns the max number of threads. The max number of threads of
:doc:`/reference/executables/groonga-httpd` always returns ``1``
because :doc:`/reference/executables/groonga-httpd` uses single thread
model.

If you're using Groonga as a library, ``thread_count`` doesn't work
without you set custom functions by
:c:func:`grn_thread_set_get_count_func()` and
:c:func:`grn_thread_set_set_count_func()`. If you set a function by
:c:func:`grn_thread_set_get_count_func()`, the feature that returns
the max number of threads works. If you set a function by
:c:func:`grn_thread_set_set_count_func()`, the feature that sets the
max number of threads works.

Syntax
------

This command takes only one optional parameter::

  thread_count [new_count=null]

Usage
-----

You can get the max number of threads by calling without any parameters:

.. groonga-command
.. include:: ../../example/reference/commands/thread_count/usage_get.log
.. thread_count

If it returns ``0``, your Groonga server doesn't support the feature.

You can set the max number of threads by calling ``new_count`` parameter:

.. groonga-command
.. include:: ../../example/reference/commands/thread_count/usage_set.log
.. thread_count --new_count 4

It returns the previous max number of threads when you pass
``new_count`` parameter.

Parameters
----------

This section describes all parameters.

Required parameters
^^^^^^^^^^^^^^^^^^^

There is no required parameters.

Optional parameters
^^^^^^^^^^^^^^^^^^^

There is one optional parameter.

.. _thread-count-new-count:

``new_count``
"""""""""""""

Specifies the new max number of threads.

You must specify positive integer:

.. groonga-command
.. include:: ../../example/reference/commands/thread_count/new_count.log
.. thread_count --new_count 3

If you specify ``new_count`` parameter, ``thread_count`` returns the
max number of threads before ``new_count`` is applied.

Return value
------------

The command returns the max number of threads as body::

  [HEADER, N_MAX_THREADS]

If ``new_count`` is specified, ``N_MAX_THREADS`` is the max number of
threads before ``new_count`` is applied.

See :doc:`/reference/command/output_format` for ``HEADER``.
