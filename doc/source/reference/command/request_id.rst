.. -*- rst -*-

Request ID
==========

Summary
-------

.. versionadded:: 4.0.9

You can assign ID to each request.

The ID can be used by canceling the request. See also
:doc:`/reference/commands/request_cancel` for details about canceling
a request.

Request ID should be managed by user. If you assign the same ID for
some running requests, you can't cancel the request.

The simplest ID sequence is incremented numbers such as ``1``,
``2`` , ``...``.

A request ID is a string. The maximum request ID size is 4096 byte.

How to assign ID to request
---------------------------

All commands accept ``request_id`` parameter. You can assign ID to
request by adding ``request_id`` parameter.

Here is an example to assign ``id-1`` ID to a request::

  select Users --request_id id-1

See also
--------

* :doc:`/reference/commands/request_cancel`

