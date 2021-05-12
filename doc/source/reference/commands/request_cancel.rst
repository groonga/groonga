.. -*- rst -*-

``request_cancel``
==================

Summary
-------

.. versionadded:: 4.0.9

``request_cancel`` command cancels a running request.

There are some limitations:

  * Request ID must be managed by user. (You need to assign unique key
    for each request.)
  * Cancel request may be ignored. (You can send ``request_cancel``
    command multiple times for the same request ID.)
  * Only multithreading type Groonga server is supported. (You can use
    with :doc:`/reference/executables/groonga` based server but can't
    use with :doc:`/reference/executables/groonga-httpd`.)

See :doc:`/reference/command/request_id` about request ID.

If the request is canceled, a :doc:`/reference/command/return_code` of the canceled request as below.

  * ``-5`` (``GRN_INTERRUPTED_FUNCTION_CALL``) (Groonga version 6.0.1 before)
  * ``-77`` (``GRN_CANCEL``) (Groonga version 6.0.1 or later)

Syntax
------

This command takes only one required parameter::

  request_cancel id

Usage
-----

Here is an example of ``request_cancel`` command::

  $ curl 'http://localhost:10041/d/select?table=LargeTable&filter=true&request_id=unique-id-1' &
  # The above "select" takes a long time...
  # Point: "request_id=unique-id-1"
  $ curl 'http://localhost:10041/d/request_cancel?id=unique-id-1'
  [[...], {"id": "unique-id-1", "canceled": true}]
  # Point: "id=unique-id-1"

Assume that the first ``select`` command takes a long
time. ``unique-id-1`` request ID is assigned to the ``select`` command
by ``request_id=unique-id-1`` parameter.

The second ``request_cancel`` command passes ``id=unique-id-1``
parameter. ``unique-id-1`` is the same request ID passed in ``select``
command.

The ``select`` command may not be canceled immediately. And the cancel
request may be ignored.

You can send cancel request for the same request ID multiple times. If
the target request is canceled or finished, ``"canceled"`` value is
changed to ``false`` from ``true`` in return value::

  $ curl 'http://localhost:10041/d/request_cancel?id=unique-id-1'
  [[...], {"id": "unique-id-1", "canceled": true}]
  # "select" is still running... ("canceled" is "true")
  $ curl 'http://localhost:10041/d/request_cancel?id=unique-id-1'
  [[...], {"id": "unique-id-1", "canceled": true}]
  # "select" is still running... ("canceled" is "true")
  $ curl 'http://localhost:10041/d/request_cancel?id=unique-id-1'
  [[...], {"id": "unique-id-1", "canceled": false}]
  # "select" is canceled or finished. ("canceled" is "false")

If the ``select`` command is canceled, response of the ``select``
command has ``-5`` (``GRN_INTERRUPTED_FUNCTION_CALL``) as
:doc:`/reference/command/return_code`::

  $ curl 'http://localhost:10041/d/select?table=LargeTable&filter=true&request_id=unique-id-1' &
  [[-5, ...], ...]

Parameters
----------

This section describes parameters of ``request_cancel``.

Required parameters
^^^^^^^^^^^^^^^^^^^

There is required parameter, ``id``.

``id``
""""""

Specifies the ID for the target request.

Return value
------------

``request_cancel`` command returns the result of the cancel request::

  [
    HEADER,
    {
      "id":       ID,
      "canceled": CANCEL_REQUEST_IS_ACCEPTED_OR_NOT
    }
  ]

``HEADER``

  See :doc:`/reference/command/output_format` about ``HEADER``.

``ID``

  The ID of the target request.

``CANCEL_REQUEST_IS_ACCEPTED_OR_NOT``

  If the cancel request is accepted, this is ``true``, otherwise this
  is ``false``.

  Note that "cancel request is accepted" doesn't means that "the
  target request is canceled". It just means "cancel request is
  notified to the target request but the cancel request may be ignored
  by the target request".

  If request assigned with the request ID doesn't exist, this is
  ``false``.

See also
--------

* :doc:`/reference/command/request_id`
