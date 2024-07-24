.. -*- rst -*-

``grn_ctx``
===========

Summary
-------

:c:type:`grn_ctx` is the most important object. :c:type:`grn_ctx`
keeps the current information such as:

* The last occurred error.
* The current encoding.
* The default thresholds. (e.g. :ref:`select-match-escalation-threshold`)
* The default command version. (See :doc:`/reference/command/command_version`)

:c:type:`grn_ctx` provides platform features such as:

* Memory management.
* Logging.

Most APIs receive :c:type:`grn_ctx` as the first argument.

You can't use the same :c:type:`grn_ctx` from two or more threads. You
need to create a :c:type:`grn_ctx` for a thread. You can use two or
more :c:type:`grn_ctx` in a thread but it is not needed for usual
use-case.

Example
-------

TODO...

Reference
---------

.. note::
   We are currently switching to automatic generation using Doxygen.
